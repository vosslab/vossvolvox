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


/*********************************************/
int trimYAxis (gridpt grid[]) {
  int voxels=0;
  int error=0;
  if (DEBUG > 0)
    cerr << "Trimming Y Axis from Grids...  " << flush;
	
  float x,y,z;
  for(unsigned int pt=0; pt<NUMBINS; pt++) {
    if (grid[pt]) {
      pt2xyz(pt, x, y, z);
      if(y > 170) {
        grid[pt] = 0;
      }
    }
  }
  if (DEBUG > 0) {
    cerr << "done [ " << voxels << " vox changed ]" << endl;
  //cerr << "done [ " << error << " errors : " <<
//	int(1000.0*error/(voxels+error))/10.0 << "% ]" << endl;
    cerr << endl;
  }
  return voxels;
};


int main(int argc, char *argv[]) {
  std::cerr << std::endl;
  vossvolvox::set_command_line(argc, argv);

  std::string rna_file;
  std::string amino_file;
  double PROBE = 10.0;
  float grid = GRID;
  vossvolvox::FilterSettings filters;

  vossvolvox::ArgumentParser parser(
      argv[0],
      "Compute RNA vs protein volumes on a shared grid and export MRC maps.");
  parser.add_option("-r",
                    "--rna-input",
                    rna_file,
                    std::string(),
                    "Input structure file containing RNA coordinates (XYZR, PDB, mmCIF, PDBML).",
                    "<rna input>");
  parser.add_option("-a",
                    "--amino-input",
                    amino_file,
                    std::string(),
                    "Input structure file containing amino-acid coordinates (XYZR, PDB, mmCIF, PDBML).",
                    "<amino input>");
  parser.add_option("-p",
                    "--probe",
                    PROBE,
                    10.0,
                    "Probe radius in Angstroms.",
                    "<probe>");
  parser.add_option("-g",
                    "--grid",
                    grid,
                    GRID,
                    "Grid spacing in Angstroms.",
                    "<grid>");
  vossvolvox::add_filter_options(parser, filters);
  parser.add_example("./Custom.exe -r rna.xyzr -a protein.xyzr -p 10 -g 0.8");

  const auto parse_result = parser.parse(argc, argv);
  if (parse_result == vossvolvox::ArgumentParser::ParseResult::HelpRequested) {
    return 0;
  }
  if (parse_result == vossvolvox::ArgumentParser::ParseResult::Error) {
    return 1;
  }
  if (rna_file.empty() || amino_file.empty()) {
    std::cerr << "Error: both --rna-input and --amino-input are required.\n";
    parser.print_help();
    return 1;
  }

  GRID = grid;

  if (!vossvolvox::quiet_mode()) {
    printCompileInfo(argv[0]);
    printCitation();
  }

  const auto convert_options = vossvolvox::make_conversion_options(filters);

  XYZRBuffer rna_xyzr_buffer;
  if (!vossvolvox::load_xyzr_or_exit(rna_file, convert_options, rna_xyzr_buffer)) {
    return 1;
  }
  XYZRBuffer amino_xyzr_buffer;
  if (!vossvolvox::load_xyzr_or_exit(amino_file, convert_options, amino_xyzr_buffer)) {
    return 1;
  }
  const std::vector<const XYZRBuffer*> buffers = {&rna_xyzr_buffer, &amino_xyzr_buffer};
  const auto grid_result = vossvolvox::prepare_grid_from_xyzr(
      buffers,
      grid,
      static_cast<float>(PROBE),
      rna_file,
      false);
  int rnanumatoms = 0;
  int aminonumatoms = 0;
  if (grid_result.per_input.size() >= 2) {
    rnanumatoms = grid_result.per_input[0];
    aminonumatoms = grid_result.per_input[1];
  }

//HEADER CHECK
  cerr << "Probe Radius: " << PROBE << endl;
  cerr << "Grid Spacing: " << GRID << endl;
  cerr << "Resolution:   " << int(1000.0/float(GRIDVOL))/1000.0 << " voxels per A^3" << endl;
  cerr << "Resolution:   " << int(11494.0/float(GRIDVOL))/1000.0 << " voxels per water molecule" << endl;
  cerr << "RNA file:     " << rna_file << endl;
  cerr << "Amino file:   " << amino_file << endl;

// ****************************************************
// STARTING FIRST FILE
// ****************************************************
//READ FILE INTO RNAgrid
  gridpt *RNAgrid;
  RNAgrid = (gridpt*) std::malloc (NUMBINS);
  if (RNAgrid==NULL) { cerr << "GRID IS NULL" << endl; exit (1); }
  zeroGrid(RNAgrid);
  int rnavoxels = get_ExcludeGrid_fromArray(rnanumatoms, PROBE, rna_xyzr_buffer, RNAgrid);

//READ FILE INTO AminoGrid
  gridpt *AminoGrid;
  AminoGrid = (gridpt*) std::malloc (NUMBINS);
  if (AminoGrid==NULL) { cerr << "GRID IS NULL" << endl; exit (1); }
  zeroGrid(AminoGrid);
  int aminovoxels = get_ExcludeGrid_fromArray(aminonumatoms, PROBE, amino_xyzr_buffer, AminoGrid);

//Subtract the protein grid
  subt_Grids(RNAgrid, AminoGrid);

  trimYAxis(AminoGrid);
  trimYAxis(RNAgrid);

  const char* rnafilename = "rna.mrc";
  writeMRCFile(RNAgrid, rnafilename);
  const char* aminofilename = "amino.mrc";
  writeMRCFile(AminoGrid, aminofilename);

//RELEASE TEMPGRID
  std::free (AminoGrid);
  std::free (RNAgrid);

  cerr << endl << "Program Completed Sucessfully" << endl << endl;
  return 0;
};
