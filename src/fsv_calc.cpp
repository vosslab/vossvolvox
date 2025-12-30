#include <iostream>
#include <string>
#include <vector>

#include "argument_helper.hpp"
#include "pdb_io.hpp"
#include "utils.hpp"
#include "vossvolvox_cli_common.hpp"
#include "xyzr_cli_helpers.hpp"

// Globals
extern float GRID, GRIDVOL;

int main(int argc, char *argv[]) {
  std::cerr << std::endl;
  vossvolvox::set_command_line(argc, argv);

  std::string input_path;
  double BIGPROBE = 10.0;
  double PROBESTEP = 0.1;
  double TRIMPROBE = 1.5;
  float grid = GRID;
  vossvolvox::FilterSettings filters;
  vossvolvox::DebugSettings debug;

  vossvolvox::ArgumentParser parser(
      argv[0],
      "Calculate fractional solvent volume as the probe radius varies.");
  vossvolvox::add_input_option(parser, input_path);
  parser.add_option("-b",
                    "--big-probe",
                    BIGPROBE,
                    10.0,
                    "Maximum probe radius in Angstroms.",
                    "<big probe>");
  parser.add_option("-s",
                    "--probe-step",
                    PROBESTEP,
                    0.1,
                    "Probe radius increment in Angstroms.",
                    "<step>");
  parser.add_option("-t",
                    "--trim-probe",
                    TRIMPROBE,
                    1.5,
                    "Trim radius applied to the shell (Angstroms).",
                    "<trim>");
  parser.add_option("-g",
                    "--grid",
                    grid,
                    GRID,
                    "Grid spacing in Angstroms.",
                    "<grid>");
  vossvolvox::add_filter_options(parser, filters);
  vossvolvox::add_debug_option(parser, debug);
  parser.add_example("./FsvCalc.exe -i sample.xyzr -b 10 -s 0.25 -t 1.5 -g 0.8");

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
  vossvolvox::debug_report_cli(input_path, nullptr);

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
  cerr << "Grid Spacing: " << GRID << endl;
  cerr << "Resolution:      " << int(1000.0/float(GRIDVOL))/1000.0 << " voxels per A^3" << endl;
  cerr << "Resolution:      " << int(11494.0/float(GRIDVOL))/1000.0 << " voxels per water molecule" << endl;
  cerr << "Complexity:      " << int(8000000/float(GRIDVOL))/1000.0 << endl;
  cerr << "Input file:   " << input_path << endl;

// ****************************************************
// STARTING FIRST FILE
// ****************************************************

//GET SHELL
  auto shell = make_zeroed_grid();
  int shellvol = get_ExcludeGrid_fromArray(numatoms, BIGPROBE, xyzr_buffer, shell.get());

//INIT NEW smShellACC GRID
  cerr << "Trimming Radius: " << TRIMPROBE << endl;
  auto smShell = make_zeroed_grid();

//COPY AND TRUNCATE (IF NECESSARY)
  copyGrid(shell.get(), smShell.get());
  if(TRIMPROBE > 0) {
    trun_ExcludeGrid(TRIMPROBE, shell.get(), smShell.get());
  }
  shell.reset();

// ****************************************************
// STARTING LOOP OVER PROBE SIZE
// ****************************************************

  cout << "probe\tshell_vol\tsolvent_vol\tfsv\tfile" << endl;

  for (double SMPROBE=0.0; SMPROBE<BIGPROBE; SMPROBE+=PROBESTEP) {
	//COPY SMSHELL INTO CHANACC
	  auto solventACC = make_zeroed_grid();
	  int smshellvol = copyGrid(smShell.get(), solventACC.get());

	//SUBTRACT PROBE_ACC FROM SHELL TO GET ACC CHANNELS
	  auto probeACC = make_zeroed_grid();
	  fill_AccessGrid_fromArray(numatoms, SMPROBE, xyzr_buffer, probeACC.get());
	  subt_Grids(solventACC.get(), probeACC.get());

	//INIT NEW solventEXC GRID
	  auto solventEXC = make_zeroed_grid();

	//GROW EXCLUDED SURFACE FROM ACCESSIBLE
	  //copyGrid(solventACC,solventEXC);
	  grow_ExcludeGrid(SMPROBE, solventACC.get(), solventEXC.get());

	//INTERSECT
	  intersect_Grids(solventEXC.get(), smShell.get()); //modifies solventEXC

	//OUTPUT
	  int solventvol = countGrid(solventEXC.get());
	  cout << SMPROBE << "\t";
	  printVolCout(shellvol);
	  printVolCout(solventvol);
	  double fsv = double(solventvol) / double(shellvol);
	  cout << fsv << "\t" << input_path << endl;

    /*
	  if(pdbfile[0] != '\0') {
		write_SurfPDB(solventEXC, pdbfile);
	  }
	  if(ezdfile[0] != '\0') {
		write_HalfEZD(solventEXC, ezdfile);
	  }
	  if(mrcfile[0] != '\0') {
		writeMRCFile(solventEXC, mrcfile);
	  }
	*/
  }
  smShell.reset();

  cerr << endl << "Program Completed Sucessfully" << endl << endl;
  return 0;
};
