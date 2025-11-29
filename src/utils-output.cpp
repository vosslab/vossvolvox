/*
** utils-output.cpp
*/
#include <cstdlib>    // for abs
#include <ctime>      // for time, ctime
#include <fstream>    // for basic_ofstream, ofstream, right
#include <iomanip>    // for operator<<, setw, setprecision
#include <iostream>   // for cerr
#include <sstream>    // for basic_ostringstream, ostringstream
#include <string>     // for char_traits, allocator, basic_string
#include "utils.h"    // for GRID, endl, DX, DXY, gridpt

/*********************************************
**********************************************
       PDB OUTPUT FUNCTIONS
**********************************************
*********************************************/

//========================================================
//========================================================
// Function to convert grid indices (i, j, k) and atom number to a water HETATM line for PDB
std::string ijk2pdb(int i, int j, int k, int n) {
  // Calculate XYZ coordinates
  float x = float(i) * GRID + XMIN;
  float y = float(j) * GRID + YMIN;
  float z = float(k) * GRID + ZMIN;

  // Calculate temperature (distance for illustrative purposes)
  float dist = distFromPt(x, y, z);

  // Format the PDB HETATM line
  std::ostringstream oss;
  oss << "HETATM"                                  // Record name
    << std::setw(5) << std::right << (n % 99999 + 1) // Atom serial number
    << "  O   HOH  "                            // Atom and residue type
    << std::setw(4) << std::right << ((n / 10) % 9999 + 1) // Residue serial number
    << "    "                                   // Spacer
    << std::fixed << std::setprecision(3)
    << std::setw(8) << x                        // X coordinate
    << std::setw(8) << y                        // Y coordinate
    << std::setw(8) << z                        // Z coordinate
    << "  1.00"                                 // Occupancy
    << std::setw(6) << std::setprecision(2) << dist; // Temperature factor

  return oss.str();
}

//========================================================
//========================================================
void write_PDB(const gridpt grid[], const char outfile[]) {
  cerr << "Writing FULL PDB to file: " << outfile << endl;

  // Open the output file
  std::ofstream out(outfile);
  if (!out.is_open()) {
    cerr << "Error: Could not open file " << outfile << " for writing." << endl;
    return;
  }

  // Write header information
  out << "REMARK (c) Neil Voss, 2005" << endl;
  out << "REMARK PDB file created from " << XYZRFILE << endl;
  out << "REMARK Grid: " << GRID << "\tGRIDVOL: " << GRIDVOL
      << "\tWater_Res: " << WATER_RES << "\tMaxProbe: " << MAXPROBE
      << "\tCutoff: " << CUTOFF << endl;

  // Progress tracking
  float count = 0;
  int anum = 0;
  const float cat = DZ / 60.0;
  float cut = cat;

  cerr << "Writing the grid to [ " << outfile << " ]..." << endl;
  printBar();

  // Loop through the grid
  int sk = -1;
  for (int k = 0; k < DXYZ; k += DXY) {
    sk++;
    count++;
    if (count > cut) {
      cerr << "^" << flush;
      cut += cat;
    }

    int sj = -1;
    for (int j = 0; j < DXY; j += DX) {
      sj++;
      for (int i = 0; i < DX; i++) {
        if (grid[i + j + k]) {  // Check if grid point is occupied
          anum++;
          // Use ijk2pdb to generate the PDB line
          std::string line = ijk2pdb(i, sj, sk, anum);
          out << line << endl;
        }
      }
    }
  }

  // Finalize output
  out << endl;
  out.close();
  cerr << endl << "Done. Wrote " << anum << " atoms." << endl << endl;
};

//========================================================
//========================================================
void write_SurfPDB(const gridpt grid[], const char outfile[]) {
  std::cerr << "Writing SURFACE PDB to file: " << outfile << std::endl;
  std::ofstream out(outfile);

  // Write header
  out << "REMARK (c) Neil Voss, 2005" << std::endl;
  out << "REMARK PDB file created from " << XYZRFILE << std::endl;
  out << "REMARK Grid: " << GRID << "\tGRIDVOL: " << GRIDVOL
      << "\tWater_Res: " << WATER_RES
      << "\tMaxProbe: " << MAXPROBE << "\tCutoff: " << CUTOFF << std::endl;

  // Progress tracking
  int anum = 0, pnum = 0;
  const float cat = DZ / 60.0;
  float cut = cat;

  std::cerr << "Writing the grid to [" << outfile << "]..." << std::endl;
  printBar();

  // Loop through the grid and write PDB entries for edge points
  int sk = -1, sj = -1;
  for (int k = 0; k < DXYZ; k += DXY) {
    sk++;
    if (++pnum > cut) {
        std::cerr << "^" << std::flush;
        cut += cat;
    }
    sj = -1;
    for (int j = 0; j < DXY; j += DX) {
      sj++;
      for (int i = 0; i < DX; i++) {
        int pt = i + j + k;
        if (grid[pt] && hasEmptyNeighbor(pt, grid)) {
          anum++;
          out << ijk2pdb(i, sj, sk, anum) << std::endl; // Use refactored function
        }
      }
    }
  }

  out << std::endl;
  out.close();

  std::cerr << std::endl << "done! wrote " << anum << " of " << pnum << std::endl << std::endl;
};

/*********************************************
**********************************************
       EZD OUTPUT MASTER FUNCTION
**********************************************
*********************************************/

//========================================================
// Function: computeBlurredValue
// Purpose: Compute a blurred value for a voxel based on its neighbors,
//          applying weights based on their Manhattan distances.
// Parameters:
//    - grid: The grid array representing the 3D voxel structure.
//    - voxelIndex: The index of the voxel for which to compute the blurred value.
// Returns:
//    - The blurred value (normalized or thresholded) for the given voxel.
//========================================================
float computeBlurredValue(const gridpt grid[], int voxelIndex) {
  // Initialize the blurred value
  float value = 0.0;

  // Constants for weighted distances
  const float INV_SQRT_2 = 0.7071; // Weight for neighbors at diagonal distance 2
  const float INV_SQRT_3 = 0.5774; // Weight for neighbors at diagonal distance 3
  const float fill = 21.1044;      // Total possible contribution of all neighbors

  // Iterate over the 3x3x3 neighborhood centered on the voxel
  for (int di = -1; di <= 1; di++) {         // Neighbor offset along the x-axis
    for (int dj = -1; dj <= 1; dj++) {       // Neighbor offset along the y-axis
      for (int dk = -1; dk <= 1; dk++) {     // Neighbor offset along the z-axis
        // Compute the neighbor index
        int neighborIndex = voxelIndex + di + dj * DX + dk * DXY;

        // Check if the neighbor is occupied
        if (grid[neighborIndex]) {
          // Calculate Manhattan distance from the central voxel
          int dist = abs(di) + abs(dj) + abs(dk);

          // Assign weights based on distance
          if (dist == 0) {
            value += 2.0;            // Central voxel
          } else if (dist == 1) {
            value += 1.0;            // Direct neighbor
          } else if (dist == 2) {
            value += INV_SQRT_2;     // Diagonal neighbor (e.g., edge)
          } else if (dist == 3) {
            value += INV_SQRT_3;     // Corner neighbor
          }
        }
      }
    }
  }

  // Normalize or threshold the value
  if (value > 0.5 && value < fill) {
    // Scale to 4 decimal places
    value = int(10000.0 * value / fill + 0.5) / 10000.0;
  } else if (value >= fill) {
    // Maximum value
    value = 1.0;
  } else {
    // Minimum value
    value = 0.0;
  }

  // Return the computed and normalized/thresholded value
  return value;
}

/*********************************************
**********************************************
       EZD OUTPUT MASTER FUNCTION
**********************************************
*********************************************/

void write_BinnedEZD(const gridpt grid[], const char outfile[], int binFactor, bool blur = false) {
  if (binFactor <= 0) {
    std::cerr << "Error: Invalid binFactor. Must be greater than 0." << std::endl;
    return;
  }

  std::cerr << "Processing EZD with bin factor " << binFactor
            << (blur ? " and blurring enabled" : "")
            << " for file: " << outfile << std::endl;

  // Arrays for grid start, end, min/max physical dimensions, origin, and extent
  unsigned int start[3] = {NUMBINS, NUMBINS, NUMBINS};
  unsigned int end[3] = {0, 0, 0};
  float min[3], max[3];
  unsigned int origin[3], extent[3];

  // Identify the boundaries of the occupied grid
  for (unsigned int ind = 0; ind < NUMBINS; ind++) {
    if (grid[ind]) {
      const int coords[3] = {
        int(ind % DX),
        int((ind % DXY) / DX),
        int(ind / DXY)
      };

      for (int axis = 0; axis < 3; axis++) {
        if (coords[axis] < start[axis]) start[axis] = coords[axis];
        if (coords[axis] > end[axis]) end[axis] = coords[axis];
      }
    }
  }

  // Expand grid boundaries for binning factor
  for (int axis = 0; axis < 3; axis++) {
    start[axis] -= binFactor;
    end[axis] += binFactor;
  }

  // Compute physical min/max values
  min[0] = start[0] * GRID + XMIN;
  min[1] = start[1] * GRID + YMIN;
  min[2] = start[2] * GRID + ZMIN;

  max[0] = end[0] * GRID + XMIN;
  max[1] = end[1] * GRID + YMIN;
  max[2] = end[2] * GRID + ZMIN;

  // Compute ORIGIN and EXTENT
  for (int axis = 0; axis < 3; axis++) {
    origin[axis] = int((min[axis] / GRID - 1.0) / binFactor + 0.5);
    extent[axis] = int((end[axis] - start[axis] + 1) / binFactor + 0.5);
  }

  // Open the output file
  std::ofstream out(outfile);
  if (!out.is_open()) {
    std::cerr << "Error: Could not open file " << outfile << " for writing." << std::endl;
    return;
  }

  // Write the EZD header
  time_t t;
  time(&t);
  out << "EZD_MAP" << std::endl
      << "! EZD file (c) Neil Voss, 2005" << std::endl
      << "! Grid spacing: " << GRID << " A, scaled by binning factor: " << binFactor << std::endl
      << "! Dimensions (X, Y, Z): " << max[0] - min[0] << " x " << max[1] - min[1] << " x " << max[2] - min[2] << " A" << std::endl
      << "! Water resolution: " << WATER_RES << " A" << std::endl
      << "! Date: " << ctime(&t) << std::endl;

  if (binFactor > 1) {
    out << "! NOTE: This grid is binned by a factor of " << binFactor << "." << std::endl;
  }

  // Write CELL, ORIGIN, EXTENT, and GRID information
  out << "CELL " << int(max[0] - min[0] + 1) << ".0 "
      << int(max[1] - min[1] + 1) << ".0 "
      << int(max[2] - min[2] + 1) << ".0 90.0 90.0 90.0" << std::endl;

  out << "ORIGIN " << origin[0] << " " << origin[1] << " " << origin[2] << std::endl;
  out << "EXTENT " << extent[0] << " " << extent[1] << " " << extent[2] << std::endl;
  out << "GRID " << extent[0] << " " << extent[1] << " " << extent[2] << std::endl;
  out << "SCALE 1.0" << std::endl;
  out << "MAP" << std::endl;

  // Write voxel data
  int outcnt = 0;
  for (int k = start[2]; k <= end[2]; k += binFactor) {
    for (int j = start[1]; j <= end[1]; j += binFactor) {
      for (int i = start[0]; i <= end[0]; i += binFactor) {
        int voxelIndex = i + j * DX + k * DXY;
        float value = 0.0f;
        if (blur) {
          value = computeBlurredValue(grid, voxelIndex);
        } else if (grid[voxelIndex]) {
          value = 1.0f;
        } else {
          value = 0.0f;
        }

        // Write the computed value to the file
        out << value << " ";
        outcnt++;

        // Add a line break after every 7 values for readability
        if (outcnt % 7 == 0) {
          out << std::endl;
        }
      }
    }
  }

  // Finalize the EZD file
  out << std::endl << "END" << std::endl;
  out.close();
  std::cerr << "Done. Wrote file: " << outfile << std::endl;
}

void write_EZD(const gridpt grid[], const char outfile[]) {
  write_BinnedEZD(grid, outfile, 1, false);
}

void write_HalfEZD(const gridpt grid[], const char outfile[]) {
  write_BinnedEZD(grid, outfile, 2, false);
}

void write_ThirdEZD(const gridpt grid[], const char outfile[]) {
  write_BinnedEZD(grid, outfile, 3, false);
}

void write_FifthEZD(const gridpt grid[], const char outfile[]) {
  write_BinnedEZD(grid, outfile, 5, false);
}

//========================================================
//========================================================
void write_BlurEZD (const gridpt grid[], const char outfile[]) {
  write_BinnedEZD(grid, outfile, 1, true);
};
