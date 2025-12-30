#include <iostream>
#include <string>
#include <vector>

#include "argument_helper.hpp"
#include "pdb_io.hpp"
#include "utils.hpp"
#include "vossvolvox_cli_common.hpp"
#include "xyzr_cli_helpers.hpp"


// Globals
extern float GRID;

int main(int argc, char *argv[]) {
  std::cerr << std::endl;
  vossvolvox::set_command_line(argc, argv);

  std::string input_path;
  vossvolvox::OutputSettings outputs;
  vossvolvox::DebugSettings debug;
  double BIGPROBE = 9.0;
  double SMPROBE = 1.5;
  double TRIMPROBE = 4.0;
  double x = 1000.0;
  double y = 1000.0;
  double z = 1000.0;
  float grid = GRID;
  vossvolvox::FilterSettings filters;

  vossvolvox::ArgumentParser parser(
      argv[0],
      "Extract a particular solvent channel from a structure.");
  vossvolvox::add_input_option(parser, input_path);
  parser.add_option("-b",
                    "--big-probe",
                    BIGPROBE,
                    9.0,
                    "Probe radius for large probe (default 9.0 A).",
                    "<big probe>");
  parser.add_option("-s",
                    "--small-probe",
                    SMPROBE,
                    1.5,
                    "Probe radius for small probe (default 1.5 A).",
                    "<small probe>");
  parser.add_option("-t",
                    "--trim-probe",
                    TRIMPROBE,
                    4.0,
                    "Probe radius for trimming exterior solvent (default 4.0 A).",
                    "<trim probe>");
  parser.add_option("-g",
                    "--grid",
                    grid,
                    GRID,
                    "Grid spacing (default auto).",
                    "<grid spacing>");
  parser.add_option("-x",
                    "--x-coord",
                    x,
                    1000.0,
                    "Seed X coordinate for channel selection.",
                    "<x>");
  parser.add_option("-y",
                    "--y-coord",
                    y,
                    1000.0,
                    "Seed Y coordinate for channel selection.",
                    "<y>");
  parser.add_option("-z",
                    "--z-coord",
                    z,
                    1000.0,
                    "Seed Z coordinate for channel selection.",
                    "<z>");
  outputs.use_small_mrc = true;
  vossvolvox::add_output_options(parser, outputs);
  vossvolvox::add_filter_options(parser, filters);
  vossvolvox::add_debug_option(parser, debug);
  parser.add_example(
      "./Channel.exe -i 3hdi.xyzr -b 9.0 -s 1.5 -t 4.0 -x -10 -y 5 -z 0 -o channel.pdb");

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
  cerr << "Probe Radius: " << BIGPROBE << endl;
  cerr << "Grid Spacing: " << GRID << endl;
  cerr << "Input file:   " << input_path << endl;

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
  trun_ExcludeGrid(TRIMPROBE, biggrid.get(), trimgrid.get());
  biggrid.reset();

  cout << "bg_prb\tsm_prb\tgrid\texcvol\tsurf\taccvol\tfile" << endl;

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
// SELECT PARTICULAR CHANNEL
// ***************************************************
    auto channelACC = make_zeroed_grid();

//main channel
    get_Connected(solventACC.get(), channelACC.get(), x, y, z);
    solventACC.reset();

// ***************************************************
// GETTING CONTACT CHANNEL
// ***************************************************
    auto channelEXC = make_zeroed_grid();
    int channelACCvol = copyGrid(channelACC.get(), channelEXC.get());
    cerr << "Accessible Channel Volume  ";
    printVol(channelACCvol);
    grow_ExcludeGrid(SMPROBE, channelACC.get(), channelEXC.get());
    channelACC.reset();

//limit growth to inside trimgrid
    intersect_Grids(channelEXC.get(), trimgrid.get()); //modifies channelEXC

// ***************************************************
// OUTPUT RESULTS
// ***************************************************
    cout << BIGPROBE << "\t" << SMPROBE << "\t" << GRID << "\t" << flush;
    int chanEXC_voxels = countGrid(channelEXC.get());
    printVolCout(chanEXC_voxels);
    long double surf = surface_area(channelEXC.get());
    cout << "\t" << surf << "\t" << flush;
    printVolCout(channelACCvol);
    cout << "\t#" << input_path << endl;
    report_grid_metrics(std::cerr, chanEXC_voxels, surf);
    write_output_files(channelEXC.get(), outputs);

    cerr << endl;

  cerr << endl << "Program Completed Sucessfully" << endl << endl;
  return 0;
};
