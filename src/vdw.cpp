#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>

#include "argument_helper.h"
#include "utils.h"   // for custom utility functions like assignLimits, printCompileInfo, etc.

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

int main(int argc, char *argv[]) {
  std::cerr << std::endl;

  std::string input_path;
  std::string ezd_file;
  std::string pdb_file;
  std::string mrc_file;
  double PROBE = 0.0;
  float grid = GRID;

  vossvolvox::ArgumentParser parser(
      argv[0],
      "Calculate van der Waals volume and surface area.");
  vossvolvox::add_input_option(parser, input_path);
  parser.add_option("-g",
                    "--grid",
                    grid,
                    GRID,
                    "Grid spacing in Angstroms.",
                    "<grid spacing>");
  vossvolvox::add_output_file_options(parser, pdb_file, ezd_file, mrc_file);
  parser.add_example("./VDW.exe -i 1a01.xyzr -g 0.5 -o vdw_surface.pdb");

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


//INITIALIZE GRID
  finalGridDims(PROBE);

//HEADER CHECK
  cerr << "Probe Radius: " << PROBE << endl;
  cerr << "Grid Spacing: " << GRID << endl;
  cerr << "Resolution:      " << int(1000.0/float(GRIDVOL))/1000.0 << " voxels per A^3" << endl;
  cerr << "Resolution:      " << int(11494.0/float(GRIDVOL))/1000.0 << " voxels per water molecule" << endl;
  cerr << "Input file:   " << input_path << endl;

//FIRST PASS, MINMAX
  int numatoms = read_NumAtoms(const_cast<char*>(input_path.c_str()));

//CHECK LIMITS & SIZE
  assignLimits();

// ****************************************************
// STARTING FIRST FILE
// ****************************************************
//READ FILE INTO SASGRID
  gridpt *EXCgrid;
  EXCgrid = (gridpt*) std::malloc (NUMBINS);
  if (EXCgrid==NULL) { cerr << "GRID IS NULL" << endl; exit (1); }
  zeroGrid(EXCgrid);
  int voxels;
  if(PROBE > 0.0) { 
    voxels = get_ExcludeGrid_fromFile(numatoms,PROBE,const_cast<char*>(input_path.c_str()),EXCgrid);
  } else {
    voxels = fill_AccessGrid_fromFile(numatoms,0.0,const_cast<char*>(input_path.c_str()),EXCgrid);
  }
  long double surf;
  surf = surface_area(EXCgrid);

  if(!ezd_file.empty()) {
    write_HalfEZD(EXCgrid, const_cast<char*>(ezd_file.c_str()));
  }
  if(!pdb_file.empty()) {
    write_SurfPDB(EXCgrid, const_cast<char*>(pdb_file.c_str()));
  }
  if(!mrc_file.empty()) {
    writeMRCFile(EXCgrid, const_cast<char*>(mrc_file.c_str()));
  }

//RELEASE TEMPGRID
  std::free (EXCgrid);

  cout << PROBE << "\t" << GRID << "\t" << flush;
  printVolCout(voxels);
  cout << "\t" << surf << "\t#" << input_path << endl;

  cerr << endl << "Program Completed Sucessfully" << endl << endl;
  return 0;
};
