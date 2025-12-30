#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include "argument_helper.h"
#include "pdb_io.h"
#include "utils.h"                    // for endl, cerr, countGrid, gridpt
#include "vossvolvox_cli_common.hpp"
#include "xyzr_cli_helpers.h"

// ****************************************************
// CALCULATE EXCLUDED VOLUME, BUT FILL ANY CAVITIES
// designed for use with 3d printer
// ****************************************************

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
                        char ezdfile[],
                        char pdbfile[],
                        char mrcfile[]);

int main(int argc, char *argv[]) {
  std::cerr << std::endl;
  vossvolvox::set_command_line(argc, argv);

  std::string input_path;
  vossvolvox::OutputSettings outputs;
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
  cerr << "Resolution:      " << int(1000.0/float(GRIDVOL))/1000.0 << " voxels per A^3" << endl;
  cerr << "Resolution:      " << int(11494.0/float(GRIDVOL))/1000.0 << " voxels per water molecule" << endl;
  cerr << "Input file:   " << input_path << endl;
// ****************************************************
// STARTING FILE READ-IN
// ****************************************************


  gridpt *shellACC=NULL;
  shellACC = (gridpt*) std::malloc (NUMBINS);
  fill_AccessGrid_fromArray(numatoms, PROBE, xyzr_buffer, shellACC);
  int voxels1 = countGrid(shellACC);
  fill_cavities(shellACC);
  int voxels2 = countGrid(shellACC);
  cerr << "Fill Cavities: " << voxels2 - voxels1 << " voxels filled" << endl;

  gridpt *EXCgrid=NULL;
  EXCgrid = (gridpt*) std::malloc (NUMBINS);
  trun_ExcludeGrid(PROBE,shellACC,EXCgrid);

  std::free (shellACC);
  int voxels = countGrid(EXCgrid);



  long double surf;
  surf = surface_area(EXCgrid);

  if(!outputs.mrcFile.empty()) {
    writeMRCFile(EXCgrid, const_cast<char*>(outputs.mrcFile.c_str()));
  }
  if(!outputs.ezdFile.empty()) {
    write_HalfEZD(EXCgrid,const_cast<char*>(outputs.ezdFile.c_str()));
  }
  if(!outputs.pdbFile.empty()) {
    write_SurfPDB(EXCgrid,const_cast<char*>(outputs.pdbFile.c_str()));
  }

//RELEASE TEMPGRID
  std::free (EXCgrid);

  cout << PROBE << "\t" << GRID << "\t" << flush;
  printVolCout(voxels);
  cout << "\t" << surf << "\t#" << input_path << endl;

  cerr << endl << "Program Completed Sucessfully" << endl << endl;
  return 0;



};
