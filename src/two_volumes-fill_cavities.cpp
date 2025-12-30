#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include "argument_helper.hpp"
#include "pdb_io.hpp"
#include "utils.hpp"                    // for endl, cerr, gridpt, countGrid
#include "vossvolvox_cli_common.hpp"
#include "xyzr_cli_helpers.hpp"

// ****************************************************
// CALCULATE EXCLUDED VOLUME, BUT FILL ANY CAVITIES
// designed for use with 3d printer
// ****************************************************

// Globals
extern float GRID, GRIDVOL;
extern int DX, DY, DZ;
extern unsigned int NUMBINS;

int getCavitiesBothMeth(const float probe, gridpt shellACC[], gridpt shellEXC[],
	const int natoms, char file1[], char file2[], char mrcfile1[], char mrcfile2[]);

int main(int argc, char *argv[]) {
  std::cerr << std::endl;
  vossvolvox::set_command_line(argc, argv);

  std::string file1;
  std::string file2;
  std::string mrcfile1;
  std::string mrcfile2;
  double PROBE1 = -1;
  double PROBE2 = -1;
  unsigned int merge = 0;
  unsigned int fill = 0;
  float grid = GRID;
  vossvolvox::FilterSettings filters;
  vossvolvox::DebugSettings debug;

  vossvolvox::ArgumentParser parser(
      argv[0],
      "Produce two solvent-excluded volumes on the same grid for 3D printing.");
  parser.add_option("-i1",
                    "--input1",
                    file1,
                    std::string(),
                    "First input structure file (XYZR, PDB, mmCIF, PDBML).",
                    "<input1>");
  parser.add_option("-i2",
                    "--input2",
                    file2,
                    std::string(),
                    "Second input structure file (XYZR, PDB, mmCIF, PDBML).",
                    "<input2>");
  parser.add_option("-p1",
                    "--probe1",
                    PROBE1,
                    -1.0,
                    "Probe radius for the first file.",
                    "<probe1>");
  parser.add_option("-p2",
                    "--probe2",
                    PROBE2,
                    -1.0,
                    "Probe radius for the second file.",
                    "<probe2>");
  parser.add_option("-g",
                    "--grid",
                    grid,
                    GRID,
                    "Grid spacing in Angstroms.",
                    "<grid>");
  parser.add_option("-m1",
                    "--mrc-output1",
                    mrcfile1,
                    std::string(),
                    "Output MRC file for the first volume.",
                    "<mrc1>");
  parser.add_option("-m2",
                    "--mrc-output2",
                    mrcfile2,
                    std::string(),
                    "Output MRC file for the second volume.",
                    "<mrc2>");
  parser.add_option("",
                    "--merge",
                    merge,
                    0u,
                    "Merge mode (0=no merge, 1=vol1<-vol2, 2=vol2<-vol1).",
                    "<0|1|2>");
  parser.add_option("",
                    "--fill",
                    fill,
                    0u,
                    "Fill mode for MakerBot adjustment (0=none, 1=vol2->vol1, 2=vol1->vol2).",
                    "<0|1|2>");
  vossvolvox::add_filter_options(parser, filters);
  vossvolvox::add_debug_option(parser, debug);
  parser.add_example("./TwoVol.exe -i1 prot.xyzr -i2 lig.xyzr -p1 1.5 -p2 3 -g 0.6 -m1 prot.mrc -m2 lig.mrc");

  const auto parse_result = parser.parse(argc, argv);
  if (parse_result == vossvolvox::ArgumentParser::ParseResult::HelpRequested) {
    return 0;
  }
  if (parse_result == vossvolvox::ArgumentParser::ParseResult::Error) {
    return 1;
  }
  if (file1.empty() || file2.empty()) {
    std::cerr << "Error: both --input1 and --input2 must be provided.\n";
    parser.print_help();
    return 1;
  }

  vossvolvox::enable_debug(debug);
  vossvolvox::debug_report_cli(file1 + "," + file2, nullptr);

  GRID = grid;

  if (!vossvolvox::quiet_mode()) {
    printCompileInfo(argv[0]);
    printCitation();
  }

  const auto convert_options = vossvolvox::make_conversion_options(filters);

  XYZRBuffer xyzr_buffer1;
  if (!vossvolvox::load_xyzr_or_exit(file1, convert_options, xyzr_buffer1)) {
    return 1;
  }
  XYZRBuffer xyzr_buffer2;
  if (!vossvolvox::load_xyzr_or_exit(file2, convert_options, xyzr_buffer2)) {
    return 1;
  }
  
  if (PROBE1 < 0 && PROBE2 > 0) {
  	PROBE1 = PROBE2;
  } else if (PROBE2 < 0 && PROBE1 > 0) {
    PROBE2 = PROBE1;
  } else if (PROBE1 < 0 && PROBE2 < 0) {
  	  cerr << "Error please define a probe radius, example: -p1 1.5" << endl;
      cerr << endl;
      return 1;    
  }
  double maxPROBE, minPROBE;
  if (PROBE1 > PROBE2) {
  	maxPROBE = PROBE1;
  	minPROBE = PROBE2;
  } else {
    maxPROBE = PROBE2;
    minPROBE = PROBE1;
  }
// ****************************************************
// INITIALIZATION
// ****************************************************

//INITIALIZE GRID
  const std::vector<const XYZRBuffer*> buffers = {&xyzr_buffer1, &xyzr_buffer2};
  const auto grid_result = vossvolvox::prepare_grid_from_xyzr(
      buffers,
      grid,
      static_cast<float>(maxPROBE * 2),
      file1,
      false);
  int numatoms1 = 0;
  int numatoms2 = 0;
  if (grid_result.per_input.size() >= 2) {
    numatoms1 = grid_result.per_input[0];
    numatoms2 = grid_result.per_input[1];
  }
//HEADER CHECK
  cerr << "Grid Spacing: " << GRID << endl;
  cerr << "Resolution:      " << int(1000.0/float(GRIDVOL))/1000.0 << " voxels per A^3" << endl;
  cerr << "Resolution:      " << int(11494.0/float(GRIDVOL))/1000.0 << " voxels per water molecule" << endl;
  cerr << "Input file 1:   " << file1 << endl;
  cerr << "Input file 2:   " << file2 << endl;
  cerr << "DIMENSIONS:   " << DX << ", " << DY << ", " << DZ << endl;

  int voxels1, voxels2;

// ****************************************************
// STARTING FILE 1 READ-IN
// ****************************************************

  auto shellACC = make_zeroed_grid();
  fill_AccessGrid_fromArray(numatoms1, PROBE1, xyzr_buffer1, shellACC.get());
  if(merge == 1) {
    auto shellACC2 = make_zeroed_grid();
    fill_AccessGrid_fromArray(numatoms2, minPROBE, xyzr_buffer2, shellACC2.get());
    cerr << "Merge Volumes 1->2" << endl;
    merge_Grids(shellACC.get(), shellACC2.get());
  }
  voxels1 = countGrid(shellACC.get());
  fill_cavities(shellACC.get());
  voxels2 = countGrid(shellACC.get());
  cerr << "Fill Cavities: " << voxels2 - voxels1 << " voxels filled" << endl;

  auto EXCgrid1 = make_zeroed_grid();
  trun_ExcludeGrid(PROBE1, shellACC.get(), EXCgrid1.get());

  shellACC.reset();
  
// ****************************************************
// STARTING FILE 2 READ-IN
// ****************************************************

  shellACC = make_zeroed_grid();
  fill_AccessGrid_fromArray(numatoms2, PROBE2, xyzr_buffer2, shellACC.get());
  if(merge == 2) {
    auto shellACC2 = make_zeroed_grid();
    fill_AccessGrid_fromArray(numatoms2, minPROBE, xyzr_buffer1, shellACC2.get());
    cerr << "Merge Volumes 2->1" << endl;
    merge_Grids(shellACC.get(), shellACC2.get());
  }  
  voxels1 = countGrid(shellACC.get());
  fill_cavities(shellACC.get());
  voxels2 = countGrid(shellACC.get());
  cerr << "Fill Cavities: " << voxels2 - voxels1 << " voxels filled" << endl;

  auto EXCgrid2 = make_zeroed_grid();
  trun_ExcludeGrid(PROBE2, shellACC.get(), EXCgrid2.get());

  shellACC.reset();
  
// ****************************************************
// SUBTRACT AND SAVE
// ****************************************************

  //voxels1 = countGrid(EXCgrid1);

  cerr << "subtract grids" << endl;
  if(merge == 1) {
    subt_Grids(EXCgrid1.get(), EXCgrid2.get()); //modifies first grid
  } else {
    subt_Grids(EXCgrid2.get(), EXCgrid1.get()); //modifies first grid
  }
  
  cerr << "makerbot fill" << endl;
  if(fill == 1) {
  	makerbot_fill(EXCgrid2.get(), EXCgrid1.get());
  } else if(fill == 2) {
    makerbot_fill(EXCgrid1.get(), EXCgrid2.get());
  } else {
  	cerr << "no fill" << endl;
  }
  /* 
  ** makerbot_fill()
  ** Since you cannot see inside a 3D print, 
  ** points that are invisible in ingrid are
  ** converted to outgrid points
  ** This way the printer does not have switch 
  ** colors on the invisible (internal) parts of the model
  ** So outgrid gets bigger
  */

  if(!mrcfile1.empty()) {
    writeMRCFile(EXCgrid1.get(), const_cast<char*>(mrcfile1.c_str()));
  }

  if(!mrcfile2.empty()) {
    writeMRCFile(EXCgrid2.get(), const_cast<char*>(mrcfile2.c_str()));
  }

  //cout << PROBE1 << "\t" << PROBE2 << "\t" << GRID << "\t" << endl;


  cerr << endl << "Program Completed Sucessfully" << endl << endl;
  return 0;



};
