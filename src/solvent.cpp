#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include "argument_helper.hpp"
#include "pdb_io.hpp"
#include "utils.hpp"
#include "vossvolvox_cli_common.hpp"
#include "xyzr_cli_helpers.hpp"

// Globals
extern float GRID, GRIDVOL;
extern unsigned int NUMBINS;

int main(int argc, char *argv[]) {
  std::cerr << std::endl;
  vossvolvox::set_command_line(argc, argv);

  std::string input_path;
  vossvolvox::OutputSettings outputs;
  vossvolvox::DebugSettings debug;
  double BIGPROBE = 9.0;
  double SMPROBE = 1.5;
  double TRIMPROBE = 1.5;
  float grid = GRID;
  vossvolvox::FilterSettings filters;

  vossvolvox::ArgumentParser parser(
      argv[0],
      "Extract all solvent from a structure for the given probe radii.");
  vossvolvox::add_input_option(parser, input_path);
  parser.add_option("-s",
                    "--small-probe",
                    SMPROBE,
                    1.5,
                    "Small probe radius in Angstroms.",
                    "<small probe>");
  parser.add_option("-b",
                    "--big-probe",
                    BIGPROBE,
                    9.0,
                    "Big probe radius in Angstroms.",
                    "<big probe>");
  parser.add_option("-t",
                    "--trim-probe",
                    TRIMPROBE,
                    1.5,
                    "Trim radius applied to the exterior solvent shell.",
                    "<trim probe>");
  parser.add_option("-g",
                    "--grid",
                    grid,
                    GRID,
                    "Grid spacing in Angstroms.",
                    "<grid spacing>");
  vossvolvox::add_output_options(parser, outputs);
  vossvolvox::add_filter_options(parser, filters);
  vossvolvox::add_debug_option(parser, debug);
  parser.add_example("./Solvent.exe -i sample.xyzr -s 1.5 -b 9.0 -t 4 -g 0.5 -o solvent.pdb");

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
      static_cast<float>(BIGPROBE),
      input_path,
      false);
  const int numatoms = grid_result.total_atoms;

//HEADER CHECK
  if(SMPROBE > BIGPROBE) { cerr << "ERROR: SMPROBE > BIGPROBE" << endl; return 1; }
  cerr << "Small Probe Radius: " << SMPROBE << endl;
  cerr << " Big  Probe Radius: " << BIGPROBE << endl;
  cerr << "Trim  Probe Radius: " << TRIMPROBE << endl;
  cerr << "Grid Spacing: " << GRID << endl;
  cerr << "Input file:   " << input_path << endl;
  cerr << "Resolution:   " << int(1000.0/float(GRIDVOL))/1000.0 << " voxels per A^3" << endl;
  cerr << "Resolution:   " << int(11494.0/float(GRIDVOL))/1000.0 << " voxels per water molecule" << endl;


// ****************************************************
// STARTING LARGE PROBE
// ****************************************************
  auto biggrid = make_zeroed_grid();
  int bigvox;
  if (BIGPROBE > 0.0) {
    bigvox = get_ExcludeGrid_fromArray(numatoms, BIGPROBE, xyzr_buffer, biggrid.get());
  } else {
    cerr << "BIGPROBE <= 0" << endl;
    return 1;
  }


// ****************************************************
// TRIM LARGE PROBE SURFACE
// ****************************************************
  auto trimgrid = make_zeroed_grid();
  copyGrid(biggrid.get(), trimgrid.get());
  if(TRIMPROBE > 0) {
    trun_ExcludeGrid(TRIMPROBE, biggrid.get(), trimgrid.get());
  }
  biggrid.reset();

  //cout << "bg_prb\tsm_prb\tgrid\texcvol\tsurf\taccvol\tfile" << endl;

// ****************************************************
// STARTING SMALL PROBE
// ****************************************************
    auto smgrid = make_zeroed_grid();
    int smvox;
    smvox = fill_AccessGrid_fromArray(numatoms, SMPROBE, xyzr_buffer, smgrid.get());

// ****************************************************
// GETTING ACCESSIBLE CHANNELS
// ****************************************************
    auto solventACC = make_zeroed_grid();
    copyGrid(trimgrid.get(), solventACC.get()); //copy trimgrid into solventACC
    subt_Grids(solventACC.get(), smgrid.get()); //modify solventACC
    smgrid.reset();

// ***************************************************
// GETTING CONTACT CHANNEL
// ***************************************************
    auto solventEXC = make_zeroed_grid();
    int solventACCvol = copyGrid(solventACC.get(), solventEXC.get());
    cerr << "Accessible Channel Volume  ";
    printVol(solventACCvol);
    grow_ExcludeGrid(SMPROBE, solventACC.get(), solventEXC.get());
    solventACC.reset();

//limit growth to inside trimgrid
    intersect_Grids(solventEXC.get(), trimgrid.get()); //modifies solventEXC
    trimgrid.reset();

// ***************************************************
// OUTPUT RESULTS
// ***************************************************
    cout << BIGPROBE << "\t" << SMPROBE << "\t" << GRID << "\t" << flush;
    int solventEXCvol = countGrid(solventEXC.get());
    printVolCout(solventEXCvol);
    long double surf = surface_area(solventEXC.get());
    cout << "\t" << surf << "\t" << flush;
    //printVolCout(solventACCvol);
    cout << input_path << endl;
    write_output_files(solventEXC.get(), outputs);

  cerr << endl << "Program Completed Sucessfully" << endl << endl;
  return 0;
};
