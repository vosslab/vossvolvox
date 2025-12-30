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

int main(int argc, char *argv[]) {
  std::cerr << std::endl;
  vossvolvox::set_command_line(argc, argv);

  std::string input_path;
  vossvolvox::OutputSettings outputs;
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
  gridpt *biggrid;
  biggrid = (gridpt*) std::malloc (NUMBINS);
  if (biggrid==NULL) { cerr << "GRID IS NULL" << endl; return 1; }
  zeroGrid(biggrid);
  int bigvox;
  if(BIGPROBE > 0.0) { 
    bigvox = get_ExcludeGrid_fromArray(numatoms, BIGPROBE, xyzr_buffer, biggrid);
  } else {
    cerr << "BIGPROBE <= 0" << endl;
    return 1;
  }


// ****************************************************
// TRIM LARGE PROBE SURFACE
// ****************************************************
  gridpt *trimgrid;
  trimgrid = (gridpt*) std::malloc (NUMBINS);
  if (trimgrid==NULL) { cerr << "GRID IS NULL" << endl; return 1; }
  copyGrid(biggrid,trimgrid);
  if(TRIMPROBE > 0) {
    trun_ExcludeGrid(TRIMPROBE,biggrid,trimgrid);
  }
  std::free (biggrid);

  //cout << "bg_prb\tsm_prb\tgrid\texcvol\tsurf\taccvol\tfile" << endl;

// ****************************************************
// STARTING SMALL PROBE
// ****************************************************
    gridpt *smgrid;
    smgrid = (gridpt*) std::malloc (NUMBINS);
    if (smgrid==NULL) { cerr << "GRID IS NULL" << endl; return 1; }
    zeroGrid(smgrid);
    int smvox;
    smvox = fill_AccessGrid_fromArray(numatoms, SMPROBE, xyzr_buffer, smgrid);

// ****************************************************
// GETTING ACCESSIBLE CHANNELS
// ****************************************************
    gridpt *solventACC;
    solventACC = (gridpt*) std::malloc (NUMBINS);
    if (solventACC==NULL) { cerr << "GRID IS NULL" << endl; return 1; }
    copyGrid(trimgrid,solventACC); //copy trimgrid into solventACC
    subt_Grids(solventACC,smgrid); //modify solventACC
    std::free (smgrid);

// ***************************************************
// GETTING CONTACT CHANNEL
// ***************************************************
    gridpt *solventEXC;
    solventEXC = (gridpt*) std::malloc (NUMBINS);
    if (solventEXC==NULL) { cerr << "GRID IS NULL" << endl; return 1; }
    int solventACCvol = copyGrid(solventACC,solventEXC);
    cerr << "Accessible Channel Volume  ";
    printVol(solventACCvol);
    grow_ExcludeGrid(SMPROBE,solventACC,solventEXC);
    std::free (solventACC);

//limit growth to inside trimgrid
    intersect_Grids(solventEXC,trimgrid); //modifies solventEXC
    std::free (trimgrid);

// ***************************************************
// OUTPUT RESULTS
// ***************************************************
    cout << BIGPROBE << "\t" << SMPROBE << "\t" << GRID << "\t" << flush;
    int solventEXCvol = countGrid(solventEXC);
    printVolCout(solventEXCvol);
    long double surf = surface_area(solventEXC);
    cout << "\t" << surf << "\t" << flush;
    //printVolCout(solventACCvol);
    cout << input_path << endl;
    if(!outputs.pdbFile.empty()) {
      write_SurfPDB(solventEXC, const_cast<char*>(outputs.pdbFile.c_str()));
    }
    if(!outputs.ezdFile.empty()) {
      write_HalfEZD(solventEXC, const_cast<char*>(outputs.ezdFile.c_str()));
    }
    if(!outputs.mrcFile.empty()) {
      writeMRCFile(solventEXC, const_cast<char*>(outputs.mrcFile.c_str()));
    }

    std::free (solventEXC);

  cerr << endl << "Program Completed Sucessfully" << endl << endl;
  return 0;
};
