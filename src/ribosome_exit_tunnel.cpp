#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include "argument_helper.h"
#include "pdb_io.h"
#include "utils.h"                    // for get_Connected, endl, gridpt, cerr
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

void printTun(const float probe, 
	const float surfEXC, const int tunnEXC_voxels, const int chanEXC_voxels,
	const float surfACC, const int tunnACC_voxels, const int chanACC_voxels,
	char file[]);
void defineTunnel(gridpt tunnel[], gridpt channels[]);

int main(int argc, char *argv[]) {
  std::cerr << std::endl;
  vossvolvox::set_command_line(argc, argv);

  std::string input_path;
  vossvolvox::OutputSettings outputs;
  vossvolvox::DebugSettings debug;
  double shell_rad = 10.0;
  double tunnel_prb = 3.0;
  double trim_prb = 3.0;
  float grid = GRID;
  vossvolvox::FilterSettings filters;

  vossvolvox::ArgumentParser parser(
      argv[0],
      "Extract the ribosomal exit tunnel from a structure.");
  vossvolvox::add_input_option(parser, input_path);
  parser.add_option("-b",
                    "--shell-radius",
                    shell_rad,
                    10.0,
                    "Shell (big probe) radius in Angstroms.",
                    "<shell radius>");
  parser.add_option("-s",
                    "--tunnel-probe",
                    tunnel_prb,
                    3.0,
                    "Small tunnel probe radius in Angstroms.",
                    "<probe>");
  parser.add_option("-t",
                    "--trim-radius",
                    trim_prb,
                    3.0,
                    "Trim radius applied to the shell (Angstroms).",
                    "<trim radius>");
  parser.add_option("-g",
                    "--grid",
                    grid,
                    GRID,
                    "Grid spacing in Angstroms.",
                    "<grid>");
  outputs.use_small_mrc = true;
  vossvolvox::add_output_options(parser, outputs);
  vossvolvox::add_filter_options(parser, filters);
  vossvolvox::add_debug_option(parser, debug);
  parser.add_example("./Tunnel.exe -i 1jj2.xyzr -b 12 -s 3 -t 4 -g 0.6 -o tunnel.pdb");

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

  const auto convert_options = vossvolvox::make_conversion_options(filters);
  XYZRBuffer xyzr_buffer;
  if (!vossvolvox::load_xyzr_or_exit(input_path, convert_options, xyzr_buffer)) {
    return 1;
  }
  const std::vector<const XYZRBuffer*> buffers = {&xyzr_buffer};
  const auto grid_result = vossvolvox::prepare_grid_from_xyzr(
      buffers,
      grid,
      static_cast<float>(shell_rad),
      input_path,
      false);
  const int numatoms = grid_result.total_atoms;

//HEADER CHECK
  cerr << "Grid Spacing: " << GRID << endl;
  cerr << "Resolution:      " << int(1000.0/float(GRIDVOL))/1000.0 << " voxels per A^3" << endl;
  cerr << "Resolution:      " << int(11494.0/float(GRIDVOL))/1000.0 << " voxels per water molecule" << endl;
  cerr << "Input file:   " << input_path << endl;

// ****************************************************
// BUSINESS PART
// ****************************************************

//Compute Shell
  gridpt *shellACC=NULL;
  shellACC = (gridpt*) std::malloc (NUMBINS);
  fill_AccessGrid_fromArray(numatoms, shell_rad, xyzr_buffer, shellACC);
  fill_cavities(shellACC);

  gridpt *shellEXC=NULL;
  shellEXC = (gridpt*) std::malloc (NUMBINS);
  trun_ExcludeGrid(shell_rad,shellACC,shellEXC);
  std::free (shellACC);

//Trim Shell
  if(trim_prb > 0.0) {
    gridpt *trimEXC;
    trimEXC = (gridpt*) std::malloc (NUMBINS);
    copyGrid(shellEXC,trimEXC);
    trun_ExcludeGrid(trim_prb,shellEXC,trimEXC);  // TRIMMING PART
    zeroGrid(shellEXC);
    copyGrid(trimEXC,shellEXC);
    std::free (trimEXC);
  }

//Get Shell Volume
  int shell_vol = countGrid(shellEXC);
  printVol(shell_vol); cerr << endl;

//Get Access Volume for "probe"
  gridpt *access;
  access = (gridpt*) std::malloc (NUMBINS);
  fill_AccessGrid_fromArray(numatoms, tunnel_prb, xyzr_buffer, access);

//Get Channels for "probe"
  gridpt *chanACC;
  chanACC = (gridpt*) std::malloc (NUMBINS);
  copyGrid(shellEXC,chanACC);
  subt_Grids(chanACC,access);
  std::free (access);
  intersect_Grids(chanACC,shellEXC); //modifies chanACC
  int chanACC_voxels = countGrid(chanACC);
  printVol(chanACC_voxels); cerr << endl;

//Extract Tunnel
  gridpt *tunnACC;
  tunnACC = (gridpt*) std::malloc (NUMBINS);
  defineTunnel(tunnACC, chanACC);
  //writeMRCFile(chanACC, mrcfile);
  std::free (chanACC);
  int tunnACC_voxels = countGrid(tunnACC);
  cerr << "ACCESSIBLE TUNNEL VOLUME: ";
  printVol(tunnACC_voxels); cerr << endl << endl;
  //float surfACC = surface_area(tunnACC);
  float surfACC = 0;
  if (tunnACC_voxels*GRIDVOL > 2000000) {
    cerr << "ERROR: Accessible volume of tunnel is too large to be valid" << endl;
    std::free (shellEXC);
    //writeMRCFile(chanACC, mrcfile);
    std::free (tunnACC);
    return 0;
  }

//Grow Tunnel
  gridpt *tunnEXC;
  tunnEXC = (gridpt*) std::malloc (NUMBINS);
  grow_ExcludeGrid(tunnel_prb,tunnACC,tunnEXC);
  std::free (tunnACC);

//Intersect Grown Tunnel with Shell
  intersect_Grids(tunnEXC,shellEXC); //modifies tunnEXC
  std::free (shellEXC);

//Get EXC Props
  int tunnEXC_voxels = countGrid(tunnEXC);
  cerr << "TUNNEL VOLUME: ";
  printVol(tunnEXC_voxels); cerr << endl << endl;
  //shell volume at 6A probe 9732148*0.6^3 = 2,102,144 A^3
  //solvent volume at 1.4A probe 2465252*0.6^3 = 532,494 A^3
  if (tunnEXC_voxels*GRIDVOL > 1800000) {
    cerr << "ERROR: Excluded volume of tunnel is too large to be valid" << endl;
    std::free (tunnEXC);
    return 0;
  }
  float surfEXC = surface_area(tunnEXC);

//Output
  write_output_files(tunnEXC, outputs);

  std::free (tunnEXC);

  printTun(trim_prb,
           surfEXC,
           tunnEXC_voxels,
           0,
           surfACC,
           tunnACC_voxels,
           chanACC_voxels,
           const_cast<char*>(input_path.c_str()));

  cerr << endl << "Program Completed Sucessfully" << endl << endl;
  return 0;
};

//***********************************************************//
//***********************************************************//
//***********************************************************//

void defineTunnel(gridpt tunnel[], gridpt channels[])
{
//NEW IDEAL TUNNEL POINTS
  zeroGrid(tunnel);
//  get_Connected(chanACC,tunnACC,77.2,116.0,109.2); //tRNA cleft
  get_Connected(channels, tunnel, 74.8,130.0,83.6); //highest tunnel pt
  get_Connected(channels, tunnel, 68.3,132.2,85.6); //largest area
  get_Connected(channels, tunnel, 53.6,144.8,69.6); //below main
  get_Connected(channels, tunnel, 49.9,151.8,67.3); //2nd largest & low 
  get_Connected(channels, tunnel, 38.4,160.4,63.6); //low blob point
  get_Connected(channels, tunnel, 35.6,163.6,61.6); //lowest pt
//OLD POINTS ; CAN'T HURT
  get_Connected(channels, tunnel, 53.6,141.3,66.4);
  get_Connected(channels, tunnel, 71.5,120.4,97.3);
  get_Connected(channels, tunnel, 71.5,125.0,98.1);
  get_Connected(channels, tunnel, 70.3,131.2,81.9);
  get_Connected(channels, tunnel, 55.7,140.2,73.8);
  get_Connected(channels, tunnel, 44.6,153.2,68.7);

  //get_Connected(channels,tunnel,0.0,0.0,0.0);
  return;
};

//***********************************************************//
//***********************************************************//
//***********************************************************//

void printTun(const float probe, 
	const float surfEXC, const int tunnEXC_voxels, const int chanEXC_voxels,
	const float surfACC, const int tunnACC_voxels, const int chanACC_voxels,
        char file[])
{
    float perACC = 100*float(tunnACC_voxels) / (float(chanACC_voxels) + 0.01);
    float perEXC = 100*float(tunnEXC_voxels) / (float(chanEXC_voxels) + 0.01);

    cout << probe << "\t";
//EXCLUDE
    printVolCout(tunnEXC_voxels);
    printVolCout(chanEXC_voxels);
    cout << perEXC << "\t";
    cout << surfEXC << "\t";
//ACCESS
    printVolCout(tunnACC_voxels);
    printVolCout(chanACC_voxels);
    cout << perACC << "\t";
    cout << surfACC << "\t";
    cout << GRID << endl;

    return;
};
