#include <cstring>   // For std::strlen, std::strcpy, std::strrchr
#include <iostream>  // For std::cerr
#include <cstdio>    // For std::snprintf
#include <string>
#include <vector>

#include "argument_helper.hpp"
#include "pdb_io.hpp"
#include "utils.hpp"   // For custom utility functions
#include "vossvolvox_cli_common.hpp"
#include "xyzr_cli_helpers.hpp"

// Globals
extern float GRID, GRIDVOL;

// Function to get the directory name from a given file path
//
// Parameters:
//   - path: The input file path (string) from which the directory name is to be extracted.
//   - dir: A character array where the extracted directory name will be stored.
//
// Output:
//   - The directory part of the input file path is stored in the 'dir' variable.
//     If the path is invalid or does not contain a '/', it sets 'dir' to the current directory ("./").
//
// Modifies:
//   - The 'dir' array is modified in place to store the extracted directory name.
//   - The 'path' array is not modified.
void getDirname(char path[], char dir[]) {
  // Debugging: Output the initial path and directory values
  if (DEBUG > 0)
    std::cerr << "DIR: " << dir << " :: PATH: " << path << endl;

  // If the path is too short or does not contain a '/', default to current directory
  // This handles cases where the path is not valid or does not contain a directory
  if (strlen(path) < 3 || strrchr(path, '/') == NULL) {
    snprintf(dir, 256, "./");
    return;
  }

  // Find the last occurrence of '/' in the path to separate the directory and file
  char* temp;
  temp = strrchr(path, '/');  // Find the last slash in the path (separates file from directory)

  // Calculate the position where the directory ends
  int end = strlen(path) - strlen(temp) + 1;

  // Debugging: Output the position of the last '/' found in the path
  if (DEBUG > 0)
    printf("Last occurrence of '/' found at %d\n", end);

  // Copy the path into the 'dir' variable
  std::strcpy(dir, path);

  // Null-terminate the 'dir' string right after the last directory component
  dir[end] = '\0';  // Ensures that the 'dir' string contains only the directory part

  // Debugging: Output the final directory and the original path
  if (DEBUG > 0)
    std::cerr << "DIR: " << dir << " :: PATH: " << path << endl;

  return;
};

// Main function for channel extraction
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
  int numchan = 0;
  float grid_override = 0.0f;
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
                    grid_override,
                    0.0f,
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
  parser.add_option("-n",
                    "--num-channels",
                    numchan,
                    0,
                    "Number of channels to isolate (0 = all).",
                    "<count>");
  vossvolvox::add_output_options(parser, outputs);
  vossvolvox::add_filter_options(parser, filters);
  vossvolvox::add_debug_option(parser, debug);
  parser.add_example(
      "./AllChannel.exe -i 3hdi.xyzr -b 9.0 -s 1.5 -g 0.5 -t 4.0 -v 5000 -p 0.01 -n 1");

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
  if (grid_override > 0.0f) {
    GRID = grid_override;
  }

  vossvolvox::enable_debug(debug);
  vossvolvox::debug_report_cli(input_path, &outputs);

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

  char file[256] = "";
  char dirname[256] = "";
  char mrcfile[256] = "";
  std::snprintf(file, sizeof(file), "%s", input_path.c_str());
  if (!outputs.mrcFile.empty()) {
    std::snprintf(mrcfile, sizeof(mrcfile), "%s", outputs.mrcFile.c_str());
  }

  if (numchan > 0) {
    // set min volume really low and find biggest channels
    MINSIZE = 20;
  } else if (minvol > 0) {
    // set min volume to input, convert to voxels
    MINSIZE = int(minvol/GRIDVOL);
  } else if (minperc == 0) {
    // set min volume later
    minperc=0.01;
  }

  const std::vector<const XYZRBuffer*> buffers = {&xyzr_buffer};
  const auto grid_result = vossvolvox::prepare_grid_from_xyzr(
      buffers,
      GRID,
      static_cast<float>(BIGPROBE),
      input_path,
      false);
  const int numatoms = grid_result.total_atoms;

  // Print header information
  std::cerr << "Probe Radius: " << BIGPROBE << endl;
  std::cerr << "Grid Spacing: " << GRID << endl;
  std::cerr << "Resolution: " << int(1000.0 / float(GRIDVOL)) / 1000.0 << " voxels per A^3" << endl;
  std::cerr << "Resolution: " << int(11494.0 / float(GRIDVOL)) / 1000.0 << " voxels per water molecule" << endl;
  std::cerr << "Input file: " << file << endl;
  std::cerr << "Minimum size: " << MINSIZE << " voxels" << endl;

  // ****************************************************
  // STARTING LARGE PROBE
  // ****************************************************
  auto biggrid = make_zeroed_grid();
  int bigvox;
  if (BIGPROBE > 0.0) {
    bigvox = get_ExcludeGrid_fromArray(numatoms, BIGPROBE, xyzr_buffer, biggrid.get());
  } else {
    std::cerr << "BIGPROBE <= 0" << endl;
    return 1;
  }

  if (minperc > 0) {
    while (minperc > 1)
      minperc /= 100;
    MINSIZE = int(bigvox * minperc);
  }

  std::cerr << "CALCULATED MINSIZE: " << MINSIZE << endl;
  if (MINSIZE < 20) {
    std::cerr << "MINSIZE IS TOO SMALL, SETTING TO 20 VOXELS" << endl;
    MINSIZE = 20;
  }

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
  //snprintf(mrcfile, sizeof(mrcfile), "allsolvent.mrc");
  //writeMRCFile(solventEXC, mrcfile);
  solventEXC.reset();

  // ***************************************************
  // SELECT PARTICULAR CHANNEL
  // ***************************************************

  // initialize channel volume
  auto channelACC = make_zeroed_grid();

  //main channel loop
  int numchannels=0;
  int allchannels=0;
  int connected;
  int maxvox=0, minvox=1000000, goodminvox=1000000;
  int solventACCvol = countGrid(solventACC.get()); // initialize origSolventACC volume

  if (numchan > 0) {
    if (DEBUG > 0)
      std::cerr << "#######" << endl << "Starting NumChan Area" << endl
      << "#######" << endl;
    auto tempSolventACC = make_zeroed_grid();

    // set up a list
    std::vector<int> vollist(numchan + 2, 0);

    copyGrid(solventACC.get(), tempSolventACC.get()); // initialize tempSolventACC volume

    short int insert;
    while ( countGrid(tempSolventACC.get()) > MINSIZE ) {
      // get channel volume
      zeroGrid(channelACC.get()); // initialize channelACC volume

      // get first available filled point
      int gp = get_GridPoint(tempSolventACC.get());  //modify gp

      if (DEBUG > 0) {
        int tempSolventACCvol = countGrid(tempSolventACC.get());
        std::cerr << "Temp Solvent Volume ..." << tempSolventACCvol << endl;
      }

      // get all connected volume to point
      if (DEBUG > 0)
        std::cerr << "Time to crash..." << endl;
      connected = get_Connected_Point (tempSolventACC.get(), channelACC.get(), gp); //modify channelACC
      if (DEBUG > 0)
        std::cerr << "Connected voxel volume: " << connected << endl;

      // remove channel from total solvent
      subt_Grids(tempSolventACC.get(), channelACC.get()); //modify solventACC

      // get volume
      int channelACCvol = countGrid(channelACC.get());

      // skip volume if it is too small
      if (channelACCvol <= MINSIZE) {
        continue;
      }

      std::cerr << "Channel volume: " << int(channelACCvol*GRIDVOL) << " Angstroms^3" << endl;

      insert = 0;
      for (int i=0; i<numchan+2 && !insert; i++) {
        if (channelACCvol > 0 && channelACCvol > vollist[i]) {
          // shift elements over
          for (int j=numchan+2; j>i; j--) {
            vollist[j] = vollist[j-1];
          }
          // insert new guy
          vollist[i] = channelACCvol;
          insert = 1;
        }
      }
    }
    for (int i=0; i<numchan+2; i++) {
      std::cerr << "Vollist[] " << i << "\t" << vollist[i] << endl;
    }
    // set the minsize to be one less than a channel of
    MINSIZE = vollist[numchan-1] - 1;

    tempSolventACC.reset();
    if (MINSIZE < 10) {
      std::cerr << endl << "#######" << endl << "NO CHANNELS WERE FOUND" << endl
      << "#######" << endl;
      return 1;
    }
    std::cerr << "Setting minimum volume size in voxels (MINSIZE) to: "
    << MINSIZE << endl;
    if (DEBUG > 0)
      std::cerr << "#######" << endl << "Ending NumChan Area" << endl
      << "#######" << endl;
  }

  auto channelEXC = make_zeroed_grid();

  while ( countGrid(solventACC.get()) > MINSIZE ) {
    if (DEBUG > 0)
      std::cerr << endl << "Loop: Solvent Volume (" << countGrid(solventACC.get())
      << ") greater than MINSIZE (" << MINSIZE << ")" << endl << endl;
    allchannels++;

    // get channel volume
    zeroGrid(channelACC.get()); // initialize channelACC volume

    // get first available filled point
    int gp = get_GridPoint(solventACC.get());  //return gp

    if (DEBUG > 0) {
      std::cerr << "Solvent Volume ... " << countGrid(solventACC.get()) << endl;
      std::cerr << "Channel Volume ... " << countGrid(channelACC.get()) << endl;
    }

    if (DEBUG > 0)
      std::cerr << "Time to crash..." << endl;

    // get all connected volume to point
    connected = get_Connected_Point(solventACC.get(), channelACC.get(), gp); //modify channelACC
    if (DEBUG > 0)
      std::cerr << "Connected voxel volume: " << connected << endl;

    // remove channel from total solvent
    subt_Grids(solventACC.get(), channelACC.get()); //modify solventACC

    // initialize volume for excluded surface
    int channelACCvol = copyGrid(channelACC.get(), channelEXC.get()); // initialize channelEXC volume

    // statistics
    if (channelACCvol > maxvox)
      maxvox = channelACCvol;
    if (channelACCvol < minvox and channelACCvol > 0)
      minvox = channelACCvol;

    // skip volume if it is too small
    if (channelACCvol <= MINSIZE) {
      // print message if it is worth it
      if (channelACCvol > 20) {
        std::cerr << "Skipping channel " << allchannels << ": "
        << int(channelACCvol*GRIDVOL) << " Angstroms^3" << endl;
      }
      continue;
    }

    std::cerr << "Channel volume: " << int(channelACCvol*GRIDVOL) << " Angstroms^3" << endl;

    // statistics
    if (channelACCvol < goodminvox)
      goodminvox = channelACCvol;
    numchannels++;

    // get excluded surface
    grow_ExcludeGrid(SMPROBE, channelACC.get(), channelEXC.get());

    //limit growth to inside trimgrid
    int chanvox = intersect_Grids(channelEXC.get(), trimgrid.get()); //modifies channelEXC

    // output results
    if (DEBUG > 0)
      std::cerr << "MRC: " << mrcfile << " -- DIR: " << dirname << endl;

    if (strlen(dirname) < 3)
      getDirname(mrcfile, dirname);
    if (DEBUG > 0)
      std::cerr << "MRC: " << mrcfile << " -- DIR: " << dirname << endl;
    snprintf(mrcfile, sizeof(mrcfile), "%schannel-%03d.mrc", dirname, numchannels);

    printVolCout(chanvox);
    std::cerr << endl;
    writeSmallMRCFile(channelEXC.get(), mrcfile);
    //cerr << "---------------------------------------------" << endl;
  }

  if (numchannels == 0)
    goodminvox = 0;
  std::cerr << "Channel min size: " << int(minvox*GRIDVOL) << " A (all) " << int(goodminvox*GRIDVOL) << " A (good)" << endl;
  std::cerr << "Channel max size: " << int(maxvox*GRIDVOL) << " A " << endl;
  std::cerr << "Used " << numchannels << " of " << allchannels << " channels" << endl;
  if (allchannels > 0) {
    std::cerr << "Mean size: " << solventACCvol/float(allchannels)*GRIDVOL << " A " << endl;
  }
  std::cerr << "Cutoff size: " << MINSIZE << " voxels :: "<< MINSIZE*GRIDVOL << " Angstroms" << endl;
  std::cerr << endl << "Program Completed Sucessfully" << endl << endl;
  return 0;
};
