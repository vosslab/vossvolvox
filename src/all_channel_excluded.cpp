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
  double TRIMPROBE = 4.0;
  int MINSIZE = 20;
  double minvol = 0.0;
  double minperc = 0.0;
  float grid = GRID;
  vossvolvox::FilterSettings filters;

  vossvolvox::ArgumentParser parser(
      argv[0],
      "Extract all solvent channels from a structure above a cutoff.");
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
  parser.add_option("-v",
                    "--min-volume",
                    minvol,
                    0.0,
                    "Minimum channel volume in A^3.",
                    "<min volume>");
  parser.add_option("-p",
                    "--min-percent",
                    minperc,
                    0.0,
                    "Minimum percentage of volume for inclusion (e.g., 0.01 for 1%).",
                    "<fraction>");
  vossvolvox::add_output_options(parser, outputs);
  vossvolvox::add_filter_options(parser, filters);
  vossvolvox::add_debug_option(parser, debug);
  parser.add_example(
      "./AllChannelExc.exe -i 3hdi.xyzr -b 9.0 -s 1.5 -g 0.5 -t 4.0 -v 5000 -p 0.01");

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
  if (!outputs.pdbFile.empty() || !outputs.ezdFile.empty()) {
    std::cerr << "Warning: PDB/EZD outputs are not supported for this tool; ignoring.\n";
  }

  const auto convert_options = vossvolvox::make_conversion_options(filters);
  XYZRBuffer xyzr_buffer;
  if (!vossvolvox::load_xyzr_or_exit(input_path, convert_options, xyzr_buffer)) {
    return 1;
  }

  if (minvol > 0) {
    MINSIZE = int(minvol / GRIDVOL);
  } else if (minperc == 0) {
    minperc = 0.01;
  }

  const std::vector<const XYZRBuffer*> buffers = {&xyzr_buffer};
  const auto grid_result = vossvolvox::prepare_grid_from_xyzr(
      buffers,
      GRID,
      static_cast<float>(BIGPROBE),
      input_path,
      false);
  const int numatoms = grid_result.total_atoms;

  //HEADER CHECK
  cerr << "Probe Radius: " << BIGPROBE << endl;
  cerr << "Grid Spacing: " << GRID << endl;
  cerr << "Resolution:      " << int(1000.0/float(GRIDVOL))/1000.0 << " voxels per A^3" << endl;
  cerr << "Resolution:      " << int(11494.0/float(GRIDVOL))/1000.0 << " voxels per water molecule" << endl;
  cerr << "Input file:   " << input_path << endl;
  cerr << "Minimum size: " << MINSIZE << " voxels" << endl;

  // ****************************************************
  // STARTING LARGE PROBE
  // ****************************************************
  auto biggrid = make_zeroed_grid();
  int bigvox;
  if(BIGPROBE > 0.0) {
    bigvox = get_ExcludeGrid_fromArray(numatoms, BIGPROBE, xyzr_buffer, biggrid.get());
  } else {
    cerr << "BIGPROBE <= 0" << endl;
    return 1;
  }

  if (minperc > 0) {
    while (minperc > 1)
      minperc /= 100;
    MINSIZE = int(bigvox*minperc);
  }
  cerr << "Minimum size: " << MINSIZE << " voxels" << endl;

  // ****************************************************
  // TRIM LARGE PROBE SURFACE
  // ****************************************************
  auto trimgrid = make_zeroed_grid();
  copyGrid(biggrid.get(), trimgrid.get());
  trun_ExcludeGrid(TRIMPROBE, biggrid.get(), trimgrid.get());
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
// CALCULATE TOTAL SOLVENT
// ***************************************************
  auto solventEXC = make_zeroed_grid();
  grow_ExcludeGrid(SMPROBE, solventACC.get(), solventEXC.get());
  intersect_Grids(solventEXC.get(), trimgrid.get()); //modifies solventEXC
  const std::string solvent_output = outputs.mrcFile.empty() ? "allsolvent.mrc" : outputs.mrcFile;
  writeMRCFile(solventEXC.get(), const_cast<char*>(solvent_output.c_str()));
  solventACC.reset();

// ***************************************************
// SELECT PARTICULAR CHANNEL
// ***************************************************

  // initialize channel volume
  auto channelEXC = make_zeroed_grid();
  int solventEXCvol = countGrid(solventEXC.get());
  //main channel loop
  int numchannels=0;
  int allchannels=0;
  int connected;
  int maxvox=0, minvox=1000000, goodminvox=1000000;
  cerr << "MIN SIZE: " << MINSIZE << " voxels" << endl;
  cerr << "MIN SIZE: " << MINSIZE*GRIDVOL << " Angstroms" << endl;

  while ( countGrid(solventEXC.get()) > MINSIZE ) {
    allchannels++;

    // get channel volume
    zeroGrid(channelEXC.get()); // initialize channelEXC volume
    int gp = get_GridPoint(solventEXC.get());  //modify x,y,z
    connected = get_Connected_Point(solventEXC.get(), channelEXC.get(), gp); //modify channelEXC
    subt_Grids(solventEXC.get(), channelEXC.get()); //modify solventEXC

    // ***************************************************
    // GETTING CONTACT CHANNEL
    // ***************************************************
    int channelEXCvol = countGrid(channelEXC.get()); // initialize channelEXC volume
    if (channelEXCvol > maxvox)
      maxvox = channelEXCvol;
    if (channelEXCvol < minvox and channelEXCvol > 0)
      minvox = channelEXCvol;
    if (channelEXCvol <= MINSIZE) {
      cerr << "SKIPPING CHANNEL" << endl;
      cerr << "---------------------------------------------" << endl;
      continue;
    }
    if (channelEXCvol < goodminvox)
      goodminvox = channelEXCvol;

    numchannels++;

    // ***************************************************
    // OUTPUT RESULTS
    // ***************************************************
    cout << BIGPROBE << "\t" << SMPROBE << "\t" << GRID << "\t" << flush;
    printVolCout(channelEXCvol);
    long double surf = surface_area(channelEXC.get());
    cout << "\t" << surf << "\t" << flush;
    cout << "\t#" << input_path << endl;
    char channel_file[64];
    std::snprintf(channel_file, sizeof(channel_file), "channel-%03d.mrc", numchannels);
    writeSmallMRCFile(channelEXC.get(), channel_file);
    cerr << "---------------------------------------------" << endl;
  }
  if (numchannels == 0)
    goodminvox = 0;
  cerr << "Channel min size: " << int(minvox*GRIDVOL) << " A (all) " << int(goodminvox*GRIDVOL) << " A (good)" << endl;
  cerr << "Channel max size: " << int(maxvox*GRIDVOL) << " A " << endl;
  cerr << "Used " << numchannels << " of " << allchannels << " channels" << endl;
  if (allchannels > 0) {
    cerr << "Mean size: " << solventEXCvol/float(allchannels)*GRIDVOL << " A " << endl;
  }
  cerr << "Cutoff size: " << MINSIZE << " voxels :: "<< MINSIZE*GRIDVOL << " Angstroms" << endl;
  cerr << endl << "Program Completed Sucessfully" << endl << endl;
  return 0;
};
