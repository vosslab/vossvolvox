#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "argument_helper.hpp"
#include "pdb_io.hpp"
#include "vossvolvox_cli_common.hpp"

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

}  // namespace

int main(int argc, char** argv) {
    std::ios::sync_with_stdio(false);
    std::string input_file;
    vossvolvox::FilterSettings filters;
    vossvolvox::DebugSettings debug;

    vossvolvox::ArgumentParser parser(
        argv[0],
        "Convert structural inputs (PDB/mmCIF/PDBML/XYZR) to XYZR format.");
    vossvolvox::add_input_option(parser, input_file);
    vossvolvox::add_filter_options(parser, filters);
    vossvolvox::add_debug_option(parser, debug);
    parser.add_example(std::string(argv[0]) +
                       " -i 1A01.pdb --exclude-ions --exclude-water > 1a01-filtered.xyzr");
    parser.add_example(std::string(argv[0]) + " -i - --exclude-water < 1a01.pdb > 1a01.xyzr");

    std::string positional_input;
    std::vector<std::string> filtered_args;
    filtered_args.reserve(static_cast<size_t>(argc));
    filtered_args.push_back(argv[0]);

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--") {
            if (i + 1 < argc) {
                if (!positional_input.empty()) {
                    std::cerr << "pdb_to_xyzr: multiple input files provided\n";
                    return 2;
                }
                positional_input = argv[i + 1];
                if (i + 2 < argc) {
                    std::cerr << "pdb_to_xyzr: multiple input files provided\n";
                    return 2;
                }
            }
            break;
        }
        if (arg == "-i" || arg == "--input") {
            filtered_args.push_back(arg);
            if (i + 1 < argc) {
                filtered_args.push_back(argv[i + 1]);
                ++i;
            }
            continue;
        }
        if (!arg.empty() && arg[0] != '-') {
            if (!positional_input.empty()) {
                std::cerr << "pdb_to_xyzr: multiple input files provided\n";
                return 2;
            }
            positional_input = arg;
            continue;
        }
        filtered_args.push_back(arg);
    }

    std::vector<char*> filtered_argv;
    filtered_argv.reserve(filtered_args.size());
    for (auto& arg : filtered_args) {
        filtered_argv.push_back(arg.data());
    }

    const auto parse_result = parser.parse(static_cast<int>(filtered_argv.size()), filtered_argv.data());
    if (parse_result == vossvolvox::ArgumentParser::ParseResult::HelpRequested) {
        return 0;
    }
    if (parse_result == vossvolvox::ArgumentParser::ParseResult::Error) {
        return 1;
    }

    if (!input_file.empty() && !positional_input.empty()) {
        std::cerr << "pdb_to_xyzr: multiple input files provided\n";
        return 2;
    }
    if (input_file.empty() && !positional_input.empty()) {
        input_file = positional_input;
    }

    vossvolvox::enable_debug(debug);
    vossvolvox::debug_report_cli(input_file, nullptr);

    if (!vossvolvox::quiet_mode()) {
        print_compile_info(argv[0]);
        print_citation();
    }

    const auto convert_options = vossvolvox::make_conversion_options(filters);

    const bool use_stdin = input_file.empty() || input_file == "-";
    if (!use_stdin) {
        vossvolvox::pdbio::XyzrData data;
        if (!vossvolvox::pdbio::ReadFileToXyzr(input_file, convert_options, data)) {
            return 2;
        }
        vossvolvox::pdbio::WriteXyzrToStream(std::cout, data);
        return 0;
    }

    vossvolvox::pdbio::PdbToXyzrConverter converter;
    converter.ConvertStream(std::cin, "<stdin>", convert_options, std::cout);
    return 0;
}
