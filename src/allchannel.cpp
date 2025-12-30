#include <cstdlib>   // For std::free, std::malloc, std::exit, std::atof
#include <cstring>   // For std::strlen, std::strcpy, std::strrchr
#include <iostream>  // For std::cerr
#include <cstdio>    // For std::snprintf
#include <string>
#include "argument_helper.h"
#include "pdb_io.h"
#include "utils.h"   // For custom utility functions

extern float XMIN, YMIN, ZMIN;
extern float XMAX, YMAX, ZMAX;
extern int DX, DY, DZ;
extern int DXY, DXYZ;
extern unsigned int NUMBINS;
extern float MAXPROBE;
extern float GRID;
extern float GRIDVOL;
extern float WATER_RES;
extern float CUTOFF;
extern char XYZRFILE[256];

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

  std::string input_path;
  std::string pdb_file;
  std::string ezd_file;
  std::string mrc_file;
  double BIGPROBE = 9.0;
  double SMPROBE = 1.5;
  double TRIMPROBE = 4.0;
  int MINSIZE = 20;
  double minvol = 0.0;
  double minperc = 0.0;
  int numchan = 0;
  float grid_override = 0.0f;
  bool use_hydrogens = false;
  bool exclude_ions = false;
  bool exclude_ligands = false;
  bool exclude_hetatm = false;
  bool exclude_water = false;
  bool exclude_nucleic = false;
  bool exclude_amino = false;

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
  vossvolvox::add_output_file_options(parser, pdb_file, ezd_file, mrc_file);
  vossvolvox::add_xyzr_filter_flags(parser,
                                    use_hydrogens,
                                    exclude_ions,
                                    exclude_ligands,
                                    exclude_hetatm,
                                    exclude_water,
                                    exclude_nucleic,
                                    exclude_amino);
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

  if (!vossvolvox::quiet_mode()) {
    printCompileInfo(argv[0]);
    printCitation();
  }
  if (!pdb_file.empty() || !ezd_file.empty()) {
    std::cerr << "Warning: PDB/EZD outputs are not supported for this tool; ignoring.\n";
  }

  vossvolvox::pdbio::ConversionOptions convert_options;
  convert_options.use_united = !use_hydrogens;
  convert_options.filters.exclude_ions = exclude_ions;
  convert_options.filters.exclude_ligands = exclude_ligands;
  convert_options.filters.exclude_hetatm = exclude_hetatm;
  convert_options.filters.exclude_water = exclude_water;
  convert_options.filters.exclude_nucleic_acids = exclude_nucleic;
  convert_options.filters.exclude_amino_acids = exclude_amino;
  vossvolvox::pdbio::XyzrData xyzr_data;
  if (!vossvolvox::pdbio::ReadFileToXyzr(input_path, convert_options, xyzr_data)) {
    std::cerr << "Error: unable to load XYZR data from '" << input_path << "'\n";
    return 1;
  }
  XYZRBuffer xyzr_buffer;
  xyzr_buffer.atoms.reserve(xyzr_data.atoms.size());
  for (const auto& atom : xyzr_data.atoms) {
    xyzr_buffer.atoms.push_back(
        XYZRAtom{static_cast<float>(atom.x),
                 static_cast<float>(atom.y),
                 static_cast<float>(atom.z),
                 static_cast<float>(atom.radius)});
  }
  if (!XYZRFILE[0]) {
    std::strncpy(XYZRFILE, input_path.c_str(), sizeof(XYZRFILE));
    XYZRFILE[sizeof(XYZRFILE) - 1] = '\0';
  }

  char file[256] = "";
  char dirname[256] = "";
  char mrcfile[256] = "";
  std::snprintf(file, sizeof(file), "%s", input_path.c_str());
  if (!mrc_file.empty()) {
    std::snprintf(mrcfile, sizeof(mrcfile), "%s", mrc_file.c_str());
  }

  if (numchan > 0) {
    // set min volume really low and find biggest channels
    MINSIZE = 20;
  } else if (minvol > 0) {
    // set min volume to input, convert to voxels
    MINSIZE = int(minvol/GRID/GRID/GRID);
  } else if (minperc == 0) {
    // set min volume later
    minperc=0.01;
  }

  // Initialize grid dimensions
  finalGridDims(BIGPROBE);

  // Print header information
  std::cerr << "Probe Radius: " << BIGPROBE << endl;
  std::cerr << "Grid Spacing: " << GRID << endl;
  std::cerr << "Resolution: " << int(1000.0 / float(GRIDVOL)) / 1000.0 << " voxels per A^3" << endl;
  std::cerr << "Resolution: " << int(11494.0 / float(GRIDVOL)) / 1000.0 << " voxels per water molecule" << endl;
  std::cerr << "Input file: " << file << endl;
  std::cerr << "Minimum size: " << MINSIZE << " voxels" << endl;

  // First pass to read atoms and check limits
  int numatoms = read_NumAtoms_from_array(xyzr_buffer);
  assignLimits();

  // ****************************************************
  // STARTING LARGE PROBE
  // ****************************************************
  gridpt *biggrid;
  biggrid = (gridpt*) std::malloc (NUMBINS);
  if (biggrid == NULL) { std::cerr << "GRID IS NULL" << endl; return 1; }
  zeroGrid(biggrid);
  int bigvox;
  if (BIGPROBE > 0.0) {
    bigvox = get_ExcludeGrid_fromArray(numatoms, BIGPROBE, xyzr_buffer, biggrid);
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
  gridpt *trimgrid;
  trimgrid = (gridpt*) std::malloc (NUMBINS);
  if (trimgrid==NULL) { std::cerr << "GRID IS NULL" << endl; return 1; }
  copyGrid(biggrid,trimgrid);
  trun_ExcludeGrid(TRIMPROBE,biggrid,trimgrid);
  std::free (biggrid);

  //cout << "bg_prb\tsm_prb\tgrid\texcvol\tsurf\taccvol\tfile" << endl;

  // ****************************************************
  // STARTING SMALL PROBE
  // ****************************************************
  gridpt *smgrid;
  smgrid = (gridpt*) std::malloc (NUMBINS);
  if (smgrid==NULL) { std::cerr << "GRID IS NULL" << endl; return 1; }
  zeroGrid(smgrid);
  int smvox;
  smvox = fill_AccessGrid_fromArray(numatoms, SMPROBE, xyzr_buffer, smgrid);

  // ****************************************************
  // GETTING ACCESSIBLE CHANNELS
  // ****************************************************
  gridpt *solventACC;
  solventACC = (gridpt*) std::malloc (NUMBINS);
  if (solventACC==NULL) { std::cerr << "GRID IS NULL" << endl; return 1; }
  copyGrid(trimgrid, solventACC); //copy trimgrid into solventACC
  subt_Grids(solventACC, smgrid); //modify solventACC
  std::free (smgrid);

  // ***************************************************
  // CALCULATE TOTAL SOLVENT
  // ***************************************************
  gridpt *solventEXC;
  solventEXC = (gridpt*) std::malloc (NUMBINS);
  if (solventEXC==NULL) { std::cerr << "GRID IS NULL" << endl; return 1; }
  grow_ExcludeGrid(SMPROBE, solventACC, solventEXC);
  intersect_Grids(solventEXC, trimgrid); //modifies solventEXC
  //snprintf(mrcfile, sizeof(mrcfile), "allsolvent.mrc");
  //writeMRCFile(solventEXC, mrcfile);
  std::free (solventEXC);

  // ***************************************************
  // SELECT PARTICULAR CHANNEL
  // ***************************************************

  // initialize channel volume
  gridpt *channelACC;
  channelACC = (gridpt*) std::malloc (NUMBINS);
  if (channelACC==NULL) { std::cerr << "GRID IS NULL" << endl; return 1; }

  //main channel loop
  int numchannels=0;
  int allchannels=0;
  int connected;
  int maxvox=0, minvox=1000000, goodminvox=1000000;
  int solventACCvol = countGrid(solventACC); // initialize origSolventACC volume

  if (numchan > 0) {
    if (DEBUG > 0)
      std::cerr << "#######" << endl << "Starting NumChan Area" << endl
      << "#######" << endl;
    gridpt *tempSolventACC;
    tempSolventACC = (gridpt*) std::malloc (NUMBINS);
    if (tempSolventACC==NULL) { std::cerr << "GRID IS NULL" << endl; return 1; }

    // set up a list
    int *vollist;
    vollist = (int*) std::malloc ((numchan+2)*sizeof(int));
    if (vollist==NULL) { std::cerr << "LIST IS NULL" << endl; return 1; }
    for (int i=0; i<numchan+2; i++) {
      vollist[i] = 0;
    }

    copyGrid(solventACC, tempSolventACC); // initialize tempSolventACC volume

    short int insert;
    while ( countGrid(tempSolventACC) > MINSIZE ) {
      // get channel volume
      zeroGrid(channelACC); // initialize channelACC volume

      // get first available filled point
      int gp = get_GridPoint(tempSolventACC);  //modify gp

      if (DEBUG > 0) {
        int tempSolventACCvol = countGrid(tempSolventACC);
        std::cerr << "Temp Solvent Volume ..." << tempSolventACCvol << endl;
      }

      // get all connected volume to point
      if (DEBUG > 0)
        std::cerr << "Time to crash..." << endl;
      connected = get_Connected_Point (tempSolventACC, channelACC, gp); //modify channelACC
      if (DEBUG > 0)
        std::cerr << "Connected voxel volume: " << connected << endl;

      // remove channel from total solvent
      subt_Grids(tempSolventACC, channelACC); //modify solventACC

      // get volume
      int channelACCvol = countGrid(channelACC);

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

    std::free (tempSolventACC);
    std::free (vollist);
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

  gridpt *channelEXC;
  channelEXC = (gridpt*) std::malloc (NUMBINS);
  if (channelEXC==NULL) { std::cerr << "GRID IS NULL" << endl; return 1; }

  while ( countGrid(solventACC) > MINSIZE ) {
    if (DEBUG > 0)
      std::cerr << endl << "Loop: Solvent Volume (" << countGrid(solventACC)
      << ") greater than MINSIZE (" << MINSIZE << ")" << endl << endl;
    allchannels++;

    // get channel volume
    zeroGrid(channelACC); // initialize channelACC volume

    // get first available filled point
    int gp = get_GridPoint(solventACC);  //return gp

    if (DEBUG > 0) {
      std::cerr << "Solvent Volume ... " << countGrid(solventACC) << endl;
      std::cerr << "Channel Volume ... " << countGrid(channelACC) << endl;
    }

    if (DEBUG > 0)
      std::cerr << "Time to crash..." << endl;

    // get all connected volume to point
    connected = get_Connected_Point(solventACC, channelACC, gp); //modify channelACC
    if (DEBUG > 0)
      std::cerr << "Connected voxel volume: " << connected << endl;

    // remove channel from total solvent
    subt_Grids(solventACC, channelACC); //modify solventACC

    // initialize volume for excluded surface
    int channelACCvol = copyGrid(channelACC, channelEXC); // initialize channelEXC volume

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
    grow_ExcludeGrid(SMPROBE, channelACC, channelEXC);

    //limit growth to inside trimgrid
    int chanvox = intersect_Grids(channelEXC, trimgrid); //modifies channelEXC

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
    writeSmallMRCFile(channelEXC, mrcfile);
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
  std::free (channelACC);
  std::free (channelEXC);
  std::free (solventACC);
  std::free (trimgrid);
  std::cerr << endl << "Program Completed Sucessfully" << endl << endl;
  return 0;
};
