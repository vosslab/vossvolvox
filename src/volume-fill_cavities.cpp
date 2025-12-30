#include <iostream>
#include <string>
#include <vector>

#include "argument_helper.hpp"
#include "pdb_io.hpp"
#include "utils.hpp"                    // for endl, cerr, countGrid, gridpt
#include "vossvolvox_cli_common.hpp"
#include "xyzr_cli_helpers.hpp"

// ****************************************************
// CALCULATE EXCLUDED VOLUME, BUT FILL ANY CAVITIES
// designed for use with 3d printer
// ****************************************************

// Globals
extern float GRID, GRIDVOL;

int getCavitiesBothMeth(const float probe,
                        gridpt shellACC[],
                        gridpt shellEXC[],
                        const int natoms,
                        const XYZRBuffer& xyzr_buffer,
                        char ezdfile[],
                        char pdbfile[],
                        char mrcfile[]);

int main(int argc, char *argv[]) {
  std::cerr << std::endl;
  vossvolvox::set_command_line(argc, argv);

  std::string input_path;
  vossvolvox::OutputSettings outputs;
  vossvolvox::DebugSettings debug;
  double PROBE = 1.5;
  float grid = GRID;
  vossvolvox::FilterSettings filters;

  vossvolvox::ArgumentParser parser(
      argv[0],
      "Calculate excluded volume while filling cavities.");
  vossvolvox::add_input_option(parser, input_path);
  parser.add_option("-p",
                    "--probe",
                    PROBE,
                    1.5,
                    "Probe radius in Angstroms.",
                    "<probe>");
  parser.add_option("-g",
                    "--grid",
                    grid,
                    GRID,
                    "Grid spacing in Angstroms.",
                    "<grid>");
  vossvolvox::add_output_options(parser, outputs);
  vossvolvox::add_filter_options(parser, filters);
  vossvolvox::add_debug_option(parser, debug);
  parser.add_example("./VolumeNoCav.exe -i sample.xyzr -p 1.5 -g 0.8 -o filled.pdb");

  const auto parse_result = parser.parse(argc, argv);
  if (parse_result == vossvolvox::ArgumentParser::ParseResult::HelpRequested) {
    return 0;
  }
  if (parse_result == vossvolvox::ArgumentParser::ParseResult::Error) {
    return 1;
  }
  if (!vossvolvox::ensure_input_present(input_path, parser)) {
    return 1;
  }

  vossvolvox::enable_debug(debug);
  vossvolvox::debug_report_cli(input_path, &outputs);

  GRID = grid;

  if (!vossvolvox::quiet_mode()) {
    printCompileInfo(argv[0]);
    printCitation();
  }

  const auto convert_options = vossvolvox::make_conversion_options(filters);
  XYZRBuffer xyzr_buffer;
  if (!vossvolvox::load_xyzr_or_exit(input_path, convert_options, xyzr_buffer)) {
    return 1;
  }

  const std::vector<const XYZRBuffer*> buffers = {&xyzr_buffer};
  const auto grid_result = vossvolvox::prepare_grid_from_xyzr(
      buffers,
      grid,
      static_cast<float>(PROBE * 2),
      input_path,
      false);
  const int numatoms = grid_result.total_atoms;

// ****************************************************
// INITIALIZATION
// ****************************************************
//HEADER CHECK
  cerr << "Grid Spacing: " << GRID << endl;
  cerr << "Input file:   " << input_path << endl;
// ****************************************************
// STARTING FILE READ-IN
// ****************************************************

  auto shellACC = make_zeroed_grid();
  fill_AccessGrid_fromArray(numatoms, PROBE, xyzr_buffer, shellACC.get());
  int voxels1 = countGrid(shellACC.get());
  fill_cavities(shellACC.get());
  int voxels2 = countGrid(shellACC.get());
  cerr << "Fill Cavities: " << voxels2 - voxels1 << " voxels filled" << endl;

  auto EXCgrid = make_zeroed_grid();
  trun_ExcludeGrid(PROBE, shellACC.get(), EXCgrid.get());

  shellACC.reset();
  int voxels = countGrid(EXCgrid.get());

  long double surf;
  surf = surface_area(EXCgrid.get());

  report_grid_metrics(std::cerr, voxels, surf);

  write_output_files(EXCgrid.get(), outputs);

  cout << PROBE << "\t" << GRID << "\t" << flush;
  printVolCout(voxels);
  cout << "\t" << surf << "\t#" << input_path << endl;

  cerr << endl << "Program Completed Sucessfully" << endl << endl;
  return 0;



};
