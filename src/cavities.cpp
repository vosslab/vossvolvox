#include <iostream>
#include <string>
#include <vector>

#include "argument_helper.hpp"
#include "pdb_io.hpp"
#include "utils.hpp"
#include "vossvolvox_cli_common.hpp"
#include "xyzr_cli_helpers.hpp"

// Globals
extern float GRID;
extern unsigned int NUMBINS;

int getCavitiesBothMeth(const float probe,
                        gridpt shellACC[],
                        gridpt shellEXC[],
                        const int natoms,
                        const XYZRBuffer& xyzr_buffer,
                        const std::string& input_label,
                        const vossvolvox::OutputSettings& outputs);

int main(int argc, char *argv[]) {
  std::cerr << std::endl;
  vossvolvox::set_command_line(argc, argv);

  std::string input_path;
  vossvolvox::OutputSettings outputs;
  vossvolvox::DebugSettings debug;
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
  vossvolvox::add_debug_option(parser, debug);
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
      static_cast<float>(shell_rad * 2),
      input_path,
      false);
  const int numatoms = grid_result.total_atoms;

// ****************************************************
// INITIALIZATION
// ****************************************************
//HEADER CHECK
  cerr << "Grid Spacing: " << GRID << endl;
  cerr << "Input file:   " << input_path << endl;
// ****************************************************
// STARTING FILE READ-IN
// ****************************************************

  auto shellACC = make_zeroed_grid();
  fill_AccessGrid_fromArray(numatoms, shell_rad, xyzr_buffer, shellACC.get());
  fill_cavities(shellACC.get());

  auto shellEXC = make_zeroed_grid();
  trun_ExcludeGrid(shell_rad, shellACC.get(), shellEXC.get());

// ****************************************************
// STARTING MAIN PROGRAM
// ****************************************************

  getCavitiesBothMeth(probe_rad,
                      shellACC.get(),
                      shellEXC.get(),
                      numatoms,
                      xyzr_buffer,
                      input_path,
                      outputs);

// ****************************************************
// CLEAN UP AND QUIT
// ****************************************************

  cerr << endl << "Program Completed Sucessfully" << endl << endl;
  return 0;
};

int getCavitiesBothMeth(const float probe,
                        gridpt shellACC[],
                        gridpt shellEXC[],
                        const int natoms,
                        const XYZRBuffer& xyzr_buffer,
                        const std::string& input_label,
                        const vossvolvox::OutputSettings& outputs)
{
/* THIS USES THE ACCESSIBLE SHELL AS THE BIG SURFACE */
/*******************************************************
Accessible Process
*******************************************************/

//Create access map
  auto access = make_zeroed_grid();
  fill_AccessGrid_fromArray(natoms, probe, xyzr_buffer, access.get());

//Create inverse access map
  auto cavACC = make_zeroed_grid();
  copyGrid(shellACC, cavACC.get());
  subt_Grids(cavACC.get(), access.get()); //modifies cavACC
  access.reset();
  int achanACC_voxels = countGrid(cavACC.get());

// EXTRA STEPS TO REMOVE SURFACE CAVITIES???

//Get first point
  bool stop = 1; int firstpt = 0;
  for(unsigned int pt=1; pt<NUMBINS && stop; pt++) {
    if (cavACC[pt]) { stop = 0; firstpt = pt;}
  }
  cerr << "FIRST POINT: " << firstpt << endl;
//LAST POINT
  stop = 1; int lastpt = NUMBINS-1;
  for(unsigned int pt=NUMBINS-1; pt>0 && stop; pt--) {
    if (cavACC[pt]) { stop = 0; lastpt = pt;}
  }
  cerr << "LAST  POINT: " << lastpt << endl;
//  get_Connected_Point(cavACC,chanACC,lastpt);


//Pull channels out of inverse access map
  auto chanACC = make_zeroed_grid();
  cerr << "Getting Connected Next" << endl;
  get_Connected_Point(cavACC.get(), chanACC.get(), firstpt); //modifies chanACC
  get_Connected_Point(cavACC.get(), chanACC.get(), lastpt); //modifies chanACC
  int chanACC_voxels = countGrid(chanACC.get());
//Subtract channels from access map leaving cavities
  subt_Grids(cavACC.get(), chanACC.get()); //modifies cavACC
  chanACC.reset();
  int cavACC_voxels = countGrid(cavACC.get());

//Grow Access Cavs
  auto ecavACC = make_zeroed_grid();
  grow_ExcludeGrid(probe, cavACC.get(), ecavACC.get());
  cavACC.reset();
  
//Intersect Grown Access Cavities with Shell
  int scavACC_voxels = countGrid(ecavACC.get());
  int ecavACC_voxels = intersect_Grids(ecavACC.get(), shellEXC); //modifies ecavACC

  //float surfEXC = surface_area(ecavACC);
  ecavACC.reset();

/*******************************************************
Excluded Process
*******************************************************/

//Create access map
  auto access2 = make_zeroed_grid();
  fill_AccessGrid_fromArray(natoms, probe, xyzr_buffer, access2.get());

//Create exclude map
  auto exclude = make_zeroed_grid();
  trun_ExcludeGrid(probe, access2.get(), exclude.get());
  access2.reset();

//Create inverse exclude map
  auto cavEXC = make_zeroed_grid();
  copyGrid(shellEXC, cavEXC.get());
  subt_Grids(cavEXC.get(), exclude.get()); //modifies cavEXC
  exclude.reset();
  int echanEXC_voxels = countGrid(cavEXC.get());

//Get first point
  stop = 1; firstpt = 0;
  for(unsigned int pt=1; pt<NUMBINS && stop; pt++) {
    if (cavEXC[pt]) { stop = 0; firstpt = pt;}
  }
  cerr << "FIRST POINT: " << firstpt << endl;
//LAST POINT
  stop = 1; lastpt = NUMBINS-1;
  for(unsigned int pt=NUMBINS-1; pt>0 && stop; pt--) {
    if (cavEXC[pt]) { stop = 0; lastpt = pt;}
  }
  cerr << "LAST  POINT: " << lastpt << endl;

//Pull channels out of inverse excluded map
  auto chanEXC = make_zeroed_grid();
  cerr << "Getting Connected Next" << endl;
  get_Connected_Point(cavEXC.get(), chanEXC.get(), firstpt); //modifies chanEXC
  get_Connected_Point(cavEXC.get(), chanEXC.get(), lastpt); //modifies chanEXC
  int chanEXC_voxels = countGrid(chanEXC.get());
//Subtract channels from exclude map leaving cavities
  subt_Grids(cavEXC.get(), chanEXC.get()); //modifies cavEXC
  chanEXC.reset();
  int cavEXC_voxels = countGrid(cavEXC.get());

//Write out exclude cavities
  long double surfEXC = surface_area(cavEXC.get());
  report_grid_metrics(std::cerr, cavEXC_voxels, surfEXC);
  write_output_files(cavEXC.get(), outputs);
  cavEXC.reset();

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
