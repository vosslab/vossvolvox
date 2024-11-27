#include <stdlib.h>                   // for free, malloc
#include <string.h>                   // for NULL, strrchr
#include <iostream>                   // for char_traits, cerr
#include "utils.h"                    // for endl, countGrid, gridpt, DEBUG

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
    snprintf(dir, sizeof(dir), "./");  // Set directory to current directory
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
  strcpy(dir, path);

  // Null-terminate the 'dir' string right after the last directory component
  dir[end] = '\0';  // Ensures that the 'dir' string contains only the directory part

  // Debugging: Output the final directory and the original path
  if (DEBUG > 0)
    std::cerr << "DIR: " << dir << " :: PATH: " << path << endl;

  return;
};

// Function to print help message for the program
void printHelp() {
  std::cerr << "Usage: ./AllChannel.exe -i <file> -b <big_probe> -s <small_probe> -g <gridspace> " << std::endl
  << "\t-t <trim_probe> -v <min_volume in A^3> -p <min_percent> -n <number_of_channels>" << std::endl
  << std::endl;
  std::cerr << "Purpose: AllChannel.exe extracts all solvent channels from a structure above a cutoff." << std::endl;
  std::cerr << std::endl;
  std::cerr << "Options:" << std::endl;
  std::cerr << "  -i <file>         : Input file in XYZR format." << std::endl;
  std::cerr << "  -b <big_probe>    : Probe radius for large probe (default is 9.0 Angstroms)." << std::endl;
  std::cerr << "  -s <small_probe>  : Probe radius for small probe (default is 1.5 Angstroms)." << std::endl;
  std::cerr << "  -g <gridspace>    : Grid spacing (default 1.0 Angstrom per voxel edge length, controlling the resolution of the grid)." << std::endl;
  std::cerr << "  -t <trim_probe>   : Probe radius for trimming the exterior solvent (default is 4.0 Angstroms)." << std::endl;
  std::cerr << "  -v <min_volume>   : Minimum volume in A^3 for a channel to be considered." << std::endl;
  std::cerr << "  -p <min_percent>  : Minimum percentage of volume for channel inclusion (e.g., 0.01 for 1%)." << std::endl;
  std::cerr << "  -n <num_channels> : Number of channels to isolate (optional, default is to find all)." << std::endl;
  std::cerr << std::endl;
  std::cerr << "Example: ./AllChannel.exe -i 3hdi.xyzr -b 9.0 -s 1.5 -g 0.5 -t 4.0 -v 5000 -p 0.01 -n 1" << std::endl;
  std::cerr << std::endl;
  std::cerr << "AllChannel.exe will output multiple MRC files, each representing an isolated channel." << std::endl;
}

// Function to process command-line arguments and update the passed variables
void processArguments(int argc, char* argv[], char file[], double &BIGPROBE, double &SMPROBE, double &TRIMPROBE, double &minperc, char mrcfile[], int &numchan, double &minvol, float &GRID) {
  // Initialize defaults
  BIGPROBE = 9.0;
  SMPROBE = 1.5;
  TRIMPROBE = 4.0;
  minperc = 0;
  numchan = 0;
  minvol = 0;
  GRID = 0.0;

  // Process command-line arguments
  while (argc > 1 && argv[1][0] == '-') {
    if (argv[1][1] == 'i') {
      snprintf(file, sizeof(file), &argv[2][0]);
    } else if (argv[1][1] == 'b') {
      BIGPROBE = atof(&argv[2][0]);
    } else if (argv[1][1] == 's') {
      SMPROBE = atof(&argv[2][0]);
    } else if (argv[1][1] == 't') {
      TRIMPROBE = atof(&argv[2][0]);
    } else if (argv[1][1] == 'p') {
      minperc = atof(&argv[2][0]);
    } else if (argv[1][1] == 'm') {
      snprintf(mrcfile, sizeof(mrcfile), &argv[2][0]);
    } else if (argv[1][1] == 'n') {
      numchan = int(atof(&argv[2][0]));
    } else if (argv[1][1] == 'v') {
      minvol = atof(&argv[2][0]);
    } else if (argv[1][1] == 'g') {
      GRID = atof(&argv[2][0]);
    } else if (argv[1][1] == 'h') {
      // Print help message
      printHelp();
      exit(0);  // Exit after printing help
    }
    --argc; --argc;
    ++argv; ++argv;
  }
}



// Main function for channel extraction
int main(int argc, char *argv[]) {
  std::cerr << std::endl;

  // Print compile information and citation
  printCompileInfo(argv[0]);
  printCitation();

  // Variable initialization
  char file[256] = "";  // Input file name (initialized to an empty string)
  char dirname[256] = "";  // Directory name (initialized to an empty string)
  char mrcfile[256] = "";  // MRC file name (initialized to an empty string)
  double BIGPROBE = 9.0;  // Default big probe radius
  double SMPROBE = 1.5;  // Default small probe radius
  double TRIMPROBE = 4.0;  // Default trim probe radius
  int MINSIZE = 20;  // Default minimum size for channels
  double minvol = 0;  // Minimum volume
  double minperc = 0;  // Minimum percentage for channel volume
  int numchan = 0;  // Number of channels to isolate (0 means all channels)
  float GRID = 1.0;  // Grid spacing (default 1.0 invalid)

  // Call to process command-line arguments and modify variables accordingly
  processArguments(argc, argv, file, BIGPROBE, SMPROBE, TRIMPROBE, minperc, mrcfile, numchan, minvol, GRID);

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
  int numatoms = read_NumAtoms(file);
  assignLimits();

  // ****************************************************
  // STARTING LARGE PROBE
  // ****************************************************
  gridpt *biggrid;
  biggrid = (gridpt*) malloc (NUMBINS);
  if (biggrid == NULL) { std::cerr << "GRID IS NULL" << endl; return 1; }
  zeroGrid(biggrid);
  int bigvox;
  if (BIGPROBE > 0.0) {
    bigvox = get_ExcludeGrid_fromFile(numatoms, BIGPROBE, file, biggrid);
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
  trimgrid = (gridpt*) malloc (NUMBINS);
  if (trimgrid==NULL) { std::cerr << "GRID IS NULL" << endl; return 1; }
  copyGrid(biggrid,trimgrid);
  trun_ExcludeGrid(TRIMPROBE,biggrid,trimgrid);
  free (biggrid);

  //cout << "bg_prb\tsm_prb\tgrid\texcvol\tsurf\taccvol\tfile" << endl;

  // ****************************************************
  // STARTING SMALL PROBE
  // ****************************************************
  gridpt *smgrid;
  smgrid = (gridpt*) malloc (NUMBINS);
  if (smgrid==NULL) { std::cerr << "GRID IS NULL" << endl; return 1; }
  zeroGrid(smgrid);
  int smvox;
  smvox = fill_AccessGrid_fromFile(numatoms,SMPROBE,file,smgrid);

  // ****************************************************
  // GETTING ACCESSIBLE CHANNELS
  // ****************************************************
  gridpt *solventACC;
  solventACC = (gridpt*) malloc (NUMBINS);
  if (solventACC==NULL) { std::cerr << "GRID IS NULL" << endl; return 1; }
  copyGrid(trimgrid, solventACC); //copy trimgrid into solventACC
  subt_Grids(solventACC, smgrid); //modify solventACC
  free (smgrid);

  // ***************************************************
  // CALCULATE TOTAL SOLVENT
  // ***************************************************
  gridpt *solventEXC;
  solventEXC = (gridpt*) malloc (NUMBINS);
  if (solventEXC==NULL) { std::cerr << "GRID IS NULL" << endl; return 1; }
  grow_ExcludeGrid(SMPROBE, solventACC, solventEXC);
  intersect_Grids(solventEXC, trimgrid); //modifies solventEXC
  //snprintf(mrcfile, sizeof(mrcfile), "allsolvent.mrc");
  //writeMRCFile(solventEXC, mrcfile);
  free (solventEXC);

  // ***************************************************
  // SELECT PARTICULAR CHANNEL
  // ***************************************************

  // initialize channel volume
  gridpt *channelACC;
  channelACC = (gridpt*) malloc (NUMBINS);
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
    tempSolventACC = (gridpt*) malloc (NUMBINS);
    if (tempSolventACC==NULL) { std::cerr << "GRID IS NULL" << endl; return 1; }

    // set up a list
    int *vollist;
    vollist = (int*) malloc ((numchan+2)*sizeof(int));
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

    free (tempSolventACC);
    free (vollist);
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
  channelEXC = (gridpt*) malloc (NUMBINS);
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
  free (channelACC);
  free (channelEXC);
  free (solventACC);
  free (trimgrid);
  std::cerr << endl << "Program Completed Sucessfully" << endl << endl;
  return 0;
};

