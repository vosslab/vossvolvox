#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include "argument_helper.h"
#include "pdb_io.h"
#include "utils.h"
#include "vossvolvox_cli_common.hpp"
#include "xyzr_cli_helpers.h"

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

int getCavitiesBothMeth(const float probe,
                        gridpt shellACC[],
                        gridpt shellEXC[],
                        const int natoms,
                        const XYZRBuffer& xyzr_buffer,
                        const std::string& input_label,
                        char ezdfile[],
                        char pdbfile[],
                        char mrcfile[]);

int main(int argc, char *argv[]) {
  std::cerr << std::endl;
  vossvolvox::set_command_line(argc, argv);

  std::string input_path;
  vossvolvox::OutputSettings outputs;
  double shell_rad = 10.0;
  double probe_rad = 3.0;
  double trim_rad = 3.0;
  float grid = GRID;
  vossvolvox::FilterSettings filters;

  vossvolvox::ArgumentParser parser(
      argv[0],
      "Extract cavities within a molecular structure for a given probe radius.");
  vossvolvox::add_input_option(parser, input_path);
  parser.add_option("-b",
                    "--shell-radius",
                    shell_rad,
                    10.0,
                    "Shell (big probe) radius in Angstroms.",
                    "<shell radius>");
  parser.add_option("-s",
                    "--probe-radius",
                    probe_rad,
                    3.0,
                    "Probe radius in Angstroms.",
                    "<probe>");
  parser.add_option("-t",
                    "--trim-radius",
                    trim_rad,
                    3.0,
                    "Trim radius applied to the shell (Angstroms).",
                    "<trim>");
  parser.add_option("-g",
                    "--grid",
                    grid,
                    GRID,
                    "Grid spacing in Angstroms.",
                    "<grid spacing>");
  vossvolvox::add_output_options(parser, outputs);
  vossvolvox::add_filter_options(parser, filters);
  parser.add_example("./Cavities.exe -i 1a01.xyzr -b 10 -s 3 -t 3 -g 0.5 -o cavities.pdb");

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
      static_cast<float>(shell_rad * 2),
      input_path,
      false);
  const int numatoms = grid_result.total_atoms;

// ****************************************************
// INITIALIZATION
// ****************************************************
//HEADER CHECK
  cerr << "Grid Spacing: " << GRID << endl;
  cerr << "Resolution:      " << int(1000.0/float(GRIDVOL))/1000.0 << " voxels per A^3" << endl;
  cerr << "Resolution:      " << int(11494.0/float(GRIDVOL))/1000.0 << " voxels per water molecule" << endl;
  cerr << "Input file:   " << input_path << endl;
// ****************************************************
// STARTING FILE READ-IN
// ****************************************************

  gridpt *shellACC=NULL;
  shellACC = (gridpt*) std::malloc (NUMBINS);
  fill_AccessGrid_fromArray(numatoms, shell_rad, xyzr_buffer, shellACC);
  fill_cavities(shellACC);

  gridpt *shellEXC=NULL;
  shellEXC = (gridpt*) std::malloc (NUMBINS);
  trun_ExcludeGrid(shell_rad,shellACC,shellEXC);

// ****************************************************
// STARTING MAIN PROGRAM
// ****************************************************

  getCavitiesBothMeth(probe_rad,
                      shellACC,
                      shellEXC,
                      numatoms,
                      xyzr_buffer,
                      input_path,
                      const_cast<char*>(outputs.ezdFile.c_str()),
                      const_cast<char*>(outputs.pdbFile.c_str()),
                      const_cast<char*>(outputs.mrcFile.c_str()));

// ****************************************************
// CLEAN UP AND QUIT
// ****************************************************

  std::free (shellACC);
  std::free (shellEXC);

  cerr << endl << "Program Completed Sucessfully" << endl << endl;
  return 0;
};

int getCavitiesBothMeth(const float probe,
                        gridpt shellACC[],
                        gridpt shellEXC[],
                        const int natoms,
                        const XYZRBuffer& xyzr_buffer,
                        const std::string& input_label,
                        char ezdfile[],
                        char pdbfile[],
                        char mrcfile[])
{
/* THIS USES THE ACCESSIBLE SHELL AS THE BIG SURFACE */
/*******************************************************
Accessible Process
*******************************************************/

//Create access map
  gridpt *access=NULL;
  access = (gridpt*) std::malloc (NUMBINS);
  fill_AccessGrid_fromArray(natoms, probe, xyzr_buffer, access);

//Create inverse access map
  gridpt *cavACC=NULL;
  cavACC = (gridpt*) std::malloc (NUMBINS);
  copyGrid(shellACC,cavACC);
  subt_Grids(cavACC,access); //modifies cavACC
  std::free (access);
  int achanACC_voxels = countGrid(cavACC);

// EXTRA STEPS TO REMOVE SURFACE CAVITIES???

//Get first point
  bool stop = 1; int firstpt = 0;
  for(unsigned int pt=1; pt<NUMBINS && stop; pt++) {
    if(cavACC[pt]) { stop = 0; firstpt = pt;}
  }
  cerr << "FIRST POINT: " << firstpt << endl;
//LAST POINT
  stop = 1; int lastpt = NUMBINS-1;
  for(unsigned int pt=NUMBINS-1; pt>0 && stop; pt--) {
    if(cavACC[pt]) { stop = 0; lastpt = pt;}
  }
  cerr << "LAST  POINT: " << lastpt << endl;
//  get_Connected_Point(cavACC,chanACC,lastpt);


//Pull channels out of inverse access map
  gridpt *chanACC=NULL;
  chanACC = (gridpt*) std::malloc (NUMBINS);
  zeroGrid(chanACC);
  cerr << "Getting Connected Next" << endl;
  get_Connected_Point(cavACC,chanACC,firstpt); //modifies chanACC
  get_Connected_Point(cavACC,chanACC,lastpt); //modifies chanACC
  int chanACC_voxels = countGrid(chanACC);
//Subtract channels from access map leaving cavities
  subt_Grids(cavACC,chanACC); //modifies cavACC
  std::free (chanACC);
  int cavACC_voxels = countGrid(cavACC);

//Grow Access Cavs
  gridpt *ecavACC=NULL;
  ecavACC = (gridpt*) std::malloc (NUMBINS);
  grow_ExcludeGrid(probe,cavACC,ecavACC);
  std::free (cavACC);
  
//Intersect Grown Access Cavities with Shell
  int scavACC_voxels = countGrid(ecavACC);
  int ecavACC_voxels = intersect_Grids(ecavACC,shellEXC); //modifies ecavACC

  //float surfEXC = surface_area(ecavACC);
  std::free (ecavACC);

/*******************************************************
Excluded Process
*******************************************************/

//Create access map
  gridpt *access2=NULL;
  access2 = (gridpt*) std::malloc (NUMBINS);
  fill_AccessGrid_fromArray(natoms, probe, xyzr_buffer, access2);

//Create exclude map
  gridpt *exclude=NULL;
  exclude = (gridpt*) std::malloc (NUMBINS);
  trun_ExcludeGrid(probe,access2,exclude);
  std::free (access2);

//Create inverse exclude map
  gridpt *cavEXC=NULL;
  cavEXC = (gridpt*) std::malloc (NUMBINS);
  copyGrid(shellEXC,cavEXC);
  subt_Grids(cavEXC,exclude); //modifies cavEXC
  std::free (exclude);
  int echanEXC_voxels = countGrid(cavEXC);

//Get first point
  stop = 1; firstpt = 0;
  for(unsigned int pt=1; pt<NUMBINS && stop; pt++) {
    if(cavEXC[pt]) { stop = 0; firstpt = pt;}
  }
  cerr << "FIRST POINT: " << firstpt << endl;
//LAST POINT
  stop = 1; lastpt = NUMBINS-1;
  for(unsigned int pt=NUMBINS-1; pt>0 && stop; pt--) {
    if(cavEXC[pt]) { stop = 0; lastpt = pt;}
  }
  cerr << "LAST  POINT: " << lastpt << endl;

//Pull channels out of inverse excluded map
  gridpt *chanEXC=NULL;
  chanEXC = (gridpt*) std::malloc (NUMBINS);
  zeroGrid(chanEXC);
  cerr << "Getting Connected Next" << endl;
  get_Connected_Point(cavEXC,chanEXC,firstpt); //modifies chanEXC
  get_Connected_Point(cavEXC,chanEXC,lastpt); //modifies chanEXC
  int chanEXC_voxels = countGrid(chanEXC);
//Subtract channels from exclude map leaving cavities
  subt_Grids(cavEXC,chanEXC); //modifies cavEXC
  std::free (chanEXC);
  int cavEXC_voxels = countGrid(cavEXC);

//Write out exclude cavities
  if(pdbfile[0] != '\0') {
    write_SurfPDB(cavEXC,pdbfile);
  }
  if(ezdfile[0] != '\0') {
    write_HalfEZD(cavEXC,ezdfile);
  }
  if(mrcfile[0] != '\0') {
    writeMRCFile(cavEXC, mrcfile);
  }
  //float surfEXC = surface_area(cavEXC);
  std::free (cavEXC);

  cerr << endl;
  cerr << "achanACC_voxels = " << achanACC_voxels << endl
       << "chanACC_voxels  = " << chanACC_voxels << endl
       << "cavACC_voxels   = " << cavACC_voxels << endl
       << "scavACC_voxels  = " << scavACC_voxels << endl
       << "-------------------------------------" << endl
       << "ecavACC_voxels  = " << ecavACC_voxels << endl << endl;
  cerr << "echanEXC_voxels = " << echanEXC_voxels << endl
       << "chanEXC_voxels  = " << chanEXC_voxels << endl
       << "-------------------------------------" << endl
       << "cavEXC_voxels   = " << cavEXC_voxels << endl << endl << endl;
  cout << probe << "\t" << GRID << "\t" ;
  printVolCout(ecavACC_voxels);
  cout << "\t";
  printVolCout(cavEXC_voxels);
  //cout << "\t\t";
  //printVolCout(scavACC_voxels);
  //cout << "\t";
  //printVolCout(cavACC_voxels);
  cout << "\t" << natoms << "\t" << input_label;
  cout << "\tprobe,grid,cav_meth1,cav_meth2,num_atoms,file";
  cout << endl;
  
//  float perACC = 100*float(tunnACC_voxels) / float(chanACC_voxels);
//  float perEXC = 100*float(tunnEXC_voxels) / float(chanEXC_voxels);
  
  return cavACC_voxels+ecavACC_voxels;
};
