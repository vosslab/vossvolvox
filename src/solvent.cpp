#include <cstdio>
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
  std::string ezd_file;
  std::string pdb_file;
  std::string mrc_file;
  double BIGPROBE = 9.0;
  double SMPROBE = 1.5;
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
  vossvolvox::add_output_file_options(parser, pdb_file, ezd_file, mrc_file);
  vossvolvox::add_xyzr_filter_flags(parser,
                                    use_hydrogens,
                                    exclude_ions,
                                    exclude_ligands,
                                    exclude_hetatm,
                                    exclude_water,
                                    exclude_nucleic,
                                    exclude_amino);
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
  if(SMPROBE > BIGPROBE) { cerr << "ERROR: SMPROBE > BIGPROBE" << endl; return 1; }
  cerr << "Small Probe Radius: " << SMPROBE << endl;
  cerr << " Big  Probe Radius: " << BIGPROBE << endl;
  cerr << "Trim  Probe Radius: " << TRIMPROBE << endl;
  cerr << "Grid Spacing: " << GRID << endl;
  cerr << "Input file:   " << input_path << endl;
  cerr << "Resolution:   " << int(1000.0/float(GRIDVOL))/1000.0 << " voxels per A^3" << endl;
  cerr << "Resolution:   " << int(11494.0/float(GRIDVOL))/1000.0 << " voxels per water molecule" << endl;


//FIRST PASS, MINMAX
  int numatoms = read_NumAtoms_from_array(xyzr_buffer);

//CHECK LIMITS & SIZE
  assignLimits();

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
    if(!pdb_file.empty()) {
      write_SurfPDB(solventEXC, const_cast<char*>(pdb_file.c_str()));
    }
    if(!ezd_file.empty()) {
      write_HalfEZD(solventEXC, const_cast<char*>(ezd_file.c_str()));
    }
    if(!mrc_file.empty()) {
      writeMRCFile(solventEXC, const_cast<char*>(mrc_file.c_str()));
    }

    std::free (solventEXC);

  cerr << endl << "Program Completed Sucessfully" << endl << endl;
  return 0;
};
