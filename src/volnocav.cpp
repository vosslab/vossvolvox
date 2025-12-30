#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

#include "argument_helper.h"
#include "pdb_io.h"
#include "utils.h"                    // for endl, cerr, countGrid, gridpt

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

  std::string input_path;
  std::string ezd_file;
  std::string pdb_file;
  std::string mrc_file;
  double PROBE = 1.5;
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
  vossvolvox::add_output_file_options(parser, pdb_file, ezd_file, mrc_file);
  vossvolvox::add_xyzr_filter_flags(parser,
                                    use_hydrogens,
                                    exclude_ions,
                                    exclude_ligands,
                                    exclude_hetatm,
                                    exclude_water,
                                    exclude_nucleic,
                                    exclude_amino);
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

// ****************************************************
// INITIALIZATION
// ****************************************************

//INITIALIZE GRID
  finalGridDims(PROBE*2);
//HEADER CHECK
  cerr << "Grid Spacing: " << GRID << endl;
  cerr << "Resolution:      " << int(1000.0/float(GRIDVOL))/1000.0 << " voxels per A^3" << endl;
  cerr << "Resolution:      " << int(11494.0/float(GRIDVOL))/1000.0 << " voxels per water molecule" << endl;
  cerr << "Input file:   " << input_path << endl;
//FIRST PASS, MINMAX
  int numatoms = read_NumAtoms_from_array(xyzr_buffer);
//CHECK LIMITS & SIZE
  assignLimits();

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

  if(!mrc_file.empty()) {
    writeMRCFile(EXCgrid, const_cast<char*>(mrc_file.c_str()));
  }
  if(!ezd_file.empty()) {
    write_HalfEZD(EXCgrid,const_cast<char*>(ezd_file.c_str()));
  }
  if(!pdb_file.empty()) {
    write_SurfPDB(EXCgrid,const_cast<char*>(pdb_file.c_str()));
  }

//RELEASE TEMPGRID
  std::free (EXCgrid);

  cout << PROBE << "\t" << GRID << "\t" << flush;
  printVolCout(voxels);
  cout << "\t" << surf << "\t#" << input_path << endl;

  cerr << endl << "Program Completed Sucessfully" << endl << endl;
  return 0;



};
