#include "pdb_io.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <initializer_list>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "atmtypenumbers_data.h"

#if VOSS_HAVE_GEMMI
#include <gemmi/mmcif.hpp>
#include <gemmi/pdb.hpp>
#include <gemmi/structure.hpp>
#endif

namespace vossvolvox::pdbio {
namespace {

std::string trim(std::string_view view) {
    const auto first = view.find_first_not_of(' ');
    if (first == std::string_view::npos) {
        return {};
    }
    const auto last = view.find_last_not_of(' ');
    return std::string(view.substr(first, last - first + 1));
}

std::string to_upper(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](char c) {
        return static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    });
    return value;
}

std::string get_field(const std::string& line, std::size_t start, std::size_t length) {
    if (line.size() <= start) {
        return {};
    }
    const auto real_length = std::min(length, line.size() - start);
    return line.substr(start, real_length);
}

std::string format_coordinate(double value) {
    std::ostringstream oss;
    oss.setf(std::ios::fixed, std::ios::floatfield);
    oss << std::setw(8) << std::setprecision(3) << value;
    return oss.str();
}

struct RadiusEntry {
    std::string explicit_text{"0.01"};
    std::string united_text{"0.01"};
};

struct PatternEntry {
    std::regex residue;
    std::regex atom;
    std::string key;
};

class AtomTypeLibrary {
   public:
    AtomTypeLibrary() { load(); }

    std::pair<bool, std::string> radius_for(const std::string& residue,
                                            const std::string& atom,
                                            bool use_united) const {
        for (const auto& entry : patterns_) {
            if (std::regex_search(residue, entry.residue) &&
                std::regex_search(atom, entry.atom)) {
                const auto it = radii_.find(entry.key);
                if (it != radii_.end()) {
                    return {true, use_united ? it->second.united_text
                                             : it->second.explicit_text};
                }
            }
        }
        return {false, std::string("0.01")};
    }

   private:
    void load() {
        using namespace std::regex_constants;
        std::istringstream input(vossvolvox::kAtmTypeNumbers);
        std::string line;
        while (std::getline(input, line)) {
            const auto hash_pos = line.find('#');
            if (hash_pos != std::string::npos) {
                line = line.substr(0, hash_pos);
            }
            if (line.find_first_not_of(" \t\r\n") == std::string::npos) {
                continue;
            }
            std::istringstream line_stream(line);
            std::vector<std::string> tokens;
            std::string token;
            while (line_stream >> token) {
                tokens.push_back(token);
            }
            if (tokens.empty()) {
                continue;
            }

            if (tokens[0] == "radius") {
                if (tokens.size() < 4) {
                    continue;
                }
                const std::string key = tokens[1];
                const std::string explicit_text = tokens[3];
                try {
                    (void)std::stod(explicit_text);
                } catch (const std::exception& err) {
                    std::cerr << "pdb_io: warning: invalid radius '" << explicit_text
                              << "' for key " << key << ": " << err.what() << "\n";
                    continue;
                }

                std::string united_text = explicit_text;
                if (tokens.size() >= 5) {
                    const std::string candidate = tokens[4];
                    try {
                        (void)std::stod(candidate);
                        united_text = candidate;
                    } catch (...) {
                        united_text = explicit_text;
                    }
                }
                radii_[key] = RadiusEntry{explicit_text, united_text};
                continue;
            }

            if (tokens.size() < 3) {
                continue;
            }
            std::string residue_pattern = tokens[0] == "*" ? ".*" : tokens[0];
            std::string atom_pattern = tokens[1];
            std::replace(atom_pattern.begin(), atom_pattern.end(), '_', ' ');
            residue_pattern = "^" + residue_pattern + "$";
            atom_pattern = "^" + atom_pattern + "$";

            const std::string key = tokens[2];
            if (!radii_.count(key)) {
                radii_[key] = RadiusEntry{};
                std::cerr << "pdb_io: warning: entry " << tokens[0] << ' ' << tokens[1] << ' '
                          << key << " has no corresponding radius value\n";
            }

            try {
                patterns_.push_back(PatternEntry{std::regex(residue_pattern, extended),
                                                 std::regex(atom_pattern, extended),
                                                 key});
            } catch (const std::regex_error& err) {
                std::cerr << "pdb_io: warning: invalid regex on line '" << line
                          << "': " << err.what() << "\n";
            }
        }
    }

    std::vector<PatternEntry> patterns_;
    std::unordered_map<std::string, RadiusEntry> radii_;
};

struct AtomRecord {
    std::string x;
    std::string y;
    std::string z;
    std::string residue;
    std::string atom;
    std::string resnum;
    std::string chain;
    std::string element;
    std::string record;
};

struct ResidueInfo {
    std::string name;
    std::string chain;
    std::string resnum;
    int atom_count = 0;
    bool polymer_flag = false;
    bool hetatm_only = true;
    std::unordered_set<std::string> elements;
    bool is_water = false;
    bool is_nucleic = false;
    bool is_amino = false;
    bool is_ion = false;
    bool is_ligand = false;
};

const std::unordered_set<std::string> kWaterResidues = {
    "HOH", "H2O", "DOD", "WAT", "SOL", "TIP", "TIP3", "TIP3P", "TIP4", "TIP4P", "TIP5P", "SPC", "OH2"};
const std::unordered_set<std::string> kAminoResidues = {
    "ALA", "ARG", "ASN", "ASP", "ASX", "CYS", "GLN", "GLU", "GLX", "GLY", "HIS", "HID", "HIE", "HIP",
    "HISN", "HISL", "ILE", "LEU", "LYS", "MET", "MSE", "PHE", "PRO", "SER", "THR", "TRP", "TYR",
    "VAL", "SEC", "PYL", "ASH", "GLH"};
const std::unordered_set<std::string> kNucleicResidues = {
    "A",   "C",   "G",   "U",   "I",   "T",   "DA",  "DG",  "DC",  "DT",  "DI",  "ADE", "GUA",
    "CYT", "URI", "THY", "PSU", "OMC", "OMU", "OMG", "5IU", "H2U", "M2G", "7MG", "1MA", "1MG", "2MG"};
const std::unordered_set<std::string> kIonResidues = {
    "NA",  "K",   "MG",  "MN",  "FE",  "ZN",  "CU",  "CA",  "CL",  "BR",  "I",   "LI",  "CO",
    "NI",  "HG",  "CD",  "SR",  "CS",  "BA",  "YB",  "MO",  "RU",  "OS",  "IR",  "AU",  "AG",
    "PT",  "TI",  "AL",  "GA",  "V",   "W",   "ZN2", "FE2"};
const std::unordered_set<std::string> kIonElements = {
    "NA", "K",  "MG", "MN", "FE", "ZN", "CU", "CA", "CL", "BR", "I",  "LI", "CO", "NI",
    "HG", "CD", "SR", "CS", "BA", "YB", "MO", "RU", "OS", "IR", "AU", "AG", "PT", "TI",
    "AL", "GA", "V",  "W"};

std::string normalize_atom_name(std::string raw) {
    std::string padded = raw;
    if (padded.size() < 2) {
        padded.resize(2, ' ');
    } else {
        padded.resize(2);
    }

    const auto to_upper_local = [](char c) {
        return static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    };

    const char c0 = padded[0];
    const char c1 = padded[1];
    const bool first_blank_digit = (c0 == ' ' || std::isdigit(static_cast<unsigned char>(c0)) != 0);
    const bool second_h_like = (to_upper_local(c1) == 'H' || to_upper_local(c1) == 'D');
    if (first_blank_digit && second_h_like) {
        return "H";
    }
    const bool first_h = (to_upper_local(c0) == 'H');
    const bool second_g = (to_upper_local(c1) == 'G');
    if (first_h && !second_g) {
        return "H";
    }

    auto trimmed = trim(raw);
    trimmed.erase(std::remove(trimmed.begin(), trimmed.end(), ' '), trimmed.end());
    return trimmed;
}

std::string make_residue_key(const AtomRecord& atom) {
    std::ostringstream oss;
    oss << to_upper(atom.chain) << '|' << atom.resnum << '|' << to_upper(atom.residue);
    return oss.str();
}

bool looks_like_nucleic(const std::string& name) {
    if (name.size() == 1 && std::string("ACGUIT").find(name[0]) != std::string::npos) {
        return true;
    }
    if (name.size() == 2 && name[0] == 'D' && std::string("ACGUIT").find(name[1]) != std::string::npos) {
        return true;
    }
    return false;
}

bool is_water(const std::string& name) {
    const std::string upper = to_upper(name);
    if (kWaterResidues.count(upper) > 0) {
        return true;
    }
    if (upper.rfind("HOH", 0) == 0 || upper.rfind("TIP", 0) == 0) {
        return true;
    }
    return false;
}

bool is_amino(const std::string& name) {
    return kAminoResidues.count(to_upper(name)) > 0;
}

bool is_nucleic(const std::string& name) {
    const std::string upper = to_upper(name);
    return kNucleicResidues.count(upper) > 0 || looks_like_nucleic(upper);
}

bool is_ion(const ResidueInfo& info) {
    const std::string upper = to_upper(info.name);
    if (kIonResidues.count(upper) > 0) {
        return true;
    }
    if (info.atom_count <= 1) {
        for (const auto& element : info.elements) {
            if (kIonElements.count(to_upper(element)) > 0) {
                return true;
            }
        }
        if (kIonElements.count(upper) > 0) {
            return true;
        }
    }
    return false;
}

std::unordered_map<std::string, ResidueInfo> classify_residues(const std::vector<AtomRecord>& atoms) {
    std::unordered_map<std::string, ResidueInfo> residues;
    for (const auto& atom : atoms) {
        const std::string key = make_residue_key(atom);
        auto& info = residues[key];
        if (info.atom_count == 0) {
            info.name = atom.residue;
            info.chain = atom.chain;
            info.resnum = atom.resnum;
        }
        ++info.atom_count;
        if (!atom.element.empty()) {
            info.elements.insert(to_upper(atom.element));
        }
        if (to_upper(atom.record) == "ATOM") {
            info.polymer_flag = true;
        }
        if (to_upper(atom.record) != "HETATM") {
            info.hetatm_only = false;
        }
    }
    for (auto& entry : residues) {
        auto& info = entry.second;
        if (is_amino(info.name) || is_nucleic(info.name)) {
            info.polymer_flag = true;
        }
        info.is_water = is_water(info.name);
        info.is_amino = is_amino(info.name);
        info.is_nucleic = is_nucleic(info.name);
        info.is_ion = is_ion(info);
        info.is_ligand = !info.polymer_flag && !info.is_water && !info.is_ion;
    }
    return residues;
}

bool should_filter(const ResidueInfo& info, const Filters& filters) {
    if (filters.exclude_water && info.is_water) {
        return true;
    }
    if (filters.exclude_ions && info.is_ion) {
        return true;
    }
    if (filters.exclude_ligands && info.is_ligand) {
        return true;
    }
    if (filters.exclude_hetatm && info.hetatm_only) {
        return true;
    }
    if (filters.exclude_nucleic_acids && info.is_nucleic) {
        return true;
    }
    if (filters.exclude_amino_acids && info.is_amino) {
        return true;
    }
    return false;
}

double parse_float(const std::string& text) {
    try {
        return std::stod(text);
    } catch (...) {
        return 0.0;
    }
}

void emit_atoms(const std::vector<AtomRecord>& atoms,
                const std::string& label,
                const AtomTypeLibrary& library,
                const ConversionOptions& options,
                std::ostream* output,
                std::vector<XyzrAtom>* sink) {
    if (atoms.empty()) {
        return;
    }
    const auto residues = classify_residues(atoms);
    for (const auto& atom : atoms) {
        const auto key = make_residue_key(atom);
        const auto residue_it = residues.find(key);
        const ResidueInfo* info = residue_it != residues.end() ? &residue_it->second : nullptr;
        if (info && should_filter(*info, options.filters)) {
            continue;
        }
        const auto result = library.radius_for(atom.residue, atom.atom, options.use_united);
        if (!result.first) {
            std::cerr << "pdb_to_xyzr: error, file " << label << " residue " << atom.resnum
                      << " atom pattern " << atom.residue << ' ' << atom.atom
                      << " was not found in embedded atmtypenumbers" << std::endl;
        }
        if (output) {
            *output << atom.x << ' ' << atom.y << ' ' << atom.z << ' ' << result.second << '\n';
        }
        if (sink) {
            sink->push_back(
                XyzrAtom{parse_float(atom.x), parse_float(atom.y), parse_float(atom.z),
                         parse_float(result.second)});
        }
    }
}

bool convert_stream_impl(const AtomTypeLibrary& library,
                         std::istream& input,
                         const std::string& label,
                         const ConversionOptions& options,
                         std::ostream* output,
                         std::vector<XyzrAtom>* sink) {
    std::string line;
    std::vector<AtomRecord> atoms;
    while (std::getline(input, line)) {
        if (line.size() < 6) {
            continue;
        }
        auto record = trim(line.substr(0, 6));
        std::transform(record.begin(), record.end(), record.begin(), [](char c) {
            return static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        });
        if (record != "ATOM" && record != "HETATM") {
            continue;
        }
        const std::string raw_x = get_field(line, 30, 8);
        const std::string raw_y = get_field(line, 38, 8);
        const std::string raw_z = get_field(line, 46, 8);
        if (trim(raw_x).empty() || trim(raw_y).empty() || trim(raw_z).empty()) {
            continue;
        }
        AtomRecord atom;
        atom.record = record;
        atom.x = raw_x;
        atom.y = raw_y;
        atom.z = raw_z;
        atom.residue = trim(get_field(line, 17, 3));
        atom.atom = normalize_atom_name(get_field(line, 12, 4));
        atom.resnum = trim(get_field(line, 22, 4));
        atom.chain = trim(get_field(line, 21, 1));
        atom.element = trim(get_field(line, 76, 2));
        if (atom.element.empty() && !atom.atom.empty()) {
            atom.element = to_upper(atom.atom.substr(0, 1));
        }
        atoms.push_back(std::move(atom));
    }
    emit_atoms(atoms, label, library, options, output, sink);
    return true;
}

#if VOSS_HAVE_GEMMI
bool convert_with_gemmi_impl(const AtomTypeLibrary& library,
                             const std::string& path,
                             const ConversionOptions& options,
                             std::ostream* output,
                             std::vector<XyzrAtom>* sink) {
    try {
        gemmi::Structure structure = gemmi::read_structure(path);
        std::vector<AtomRecord> atoms;
        for (const auto& model : structure.models) {
            for (const auto& chain : model.chains) {
                for (const auto& residue : chain.residues) {
                    const std::string residue_name = trim(residue.name);
                    const std::string resnum = trim(residue.seqid.str());
                    for (const auto& atom : residue.atoms) {
                        AtomRecord record;
                        record.x = format_coordinate(atom.pos.x);
                        record.y = format_coordinate(atom.pos.y);
                        record.z = format_coordinate(atom.pos.z);
                        record.residue = residue_name;
                        record.atom = normalize_atom_name(atom.name);
                        record.resnum = resnum;
                        record.chain = chain.name;
                        record.element = to_upper(atom.element.name());
                        record.record = atom.het_flag == 'H' ? "HETATM" : "ATOM";
                        atoms.push_back(std::move(record));
                    }
                }
            }
        }
        emit_atoms(atoms, path, library, options, output, sink);
        return true;
    } catch (const std::exception& ex) {
        std::cerr << "pdb_to_xyzr: Gemmi failed to read '" << path << "': " << ex.what()
                  << std::endl;
        return false;
    }
}
#endif

bool has_extension(std::string path, std::initializer_list<const char*> exts) {
    std::transform(path.begin(), path.end(), path.begin(), [](char c) {
        return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    });
    if (path.size() > 3 && path.compare(path.size() - 3, 3, ".gz") == 0) {
        path.erase(path.size() - 3);
    }
    const auto dot = path.rfind('.');
    if (dot == std::string::npos) {
        return false;
    }
    const std::string ext = path.substr(dot);
    for (const char* candidate : exts) {
        if (ext == candidate) {
            return true;
        }
    }
    return false;
}

}  // namespace

struct PdbToXyzrConverter::Impl {
    AtomTypeLibrary library;
};

PdbToXyzrConverter::PdbToXyzrConverter() : impl_(new Impl) {}
PdbToXyzrConverter::~PdbToXyzrConverter() = default;

bool PdbToXyzrConverter::ConvertStream(std::istream& input,
                                       const std::string& label,
                                       const ConversionOptions& options,
                                       std::ostream& output) const {
    return convert_stream_impl(impl_->library, input, label, options, &output, nullptr);
}

bool PdbToXyzrConverter::ConvertFile(const std::string& path,
                                     const ConversionOptions& options,
                                     std::ostream& output) const {
    std::ifstream stream(path);
    if (!stream) {
        return false;
    }
    return ConvertStream(stream, path, options, output);
}

bool PdbToXyzrConverter::ConvertStreamToAtoms(std::istream& input,
                                              const std::string& label,
                                              const ConversionOptions& options,
                                              std::vector<XyzrAtom>& atoms) const {
    atoms.clear();
    return convert_stream_impl(impl_->library, input, label, options, nullptr, &atoms);
}

bool PdbToXyzrConverter::ConvertFileToAtoms(const std::string& path,
                                            const ConversionOptions& options,
                                            std::vector<XyzrAtom>& atoms) const {
    std::ifstream stream(path);
    if (!stream) {
        return false;
    }
    return ConvertStreamToAtoms(stream, path, options, atoms);
}

#if VOSS_HAVE_GEMMI
bool PdbToXyzrConverter::ConvertWithGemmi(const std::string& path,
                                          const ConversionOptions& options,
                                          std::ostream& output) const {
    return convert_with_gemmi_impl(impl_->library, path, options, &output, nullptr);
}

bool PdbToXyzrConverter::ConvertWithGemmiToAtoms(const std::string& path,
                                                 const ConversionOptions& options,
                                                 std::vector<XyzrAtom>& atoms) const {
    atoms.clear();
    return convert_with_gemmi_impl(impl_->library, path, options, nullptr, &atoms);
}
#endif

bool IsMmcifFile(const std::string& path) {
    return has_extension(path, {".cif", ".mmcif"});
}

bool IsPdbmlFile(const std::string& path) {
    return has_extension(path, {".xml", ".pdbml", ".pdbxml"});
}

bool IsXyzrFile(const std::string& path) {
    return has_extension(path, {".xyzr", ".xyz"});
}

bool LoadStructureAsXyzr(const std::string& path,
                         const ConversionOptions& options,
                         std::vector<XyzrAtom>& atoms) {
    atoms.clear();
    PdbToXyzrConverter converter;
    if (IsXyzrFile(path)) {
        std::ifstream input(path);
        if (!input) {
            return false;
        }
        std::string line;
        while (std::getline(input, line)) {
            if (line.empty()) {
                continue;
            }
            std::istringstream iss(line);
            double x, y, z, r;
            if (!(iss >> x >> y >> z >> r)) {
                continue;
            }
            atoms.push_back(XyzrAtom{x, y, z, r});
        }
        return true;
    }

#if VOSS_HAVE_GEMMI
    if (converter.ConvertWithGemmiToAtoms(path, options, atoms)) {
        return true;
    }
    if (IsMmcifFile(path) || IsPdbmlFile(path)) {
        return false;
    }
#else
    if (IsMmcifFile(path) || IsPdbmlFile(path)) {
        std::cerr << "pdb_to_xyzr: this build lacks Gemmi; unable to read '" << path
                  << "'. Please install Gemmi headers and recompile." << std::endl;
        return false;
    }
#endif
    return converter.ConvertFileToAtoms(path, options, atoms);
}

}  // namespace vossvolvox::pdbio
