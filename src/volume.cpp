#include <iostream>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include "argument_helper.hpp"
#include "pdb_io.hpp"
#include "utils.hpp"
#include "vossvolvox_cli_common.hpp"
#include "xyzr_cli_helpers.hpp"

// Globals
extern float GRID;

// Function prototypes
void processGrid(double probe,
                 const vossvolvox::OutputSettings& outputs,
                 const std::string& inputFile,
                 const XYZRBuffer& xyzr_buffer,
                 int numatoms);

int main(int argc, char* argv[]) {
  std::cerr << "\n";
  vossvolvox::set_command_line(argc, argv);

  // Initialize variables
  std::string inputFile;
  vossvolvox::OutputSettings outputs;
  vossvolvox::DebugSettings debug;
  double probe = 10.0;
  float grid = GRID;  // Use global GRID value initially
  vossvolvox::FilterSettings filters;

  vossvolvox::ArgumentParser parser(
      argv[0],
      "Calculate molecular volume and surface area for a given probe radius.");
  vossvolvox::add_input_option(parser, inputFile);
  parser.add_option("-p", "--probe", probe, 10.0, "Probe radius in Angstroms (default 10.0).", "<probe radius>");
  parser.add_option("-g", "--grid", grid, GRID, "Grid spacing in Angstroms.", "<grid spacing>");
  vossvolvox::add_output_options(parser, outputs);
  vossvolvox::add_filter_options(parser, filters);
  vossvolvox::add_debug_option(parser, debug);
  parser.add_example(std::string(argv[0]) + " -i sample.xyzr -p 1.5 -g 0.5 -o surface.pdb");

  const auto parse_result = parser.parse(argc, argv);
  if (parse_result == vossvolvox::ArgumentParser::ParseResult::HelpRequested) {
    return 0;
  }
  if (parse_result == vossvolvox::ArgumentParser::ParseResult::Error) {
    return 1;
  }
  if (!vossvolvox::ensure_input_present(inputFile, parser)) {
    return 1;
  }

  vossvolvox::enable_debug(debug);
  vossvolvox::debug_report_cli(inputFile, &outputs);

  if (!vossvolvox::quiet_mode()) {
    printCompileInfo(argv[0]);
    printCitation();
  }

  GRID = grid;

  // Debugging information
  std::cerr << "Initializing Calculation:\n"
            << "Probe Radius:       " << probe << " A\n"
            << "Grid Spacing:       " << GRID << " A\n"
            << "Input File:         " << inputFile << "\n";

  // Load atoms into memory and compute bounds
  const auto convert_options = vossvolvox::make_conversion_options(filters);
  XYZRBuffer xyzr_buffer;
  if (!vossvolvox::load_xyzr_or_exit(inputFile, convert_options, xyzr_buffer)) {
    return 1;
  }
  const std::vector<const XYZRBuffer*> buffers = {&xyzr_buffer};
  const auto grid_result = vossvolvox::prepare_grid_from_xyzr(
      buffers,
      grid,
      static_cast<float>(probe),
      inputFile,
      false);
  const int numatoms = grid_result.total_atoms;

  // Process the grid for volume and surface calculations
  processGrid(probe, outputs, inputFile, xyzr_buffer, numatoms);

  // Program completed successfully
  std::cerr << "\nProgram Completed Successfully\n\n";
  return 0;
}

// Process the grid for volume and surface calculations
void processGrid(double probe,
                 const vossvolvox::OutputSettings& outputs,
                 const std::string& inputFile,
                 const XYZRBuffer& xyzr_buffer,
                 int numatoms) {
  // Allocate memory for the excluded grid
  auto EXCgrid = make_zeroed_grid();

  // Populate the grid based on the probe radius
  int voxels = get_ExcludeGrid_fromArray(numatoms, probe, xyzr_buffer, EXCgrid.get());

  long double surf = surface_area(EXCgrid.get());

  std::cerr << "\nSummary of Results:\n"
            << "Probe Radius:       " << probe << " A\n";
  report_grid_metrics(std::cerr, voxels, surf);
  std::cerr << "Number of Atoms:    " << numatoms << "\n"
            << "Input File:         " << inputFile << "\n"
            << "\n";

  write_output_files(EXCgrid.get(), outputs);

  // Output results to `std::cout` (batch processing)
  std::cout << probe << "\t" << GRID << "\t";
  printVolCout(voxels);
  std::cout << "\t" << surf << "\t" << numatoms << "\t" << inputFile;
  std::cout << "\tprobe,grid,volume,surf_area,num_atoms,file\n";
}
