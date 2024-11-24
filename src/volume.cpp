#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <cstdlib> // For atof
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
void parseArguments(int argc, char* argv[], std::string& inputFile, std::string& ezdFile, std::string& pdbFile, std::string& mrcFile, double& probe, float& grid);
void processGrid(double probe, const std::string& ezdFile, const std::string& pdbFile, const std::string& mrcFile, const std::string& inputFile, int numatoms);

int main(int argc, char* argv[]) {
  std::cerr << "\n";

  // Print program information
  printCompileInfo(argv[0]);
  printCitation();

  // Initialize variables
  std::string inputFile, ezdFile, pdbFile, mrcFile;
  double probe = 10.0;
  float grid = GRID; // Use global GRID value initially

  // Parse command-line arguments
  parseArguments(argc, argv, inputFile, ezdFile, pdbFile, mrcFile, probe, grid);

  // Ensure the global GRID is updated to the parsed value
  GRID = grid;

  // Initialize grid dimensions
  finalGridDims(probe);

  // Debugging information
  std::cerr << "Initializing Calculation:\n"
            << "Probe Radius:       " << probe << " Å\n"
            << "Grid Spacing:       " << GRID << " Å\n"
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

// Parse command-line arguments
void parseArguments(int argc, char* argv[], std::string& inputFile, std::string& ezdFile, std::string& pdbFile, std::string& mrcFile, double& probe, float& grid) {
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "-i" && i + 1 < argc) {
      inputFile = argv[++i];
    } else if (arg == "-p" && i + 1 < argc) {
      probe = std::stod(argv[++i]);
    } else if (arg == "-o" && i + 1 < argc) {
      pdbFile = argv[++i];
    } else if (arg == "-e" && i + 1 < argc) {
      ezdFile = argv[++i];
    } else if (arg == "-m" && i + 1 < argc) {
      mrcFile = argv[++i];
    } else if (arg == "-g" && i + 1 < argc) {
      grid = std::stof(argv[++i]);
    } else if (arg == "-h") {
      std::cerr << "Usage: ./Volume.exe [OPTIONS]\n\n"
                << "Volume.exe calculates the volume and surface area for a given probe radius.\n"
                << "It uses grid-based methods to approximate molecular volumes and surface areas.\n"
                << "The output can be saved in various file formats including PDB, EZD, and MRC.\n\n"
                << "Options:\n"
                << "  -i <file>       Input file containing atomic coordinates in XYZR format.\n"
                << "                  This file should specify the atom positions and radii in a simple\n"
                << "                  tab-delimited format with columns: x, y, z, radius.\n\n"
                << "  -p <probe_rad>  Probe radius in Ångströms. Default is 10.0 Å. This value determines\n"
                << "                  the size of the probe used to calculate the excluded volume and\n"
                << "                  accessible surface area.\n\n"
                << "  -g <gridspacing> Grid spacing in Ångströms. Determines the resolution of the grid.\n"
                << "                  Smaller values yield higher resolution but require more memory\n"
                << "                  and computation time. Default is set in the program's configuration.\n\n"
                << "  -o <PDB outfile> Output PDB file for surface points. If specified, writes a PDB\n"
                << "                  file containing points on the molecular surface that are accessible\n"
                << "                  to the specified probe radius.\n\n"
                << "  -e <EZD outfile> Output EZD file. If specified, writes the excluded density\n"
                << "                  map in EZD format for visualization in compatible molecular\n"
                << "                  modeling software.\n\n"
                << "  -m <MRC outfile> Output MRC file. If specified, writes the density map in MRC\n"
                << "                  format for visualization with molecular graphics programs like\n"
                << "                  PyMOL or Chimera.\n\n"
                << "  -h              Display this help message and exit.\n";
      exit(0);
    } else {
      std::cerr << "Unknown argument: " << arg << std::endl;
      exit(1);
    }
  }

  if (inputFile.empty()) {
    std::cerr << "Error: Input file not specified. Use -i <file>.\n";
    exit(1);
  }
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
            << "Probe Radius:       " << probe << " Å\n"
            << "Grid Spacing:       " << GRID << " Å\n"
            << "Total Voxels:       " << voxels << "\n"
            << "Surface Area:       " << surf << " Å²\n"
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
