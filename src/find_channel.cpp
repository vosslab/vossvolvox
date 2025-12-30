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
  vossvolvox::add_output_options(parser, outputs);
  vossvolvox::add_filter_options(parser, filters);
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
  cerr << "Resolution:      " << int(1000.0/float(GRIDVOL))/1000.0 << " voxels per A^3" << endl;
  cerr << "Resolution:      " << int(11494.0/float(GRIDVOL))/1000.0 << " voxels per water molecule" << endl;
  cerr << "Input file:   " << input_path << endl;

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
  trun_ExcludeGrid(TRIMPROBE,biggrid,trimgrid);
  std::free (biggrid);

  cout << "bg_prb\tsm_prb\tgrid\texcvol\tsurf\taccvol\tfile" << endl;

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
// SELECT PARTICULAR CHANNEL
// ***************************************************
    gridpt *channelACC;
    channelACC = (gridpt*) std::malloc (NUMBINS);
    if (channelACC==NULL) { cerr << "GRID IS NULL" << endl; return 1; }
    zeroGrid(channelACC);

//main channel
    get_Connected(solventACC,channelACC, x, y, z);
    std::free (solventACC);

// ***************************************************
// GETTING CONTACT CHANNEL
// ***************************************************
    gridpt *channelEXC;
    channelEXC = (gridpt*) std::malloc (NUMBINS);
    if (channelEXC==NULL) { cerr << "GRID IS NULL" << endl; return 1; }
    int channelACCvol = copyGrid(channelACC,channelEXC);
    cerr << "Accessible Channel Volume  ";
    printVol(channelACCvol);
    grow_ExcludeGrid(SMPROBE,channelACC,channelEXC);
    std::free (channelACC);

//limit growth to inside trimgrid
    intersect_Grids(channelEXC,trimgrid); //modifies channelEXC
    //std::free (trimgrid);

// ***************************************************
// OUTPUT RESULTS
// ***************************************************
    cout << BIGPROBE << "\t" << SMPROBE << "\t" << GRID << "\t" << flush;
    int channelEXCvol = countGrid(channelEXC);
    printVolCout(channelEXCvol);
    long double surf = surface_area(channelEXC);
    cout << "\t" << surf << "\t" << flush;
    printVolCout(channelACCvol);
    cout << "\t#" << input_path << endl;
    if(!outputs.pdbFile.empty()) {
      write_SurfPDB(channelEXC, const_cast<char*>(outputs.pdbFile.c_str()));
    }
    if(!outputs.ezdFile.empty()) {
      write_HalfEZD(channelEXC, const_cast<char*>(outputs.ezdFile.c_str()));
    }
    if(!outputs.mrcFile.empty()) {
      writeSmallMRCFile(channelEXC, const_cast<char*>(outputs.mrcFile.c_str()));
    }

//RELEASE TEMPGRID
    std::free (channelEXC);
    cerr << endl;

  std::free (trimgrid);
  cerr << endl << "Program Completed Sucessfully" << endl << endl;
  return 0;
};
