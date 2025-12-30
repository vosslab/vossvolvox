#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

#include "argument_helper.h"
#include "pdb_io.h"
#include "utils.h"

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
  double BIGPROBE = 10.0;
  double PROBESTEP = 0.1;
  double TRIMPROBE = 1.5;
  float grid = GRID;
  bool use_hydrogens = false;
  bool exclude_ions = false;
  bool exclude_ligands = false;
  bool exclude_hetatm = false;
  bool exclude_water = false;
  bool exclude_nucleic = false;
  bool exclude_amino = false;

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
  vossvolvox::add_xyzr_filter_flags(parser,
                                    use_hydrogens,
                                    exclude_ions,
                                    exclude_ligands,
                                    exclude_hetatm,
                                    exclude_water,
                                    exclude_nucleic,
                                    exclude_amino);
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

  vossvolvox::pdbio::ConversionOptions convert_options;
  convert_options.use_united = !use_hydrogens;
  convert_options.filters.exclude_ions = exclude_ions;
  convert_options.filters.exclude_ligands = exclude_ligands;
  convert_options.filters.exclude_hetatm = exclude_hetatm;
  convert_options.filters.exclude_water = exclude_water;
  convert_options.filters.exclude_nucleic_acids = exclude_nucleic;
  convert_options.filters.exclude_amino_acids = exclude_amino;
  vossvolvox::pdbio::XyzrData xyzr_data;
  if (!vossvolvox::pdbio::ReadFileToXyzr(input_path, convert_options, xyzr_data)) {
    std::cerr << "Error: unable to load XYZR data from '" << input_path << "'\n";
    return 1;
  }
  XYZRBuffer xyzr_buffer;
  xyzr_buffer.atoms.reserve(xyzr_data.atoms.size());
  for (const auto& atom : xyzr_data.atoms) {
    xyzr_buffer.atoms.push_back(
        XYZRAtom{static_cast<float>(atom.x),
                 static_cast<float>(atom.y),
                 static_cast<float>(atom.z),
                 static_cast<float>(atom.radius)});
  }
  if (!XYZRFILE[0]) {
    std::strncpy(XYZRFILE, input_path.c_str(), sizeof(XYZRFILE));
    XYZRFILE[sizeof(XYZRFILE) - 1] = '\0';
  }


//INITIALIZE GRID
  finalGridDims(BIGPROBE);

//HEADER CHECK
  cerr << "Grid Spacing: " << GRID << endl;
  cerr << "Resolution:      " << int(1000.0/float(GRIDVOL))/1000.0 << " voxels per A^3" << endl;
  cerr << "Resolution:      " << int(11494.0/float(GRIDVOL))/1000.0 << " voxels per water molecule" << endl;
  cerr << "Complexity:      " << int(8000000/float(GRIDVOL))/1000.0 << endl;
  cerr << "Input file:   " << input_path << endl;

//FIRST PASS, MINMAX
  int numatoms = read_NumAtoms_from_array(xyzr_buffer);

//CHECK LIMITS & SIZE
  assignLimits();

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
