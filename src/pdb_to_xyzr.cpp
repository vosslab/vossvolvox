#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <initializer_list>
#include <regex>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "atmtypenumbers_data.h"

#if __has_include(<gemmi/structure.hpp>)
#define VOSS_HAVE_GEMMI 1
#include <gemmi/structure.hpp>
#include <gemmi/mmcif.hpp>
#include <gemmi/pdb.hpp>
#endif

namespace {

struct RadiusEntry {
    std::string explicit_text{"0.01"};
    std::string united_text{"0.01"};
};

struct PatternEntry {
    std::regex residue;
    std::regex atom;
    std::string key;
};

std::string trim(std::string_view view) {
    const auto first = view.find_first_not_of(' ');
    if (first == std::string_view::npos) {
        return {};
    }
    const auto last = view.find_last_not_of(' ');
    return std::string(view.substr(first, last - first + 1));
}

std::string normalize_atom_name(std::string raw) {
    std::string padded = raw;
    if (padded.size() < 2) {
        padded.resize(2, ' ');
    } else {
        padded.resize(2);
    }

    const auto to_upper = [](char c) {
        return static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    };

    const char c0 = padded[0];
    const char c1 = padded[1];
    const bool first_blank_digit = (c0 == ' ' || std::isdigit(static_cast<unsigned char>(c0)) != 0);
    const bool second_h_like = (to_upper(c1) == 'H' || to_upper(c1) == 'D');
    if (first_blank_digit && second_h_like) {
        return "H";
    }
    const bool first_h = (to_upper(c0) == 'H');
    const bool second_g = (to_upper(c1) == 'G');
    if (first_h && !second_g) {
        return "H";
    }

    auto trimmed = trim(raw);
    trimmed.erase(std::remove(trimmed.begin(), trimmed.end(), ' '), trimmed.end());
    return trimmed;
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
                    std::cerr << "pdb_to_xyzr.cpp: warning: invalid radius '" << explicit_text
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
                std::cerr << "pdb_to_xyzr.cpp: warning: entry " << tokens[0] << ' '
                          << tokens[1] << ' ' << key << " has no corresponding radius value\n";
            }

            try {
                patterns_.push_back(PatternEntry{std::regex(residue_pattern, extended),
                                                 std::regex(atom_pattern, extended),
                                                 key});
            } catch (const std::regex_error& err) {
                std::cerr << "pdb_to_xyzr.cpp: warning: invalid regex on line '" << line
                          << "': " << err.what() << "\n";
            }
        }
    }

    std::vector<PatternEntry> patterns_;
    std::unordered_map<std::string, RadiusEntry> radii_;
};

struct Options {
    bool use_united = true;
    std::string input = "-";
};

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

bool is_mmcif_file(const std::string& path) {
    return has_extension(path, {".cif", ".mmcif"});
}

bool is_pdbml_file(const std::string& path) {
    return has_extension(path, {".xml", ".pdbml", ".pdbxml"});
}

void print_help(const char* program) {
    std::cout << "Usage: " << program << " [-h|--hydrogens] [--help] [input]" << std::endl;
}

Options parse_options(int argc, char** argv) {
    Options options;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--help") {
            print_help(argv[0]);
            std::exit(0);
        }
        if (arg == "-h" || arg == "--hydrogens") {
            options.use_united = false;
            continue;
        }
        if (!arg.empty() && arg[0] == '-') {
            std::cerr << "pdb_to_xyzr: unknown option '" << arg << "'\n";
            print_help(argv[0]);
            std::exit(2);
        }
        if (options.input != "-") {
            std::cerr << "pdb_to_xyzr: multiple input files provided" << std::endl;
            std::exit(2);
        }
        options.input = arg;
    }
    return options;
}

bool process_stream(std::istream& input,
                    const std::string& label,
                    const AtomTypeLibrary& library,
                    bool use_united) {
    std::string line;
    int line_no = 0;
    while (std::getline(input, line)) {
        ++line_no;
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
        const std::string residue = trim(get_field(line, 17, 3));
        const std::string atom = normalize_atom_name(get_field(line, 12, 4));
        std::string resnum = trim(get_field(line, 22, 4));

        const auto result = library.radius_for(residue, atom, use_united);
        if (!result.first) {
            std::cerr << "pdb_to_xyzr: error, file " << label << " line " << line_no
                      << " residue " << resnum << " atom pattern " << residue << ' '
                      << atom << " was not found in embedded atmtypenumbers" << std::endl;
        }
        std::cout << raw_x << ' ' << raw_y << ' ' << raw_z << ' ' << result.second << '\n';
    }
    return true;
}

#ifdef VOSS_HAVE_GEMMI
bool process_with_gemmi(const std::string& path,
                        const AtomTypeLibrary& library,
                        bool use_united) {
    try {
        gemmi::Structure structure = gemmi::read_structure(path);
        for (const auto& model : structure.models) {
            for (const auto& chain : model.chains) {
                for (const auto& residue : chain.residues) {
                    const std::string residue_name = trim(residue.name);
                    const std::string resnum = trim(residue.seqid.str());
                    for (const auto& atom : residue.atoms) {
                        const std::string atom_name = atom.name;
                        const std::string normalized_atom = normalize_atom_name(atom_name);
                        const auto [matched, radius] =
                            library.radius_for(residue_name, normalized_atom, use_united);
                        if (!matched) {
                            std::cerr << "pdb_to_xyzr: error, file " << path << " residue " << resnum
                                      << " atom pattern " << residue_name << ' ' << normalized_atom
                                      << " was not found in embedded atmtypenumbers" << std::endl;
                        }
                        std::cout << format_coordinate(atom.pos.x) << ' '
                                  << format_coordinate(atom.pos.y) << ' '
                                  << format_coordinate(atom.pos.z) << ' ' << radius << '\n';
                    }
                }
            }
        }
        return true;
    } catch (const std::exception& ex) {
        std::cerr << "pdb_to_xyzr: Gemmi failed to read '" << path << "': " << ex.what()
                  << std::endl;
        return false;
    }
}
#endif

}  // namespace

int main(int argc, char** argv) {
    std::ios::sync_with_stdio(false);
    const auto options = parse_options(argc, argv);
    AtomTypeLibrary library;

    const bool use_stdin = options.input == "-";
#ifdef VOSS_HAVE_GEMMI
    if (!use_stdin) {
        if (process_with_gemmi(options.input, library, options.use_united)) {
            return 0;
        }
        if (is_mmcif_file(options.input) || is_pdbml_file(options.input)) {
            return 2;
        }
    }
#else
    if (!use_stdin && (is_mmcif_file(options.input) || is_pdbml_file(options.input))) {
        std::cerr << "pdb_to_xyzr: this build lacks Gemmi; unable to read '" << options.input
                  << "'. Please install Gemmi headers and recompile." << std::endl;
        return 2;
    }
#endif

    std::istream* input = &std::cin;
    std::ifstream file_stream;
    if (!use_stdin) {
        file_stream.open(options.input);
        if (!file_stream) {
            std::cerr << "pdb_to_xyzr: unable to open '" << options.input << "'" << std::endl;
            return 2;
        }
        input = &file_stream;
    }
    const std::string label = use_stdin ? "<stdin>" : options.input;
    process_stream(*input, label, library, options.use_united);
    return 0;
}
