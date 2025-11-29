#include <iostream>
#include <memory>
#include <string>

#include "argument_helper.h"
#include "utils.h"

// Globals
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

// Function prototypes
void processGrid(double probe,
                 const std::string& ezdFile,
                 const std::string& pdbFile,
                 const std::string& mrcFile,
                 const std::string& inputFile,
                 int numatoms);

int main(int argc, char* argv[]) {
  std::cerr << "\n";

  // Initialize variables
  std::string inputFile;
  std::string ezdFile;
  std::string pdbFile;
  std::string mrcFile;
  double probe = 10.0;
  float grid = GRID;  // Use global GRID value initially

  vossvolvox::ArgumentParser parser(
      argv[0],
      "Calculate molecular volume and surface area for a given probe radius.");
  parser.add_option("-i", "--input", inputFile, std::string(), "Input XYZR file (required).", "<XYZR file>");
  parser.add_option("-p", "--probe", probe, 10.0, "Probe radius in Angstroms (default 10.0).", "<probe radius>");
  parser.add_option("-g", "--grid", grid, GRID, "Grid spacing in Angstroms.", "<grid spacing>");
  parser.add_option("-o", "--pdb-output", pdbFile, std::string(), "Write accessible surface points to this PDB file.", "<PDB file>");
  parser.add_option("-e", "--ezd-output", ezdFile, std::string(), "Write excluded density to this EZD file.", "<EZD file>");
  parser.add_option("-m", "--mrc-output", mrcFile, std::string(), "Write excluded density to this MRC file.", "<MRC file>");
  parser.add_example(std::string(argv[0]) + " -i sample.xyzr -p 1.5 -g 0.5 -o surface.pdb");

  const auto parse_result = parser.parse(argc, argv);
  if (parse_result == vossvolvox::ArgumentParser::ParseResult::HelpRequested) {
    return 0;
  }
  if (parse_result == vossvolvox::ArgumentParser::ParseResult::Error) {
    return 1;
  }
  if (inputFile.empty()) {
    std::cerr << "Error: input file not specified. Use -i <file>.\n";
    parser.print_help();
    return 1;
  }

  if (!vossvolvox::quiet_mode()) {
    printCompileInfo(argv[0]);
    printCitation();
  }

  // Ensure the global GRID is updated to the parsed value
  GRID = grid;

  // Initialize grid dimensions
  finalGridDims(probe);

  // Debugging information
  std::cerr << "Initializing Calculation:\n"
            << "Probe Radius:       " << probe << " A\n"
            << "Grid Spacing:       " << GRID << " A\n"
            << "Input File:         " << inputFile << "\n";

  // Read atoms and set grid limits
  int numatoms = read_NumAtoms(const_cast<char*>(inputFile.c_str()));
  assignLimits();

  // Process the grid for volume and surface calculations
  processGrid(probe, ezdFile, pdbFile, mrcFile, inputFile, numatoms);

  // Program completed successfully
  std::cerr << "\nProgram Completed Successfully\n\n";
  return 0;
}

// Process the grid for volume and surface calculations
void processGrid(double probe, const std::string& ezdFile, const std::string& pdbFile, const std::string& mrcFile, const std::string& inputFile, int numatoms) {
  // Allocate memory for the excluded grid
  auto EXCgrid = std::make_unique<gridpt[]>(NUMBINS);
  if (!EXCgrid) {
    std::cerr << "Error: Grid allocation failed.\n";
    exit(1);
  }
  zeroGrid(EXCgrid.get());

  // Populate the grid based on the probe radius
  int voxels = (probe > 0.0) ? get_ExcludeGrid_fromFile(numatoms, probe, const_cast<char*>(inputFile.c_str()), EXCgrid.get())
                             : fill_AccessGrid_fromFile(numatoms, 0.0, const_cast<char*>(inputFile.c_str()), EXCgrid.get());

  long double surf = surface_area(EXCgrid.get());

  std::cerr << "\nSummary of Results:\n"
            << "Probe Radius:       " << probe << " A\n"
            << "Grid Spacing:       " << GRID << " A\n"
            << "Total Voxels:       " << voxels << "\n"
            << "Surface Area:       " << surf << " A^2\n"
            << "Number of Atoms:    " << numatoms << "\n"
            << "Input File:         " << inputFile << "\n";

  if (!mrcFile.empty()) {
    writeMRCFile(EXCgrid.get(), const_cast<char*>(mrcFile.c_str()));
  }
  if (!ezdFile.empty()) {
    write_HalfEZD(EXCgrid.get(), const_cast<char*>(ezdFile.c_str()));
  }
  if (!pdbFile.empty()) {
    write_SurfPDB(EXCgrid.get(), const_cast<char*>(pdbFile.c_str()));
  }

  // Output results to `std::cout` (batch processing)
  std::cout << probe << "\t" << GRID << "\t";
  printVolCout(voxels);
  std::cout << "\t" << surf << "\t" << numatoms << "\t" << inputFile;
  std::cout << "\tprobe,grid,volume,surf_area,num_atoms,file\n";
}
