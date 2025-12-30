#include <iostream>
#include <string>
#include <vector>

#include "argument_helper.hpp"
#include "pdb_io.hpp"
#include "utils.hpp"                    // for get_Connected, endl, gridpt, cerr
#include "vossvolvox_cli_common.hpp"
#include "xyzr_cli_helpers.hpp"

// Globals
extern float GRID, GRIDVOL;

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
  cerr << "Input file:   " << input_path << endl;

// ****************************************************
// BUSINESS PART
// ****************************************************

//Compute Shell
  auto shellACC = make_zeroed_grid();
  fill_AccessGrid_fromArray(numatoms, shell_rad, xyzr_buffer, shellACC.get());
  fill_cavities(shellACC.get());

  auto shellEXC = make_zeroed_grid();
  trun_ExcludeGrid(shell_rad, shellACC.get(), shellEXC.get());
  shellACC.reset();

//Trim Shell
  if(trim_prb > 0.0) {
    auto trimEXC = make_zeroed_grid();
    copyGrid(shellEXC.get(), trimEXC.get());
    trun_ExcludeGrid(trim_prb, shellEXC.get(), trimEXC.get());  // TRIMMING PART
    zeroGrid(shellEXC.get());
    copyGrid(trimEXC.get(), shellEXC.get());
  }

//Get Shell Volume
  int shell_vol = countGrid(shellEXC.get());
  printVol(shell_vol); cerr << endl;

//Get Access Volume for "probe"
  auto access = make_zeroed_grid();
  fill_AccessGrid_fromArray(numatoms, tunnel_prb, xyzr_buffer, access.get());

//Get Channels for "probe"
  auto chanACC = make_zeroed_grid();
  copyGrid(shellEXC.get(), chanACC.get());
  subt_Grids(chanACC.get(), access.get());
  access.reset();
  intersect_Grids(chanACC.get(), shellEXC.get()); //modifies chanACC
  int chanACC_voxels = countGrid(chanACC.get());
  printVol(chanACC_voxels); cerr << endl;

//Extract Tunnel
  auto tunnACC = make_zeroed_grid();
  defineTunnel(tunnACC.get(), chanACC.get());
  //writeMRCFile(chanACC, mrcfile);
  chanACC.reset();
  int tunnACC_voxels = countGrid(tunnACC.get());
  cerr << "ACCESSIBLE TUNNEL VOLUME: ";
  printVol(tunnACC_voxels); cerr << endl << endl;
  //float surfACC = surface_area(tunnACC);
  float surfACC = 0;
  if (tunnACC_voxels*GRIDVOL > 2000000) {
    cerr << "ERROR: Accessible volume of tunnel is too large to be valid" << endl;
    shellEXC.reset();
    //writeMRCFile(chanACC, mrcfile);
    return 0;
  }

//Grow Tunnel
  auto tunnEXC = make_zeroed_grid();
  grow_ExcludeGrid(tunnel_prb, tunnACC.get(), tunnEXC.get());
  tunnACC.reset();

//Intersect Grown Tunnel with Shell
  intersect_Grids(tunnEXC.get(), shellEXC.get()); //modifies tunnEXC
  shellEXC.reset();

//Get EXC Props
  int tunnEXC_voxels = countGrid(tunnEXC.get());
  cerr << "TUNNEL VOLUME: ";
  printVol(tunnEXC_voxels); cerr << endl << endl;
  //shell volume at 6A probe 9732148*0.6^3 = 2,102,144 A^3
  //solvent volume at 1.4A probe 2465252*0.6^3 = 532,494 A^3
  if (tunnEXC_voxels*GRIDVOL > 1800000) {
    cerr << "ERROR: Excluded volume of tunnel is too large to be valid" << endl;
    return 0;
  }
  float surfEXC = surface_area(tunnEXC.get());

//Output
  report_grid_metrics(std::cerr, tunnEXC_voxels, static_cast<long double>(surfEXC));
  write_output_files(tunnEXC.get(), outputs);

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
