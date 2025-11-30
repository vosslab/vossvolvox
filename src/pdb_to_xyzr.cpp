#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>

#include "pdb_io.h"

namespace {

void print_compile_info(const char* program) {
    static bool printed = false;
    if (printed) {
        return;
    }
    printed = true;
    std::cerr << "Program: " << program << "\n"
              << "Compiled on: " << __DATE__ << " at " << __TIME__ << "\n"
              << "C++ Standard: " << __cplusplus << "\n"
              << "Source file: " << __FILE__ << "\n"
              << std::endl;
}

void print_citation() {
    static bool printed = false;
    if (printed) {
        return;
    }
    printed = true;
    std::cerr << "Citation: Neil R Voss, et al. J Mol Biol. v360 (4): 2006, pp. 893-906.\n"
              << "DOI: http://dx.doi.org/10.1016/j.jmb.2006.05.023\n"
              << "E-mail: M Gerstein <mark.gerstein@yale.edu> or NR Voss <vossman77@yahoo.com>\n"
              << std::endl;
}

struct Options {
    bool use_united = true;
    bool quiet = false;
    std::string input = "-";
    vossvolvox::pdbio::Filters filters;
};

void print_help(const char* program) {
    std::cout << "Usage: " << program
              << " [options] [input]\n\n"
              << "Options:\n"
              << "  -h, --help               show this message and exit\n"
              << "  -H, --hydrogens          use explicit hydrogen radii\n"
              << "  -q, --quiet              suppress program banner/citation output\n"
              << "      --exclude-ions       drop residues classified as ions\n"
              << "      --exclude-ligands    drop non-polymer ligands\n"
              << "      --exclude-hetatm     drop residues composed of HETATM records only\n"
              << "      --exclude-water      drop water molecules\n"
              << "      --exclude-nucleic-acids drop nucleic-acid residues\n"
              << "      --exclude-amino-acids  drop amino-acid residues\n";
}

Options parse_options(int argc, char** argv) {
    Options options;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            print_help(argv[0]);
            std::exit(0);
        }
        if (arg == "-H" || arg == "--hydrogens") {
            options.use_united = false;
            continue;
        }
        if (arg == "-q" || arg == "--quiet") {
            options.quiet = true;
            continue;
        }
        if (arg == "--exclude-ions") {
            options.filters.exclude_ions = true;
            continue;
        }
        if (arg == "--exclude-ligands") {
            options.filters.exclude_ligands = true;
            continue;
        }
        if (arg == "--exclude-hetatm") {
            options.filters.exclude_hetatm = true;
            continue;
        }
        if (arg == "--exclude-water") {
            options.filters.exclude_water = true;
            continue;
        }
        if (arg == "--exclude-nucleic-acids") {
            options.filters.exclude_nucleic_acids = true;
            continue;
        }
        if (arg == "--exclude-amino-acids") {
            options.filters.exclude_amino_acids = true;
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

}  // namespace

int main(int argc, char** argv) {
    std::ios::sync_with_stdio(false);
    const auto options = parse_options(argc, argv);
    if (!options.quiet) {
        print_compile_info(argv[0]);
        print_citation();
    }

    vossvolvox::pdbio::PdbToXyzrConverter converter;
    vossvolvox::pdbio::ConversionOptions convert_options;
    convert_options.use_united = options.use_united;
    convert_options.filters = options.filters;

    const bool use_stdin = options.input == "-";
#if VOSS_HAVE_GEMMI
    if (!use_stdin) {
        if (converter.ConvertWithGemmi(options.input, convert_options, std::cout)) {
            return 0;
        }
        if (vossvolvox::pdbio::IsMmcifFile(options.input) ||
            vossvolvox::pdbio::IsPdbmlFile(options.input)) {
            return 2;
        }
    }
#else
    if (!use_stdin &&
        (vossvolvox::pdbio::IsMmcifFile(options.input) ||
         vossvolvox::pdbio::IsPdbmlFile(options.input))) {
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
    converter.ConvertStream(*input, label, convert_options, std::cout);
    return 0;
}

