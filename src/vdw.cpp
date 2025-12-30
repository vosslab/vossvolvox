#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

#include "argument_helper.h"
#include "pdb_io.h"
#include "utils.h"   // for custom utility functions like assignLimits, printCompileInfo, etc.

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
  std::string ezd_file;
  std::string pdb_file;
  std::string mrc_file;
  double PROBE = 0.0;
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
      "Calculate van der Waals volume and surface area.");
  vossvolvox::add_input_option(parser, input_path);
  parser.add_option("-g",
                    "--grid",
                    grid,
                    GRID,
                    "Grid spacing in Angstroms.",
                    "<grid spacing>");
  vossvolvox::add_output_file_options(parser, pdb_file, ezd_file, mrc_file);
  vossvolvox::add_xyzr_filter_flags(parser,
                                    use_hydrogens,
                                    exclude_ions,
                                    exclude_ligands,
                                    exclude_hetatm,
                                    exclude_water,
                                    exclude_nucleic,
                                    exclude_amino);
  parser.add_example("./VDW.exe -i 1a01.xyzr -g 0.5 -o vdw_surface.pdb");

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
  finalGridDims(PROBE);

//HEADER CHECK
  cerr << "Probe Radius: " << PROBE << endl;
  cerr << "Grid Spacing: " << GRID << endl;
  cerr << "Resolution:      " << int(1000.0/float(GRIDVOL))/1000.0 << " voxels per A^3" << endl;
  cerr << "Resolution:      " << int(11494.0/float(GRIDVOL))/1000.0 << " voxels per water molecule" << endl;
  cerr << "Input file:   " << input_path << endl;

//FIRST PASS, MINMAX
  int numatoms = read_NumAtoms_from_array(xyzr_buffer);

//CHECK LIMITS & SIZE
  assignLimits();

// ****************************************************
// STARTING FIRST FILE
// ****************************************************
//READ FILE INTO SASGRID
  gridpt *EXCgrid;
  EXCgrid = (gridpt*) std::malloc (NUMBINS);
  if (EXCgrid==NULL) { cerr << "GRID IS NULL" << endl; exit (1); }
  zeroGrid(EXCgrid);
  int voxels;
  if(PROBE > 0.0) { 
    voxels = get_ExcludeGrid_fromArray(numatoms, PROBE, xyzr_buffer, EXCgrid);
  } else {
    voxels = fill_AccessGrid_fromArray(numatoms, 0.0f, xyzr_buffer, EXCgrid);
  }
  long double surf;
  surf = surface_area(EXCgrid);

  if(!ezd_file.empty()) {
    write_HalfEZD(EXCgrid, const_cast<char*>(ezd_file.c_str()));
  }
  if(!pdb_file.empty()) {
    write_SurfPDB(EXCgrid, const_cast<char*>(pdb_file.c_str()));
  }
  if(!mrc_file.empty()) {
    writeMRCFile(EXCgrid, const_cast<char*>(mrc_file.c_str()));
  }

//RELEASE TEMPGRID
  std::free (EXCgrid);

  cout << PROBE << "\t" << GRID << "\t" << flush;
  printVolCout(voxels);
  cout << "\t" << surf << "\t#" << input_path << endl;

  cerr << endl << "Program Completed Sucessfully" << endl << endl;
  return 0;
};
