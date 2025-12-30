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
  double BIGPROBE = 10.0;
  double PROBESTEP = 0.1;
  double TRIMPROBE = 1.5;
  float grid = GRID;
  vossvolvox::FilterSettings filters;

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
  gridpt *shell;
  shell = (gridpt*) std::malloc (NUMBINS);
  if (shell==NULL) { cerr << "GRID IS NULL" << endl; exit (1); }
  zeroGrid(shell);
  int shellvol = get_ExcludeGrid_fromArray(numatoms, BIGPROBE, xyzr_buffer, shell);

//INIT NEW smShellACC GRID
  cerr << "Trimming Radius: " << TRIMPROBE << endl;
  gridpt *smShell;
  smShell = (gridpt*) std::malloc (NUMBINS);
  if (smShell==NULL) { cerr << "GRID IS NULL" << endl; exit (1); }

//COPY AND TRUNCATE (IF NECESSARY)
  copyGrid(shell,smShell);
  if(TRIMPROBE > 0) {
    trun_ExcludeGrid(TRIMPROBE,shell,smShell);
  }
  std::free (shell);

// ****************************************************
// STARTING LOOP OVER PROBE SIZE
// ****************************************************

  cout << "probe\tshell_vol\tsolvent_vol\tfsv\tfile" << endl;

  for (double SMPROBE=0.0; SMPROBE<BIGPROBE; SMPROBE+=PROBESTEP) {
	//COPY SMSHELL INTO CHANACC
	  gridpt *solventACC;
	  solventACC = (gridpt*) std::malloc (NUMBINS);
	  if (solventACC==NULL) { cerr << "GRID IS NULL" << endl; exit (1); }
	  int smshellvol = copyGrid(smShell,solventACC);

	//SUBTRACT PROBE_ACC FROM SHELL TO GET ACC CHANNELS
	  gridpt *probeACC;
	  probeACC = (gridpt*) std::malloc (NUMBINS);
	  if (probeACC==NULL) { cerr << "GRID IS NULL" << endl; exit (1); }
	  zeroGrid(probeACC);
	  fill_AccessGrid_fromArray(numatoms, SMPROBE, xyzr_buffer, probeACC);
	  subt_Grids(solventACC, probeACC);
	  std::free (probeACC);

	//INIT NEW solventEXC GRID
	  gridpt *solventEXC;
	  solventEXC = (gridpt*) std::malloc (NUMBINS);
	  if (solventEXC==NULL) { cerr << "GRID IS NULL" << endl; exit (1); }

	//GROW EXCLUDED SURFACE FROM ACCESSIBLE
	  //copyGrid(solventACC,solventEXC);
	  zeroGrid(solventEXC);
	  grow_ExcludeGrid(SMPROBE, solventACC, solventEXC);
	  std::free (solventACC);

	//INTERSECT
	  intersect_Grids(solventEXC, smShell); //modifies solventEXC

	//OUTPUT
	  int solventvol = countGrid(solventEXC);
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
	  std::free (solventEXC);

  }
  std::free (smShell);

  cerr << endl << "Program Completed Sucessfully" << endl << endl;
  return 0;
};
