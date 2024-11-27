/*
** utils-main.cpp
*/
#include <cstdio>     // for sscanf
#include <cstdlib>    // for exit, std::malloc, std::free
#include <cstring>    // for NULL, std::strcpy, strlen
#include <cmath>      // for ceil, pow, sqrt
#include <fstream>    // for basic_ifstream, char_traits
#include <iostream>   // for cerr, cout
#include "utils.h"    // for gridpt, DEBUG, vector, real
#include <cassert> // Required header
#include <vector>

float XMIN, YMIN, ZMIN;
float XMAX, YMAX, ZMAX;
int DX, DY, DZ;
int DXY, DXYZ;
unsigned int NUMBINS;
float MAXPROBE=15;
float GRID=0.5;
float GRIDVOL=GRID*GRID*GRID;
float WATER_RES=14.1372/GRIDVOL;
float CUTOFF=10000;
char XYZRFILE[256]; //XYZR_FILE[0]='\0';

/*********************************************
**********************************************
         INITIALIZE FUNCTIONS
**********************************************
*********************************************/

//init functions
/*********************************************/
void finalGridDims (float maxprobe) {
  GRIDVOL=GRID*GRID*GRID;
  WATER_RES=14137.2/GRIDVOL;
  MAXPROBE = maxprobe;
  XYZRFILE[0]='\0';
  XMIN=1000;
  YMIN=1000;
  ZMIN=1000;
  XMAX=-1000;
  YMAX=-1000;
  ZMAX=-1000;
};

/*********************************************/
float getIdealGrid () {

  double idealgrid, maxgrid=1, mingrid=-1;
  unsigned int dx, dy, dz, voxels;
  unsigned int maxvoxels = 1024*512*512;
  double third = 1/3.;
  idealgrid = pow((XMAX-XMIN)*(YMAX-YMIN)*(ZMAX-ZMIN)/maxvoxels, third);

  double increment = 0.001;
  idealgrid = int(idealgrid/increment)*increment;
  while(maxgrid - mingrid > 2*increment) {
  	//cerr << "xx ideal grid: " << idealgrid << std::endl;
	dx=int((XMAX-XMIN)/idealgrid/4.0+1)*4;
  	dy=int((YMAX-YMIN)/idealgrid/4.0+1)*4;
  	dz=int((ZMAX-ZMIN)/idealgrid/4.0+1)*4;
	voxels = dx*dy*dz;
  	//cerr << "voxels: " << voxels << "/" << maxvoxels << std::endl;
	if (voxels > maxvoxels) {
		mingrid = idealgrid;
		idealgrid += increment;
	} else {
		maxgrid = idealgrid;
		idealgrid -= increment;
	}
  }
  return maxgrid;
};

/********************************************
float OLDgetIdealGrid () {
  bool cont = 1;
  unsigned int numbins;
  int diff;
  unsigned int dx,dy,dz,dxy,dxyz;
  float grid = GRID;
  float bng=-1.0, bpg=-1.0;
  int bnd=1, bpd=-1;
  int counts=0;

  while(cont) {
    counts++;
    if(grid < 0.0001) { grid += 0.01; }
    dx=int((XMAX-XMIN)/grid+1);
    dy=int((YMAX-YMIN)/grid+1);
    dz=int((ZMAX-ZMIN)/grid+1);
    dxy=(dy*dx);
    dxyz=(dz*dxy);
    numbins = dxyz + dxy + dx + 1;

    diff = MAXBINS - numbins;
    //cerr << "grid = " << grid << "\tdiff = " << diff <<
//	"\tnumbins = " << numbins << std::endl;
    if(diff < 0) {
      if(bng < 0 || diff > bnd) {
        bng = grid;
        bnd = diff;
      }
      grid += 0.0001;
    }
    if(diff > 0) {
      if(bpg < 0 || diff < bpd) {
        bpg = grid;
        bpd = diff;
      }
      grid -= 0.0001;
    }
//    std::cerr << "\tbpg = " << bpg << "\tbng = " << bng << std::endl;
//    std::cerr << "\tbpdiff = " << bpd << "\tbndiff = " << bnd << std::endl;
    if(fabs(bpg - bng) < 0.0002 || counts > 10000) {
      cont = 0;
    }
  }
  return bpg;
};*/

/*********************************************/
// A helper function for clarity and reuse
unsigned int calculateDimension(float min, float max, float grid) {
  return static_cast<unsigned int>(std::ceil((max - min) / grid / 4.0)) * 4;
}

void assignLimits() {
  DX = calculateDimension(XMIN, XMAX, GRID);
  DY = calculateDimension(YMIN, YMAX, GRID);
  DZ = calculateDimension(ZMIN, ZMAX, GRID);
  DXY = DY * DX;
  DXYZ = DZ * DXY;
  NUMBINS = DXYZ + DXY + DX + 1;

  std::cerr << "Percent filled NUMBINS/2^31: " <<
            (NUMBINS * 1000.0 / MAXBINS) / 10.0 << "%" << std::endl;

  float idealGrid = getIdealGrid(); // Assuming getIdealGrid() is defined elsewhere
  std::cerr << "Ideal Grid: " << idealGrid << std::endl;

  // Improved debug message for when NUMBINS exceeds MAXBINS
  /*if(NUMBINS > MAXBINS) {
    std::cerr << "MAXPROBE " << MAXPROBE << ", grid " << GRID
              << " is too large; consider using " << idealGrid
              << " for " << XYZRFILE // Assuming XYZRFILE is defined and used elsewhere
              << std::endl << "###### grid is too large ######" << std::endl << std::endl;
    exit(1); // Consider throwing an exception instead of calling exit for better error handling
  }*/

  std::cerr << std::endl;
}

/*********************************************/
void testLimits (gridpt grid[]) {
  std::cerr << "int(1.2) is " << int(1.2) << std::endl;
  std::cerr << "int(-1.2) is " << int(-1.2) << std::endl;

  std::cerr << "XMIN: " << XMIN << std::endl;
  std::cerr << "YMIN: " << YMIN << std::endl;
  std::cerr << "ZMIN: " << ZMIN << std::endl;

  std::cerr << "DX: " << DX << std::endl;
  std::cerr << "DY: " << DY << std::endl;
  std::cerr << "DZ: " << DZ << std::endl;
  std::cerr << "DXY: " << DXY << std::endl;
  std::cerr << "DXYZ: " << DXYZ << std::endl;
  std::cerr << "NUMBINS: " << NUMBINS << std::endl;

  unsigned int i;
  std::cerr << "First filled spot: ";
  for(i=0; i<NUMBINS && !grid[i]; i++) { }
  std::cerr << i << std::endl << "Last filled spot: ";
  for(i=NUMBINS-1; i>=0 && !grid[i]; i--) { }
  std::cerr << i << std::endl;
  std::cerr << std::endl;
};

/*********************************************
**********************************************
         GRID UTILITY FUNCTIONS
**********************************************
*********************************************/

/*********************************************/
int countGrid (const gridpt grid[]) {
  int voxels=0;

  if (DEBUG > 0)
    std::cerr << "Counting up Voxels in Grid for Volume...  " << std::flush;

  for(unsigned int pt=0; pt<NUMBINS; pt++) {
    if(grid[pt]) {
      voxels++;
    }
  }
  if (DEBUG > 0)
    std::cerr << "done [ " << voxels << " voxels ]" << std::endl << std::endl;
  return voxels;
};

/*********************************************/
void zeroGrid (gridpt grid[]) {
  if (grid==NULL) {
    std::cerr << "Allocating Grid..." << std::endl;
    grid = (gridpt*) std::malloc (NUMBINS);
    if (grid==NULL) { std::cerr << "GRID IS NULL" << std::endl; exit (1); }
  }

  if (DEBUG > 0)
    std::cerr << "Zero-ing All Voxels in the Grid...  " << std::flush;

  #pragma omp parallel for
  for(unsigned int pt=0; pt<NUMBINS; pt++) {
    grid[pt] = 0;
  }
  if (DEBUG > 0)
    std::cerr << "done " << std::endl << std::endl;
  return;
};

/*********************************************/
int copyGridFromTo (const gridpt oldgrid[], gridpt newgrid[]) {
  return copyGrid(oldgrid, newgrid);
}

/*********************************************/
int copyGrid (const gridpt oldgrid[], gridpt newgrid[]) {
  //Zero Grid Not Required
  int voxels=0;
  if (newgrid==NULL) {
    std::cerr << "Allocating Grid..." << std::endl;
    newgrid = (gridpt*) std::malloc (NUMBINS);
    if (newgrid==NULL) { std::cerr << "GRID IS NULL" << std::endl; exit (1); }
  }

  if (DEBUG > 0)
    std::cerr << "Duplicating Grid and Counting up Voxels...  " << std::flush;

  #pragma omp parallel for
  for(unsigned int pt=0; pt<NUMBINS; pt++) {
    if(oldgrid[pt]) {
      #pragma omp atomic
      voxels++;
      newgrid[pt] = 1;
    } else {
      newgrid[pt] = 0;
    }
  }
  if (DEBUG > 0)
    std::cerr << "done [ " << voxels << " voxels ]" << std::endl << std::endl;
  return voxels;
};

/*********************************************/
void inverseGrid (gridpt grid[]) {
  if (grid==NULL) {
    std::cerr << "Allocating Grid..." << std::endl;
    grid = (gridpt*) std::malloc (NUMBINS);
    if (grid==NULL) { std::cerr << "GRID IS NULL" << std::endl; exit (1); }
  }
  if (DEBUG > 0)
    std::cerr << "Inversing All Voxels in the Grid...  " << std::flush;

  #pragma omp parallel for
  for(unsigned int pt=0; pt<NUMBINS; pt++) {
    if(grid[pt]) {
      grid[pt] = 0;
    } else {
      grid[pt] = 1;
    }
  }
  if (DEBUG > 0)
    std::cerr << "done " << std::endl << std::endl;
  return;
};


/*********************************************
**********************************************
        FILE BASED FUNCTIONS
**********************************************
*********************************************/

/*********************************************/
int read_NumAtoms (char file[]) {
  ifstream infile;
  int count = 0;
  char line[256];
  float minmax[6];
  minmax[0] = 100;  minmax[1] = 100;  minmax[2] = 100;
  minmax[3] = -100;  minmax[4] = -100;  minmax[5] = -100;
  std::strcpy(XYZRFILE,file);

  std::cerr << "Reading file for Min/Max: " << file << std::endl;
  infile.open(file);
  while(infile.getline(line,255)) {
    float x,y,z,r;
    sscanf(line," %f %f %f %f",&x,&y,&z,&r);
    if (r > 0 && r < 100) {
      count++;
      if(count % 3000 == 0) { std::cerr << "." << std::flush; }
      if(x < minmax[0]) { minmax[0] = x; }
      if(x > minmax[3]) { minmax[3] = x; }
      if(y < minmax[1]) { minmax[1] = y; }
      if(y > minmax[4]) { minmax[4] = y; }
      if(z < minmax[2]) { minmax[2] = z; }
      if(z > minmax[5]) { minmax[5] = z; }
    }
  }
  infile.close();
  std::cerr << std::endl;
  for(int i=0; i<=5; i++) {
    std::cerr << minmax[i] << " -- " << std::flush;
  }
  std::cerr << std::endl << " [ read " << count << " atoms ]" << std::endl << std::endl;
  if (count < 3) {
    std::cerr << std::endl << " not enough atoms were found" << std::endl << std::endl;
    exit(1);
  }

//INCREASE GRID SIZE TO ACCOMODATE SPHERES
//ALSO ROUND GRID SIZE SO THE XMIN = INTEGER * GRID (FOR BETTER OUTPUT)
  float FACT = MAXVDW + MAXPROBE + 2*GRID;
  for(int i=0;i<=2;i++) {
    minmax[i] -= FACT;
    minmax[i] = int(minmax[i]/(4*GRID)-1)*4*GRID;
  }
  for(int i=3;i<=5;i++) {
    minmax[i] += FACT;
    minmax[i] = int(minmax[i]/(4*GRID)+1)*4*GRID;
  }
  if(minmax[0] < XMIN) { XMIN = minmax[0]; }
  if(minmax[1] < YMIN) { YMIN = minmax[1]; }
  if(minmax[2] < ZMIN) { ZMIN = minmax[2]; }
  if(minmax[3] > XMAX) { XMAX = minmax[3]; }
  if(minmax[4] > YMAX) { YMAX = minmax[4]; }
  if(minmax[5] > ZMAX) { ZMAX = minmax[5]; }

  std::cerr << "Now Run AssignLimits() to Get NUMBINS Variable" << std::endl << std::endl;

  return count;
};

/*********************************************/
int fill_AccessGrid_fromFile (int numatoms, const float probe, char file[],
	gridpt grid[]) {

  if (grid==NULL) {
    std::cerr << "Allocating Grid..." << std::endl;
    grid = (gridpt*) std::malloc (NUMBINS);
    if (grid==NULL) { std::cerr << "GRID IS NULL" << std::endl; exit (1); }
  }
  zeroGrid(grid);

  ifstream infile;
  char line[256];
  if(!XYZRFILE[0]) { std::strcpy(XYZRFILE,file); }

  float count = 0;
  const float cat = float(numatoms)/60.0;
  float cut = cat;

  std::cerr << "Reading file " << file << std::endl;
  std::cerr << "Filling Atoms into Grid (probe " << probe << ")..." << std::endl;
  printBar();

  infile.open(file);
  int filled=0;
  while(infile.getline(line,255)) {
    float x,y,z,r;
    count++;
    if(count > cut) {
      std::cerr << "^" << std::flush;
      cut += cat;
    }
    sscanf(line," %f %f %f %f",&x,&y,&z,&r);
    // parallelized function
    filled += fill_AccessGrid(x,y,z,r+probe,grid);
  }
  infile.close();
  std::cerr << std::endl << "[ read " << count << " atoms ]" << std::endl;

  //OUTPUT INFO
  std::cerr << std::endl << "Access volume for probe " << probe << std::flush;
  std::cerr << "   voxels " << filled << std::flush;
  std::cerr << " x gridvol " << GRIDVOL << std::endl;
  std::cerr << "  ACCESS VOL:  ";
  printVol(filled);
  std::cerr << std::endl;

  return filled;
};

/*********************************************/
int get_ExcludeGrid_fromFile (int numatoms, const float probe,
	char file[], gridpt EXCgrid[]) {
//READ FILE INTO ACCGRID
  gridpt *ACCgrid;
  std::cerr << "Allocating Grid..." << std::endl;
  ACCgrid = (gridpt*) std::malloc (NUMBINS);
  if (ACCgrid==NULL) { std::cerr << "GRID IS NULL" << std::endl; exit (1); }
  fill_AccessGrid_fromFile(numatoms,probe,file,ACCgrid);

//TRUNCATE GRID
  //trun_ExcludeGrid(probe, ACCgrid, EXCgrid);
  trun_ExcludeGrid_fast(probe, ACCgrid, EXCgrid);

//RELEASE ACCGRID
  std::free (ACCgrid);

//OUTPUT INFO
  int voxels = countGrid(EXCgrid);
  std::cerr << std::endl << "******************************************" << std::endl;
  std::cerr << "Excluded Volume for Probe " << probe << std::flush;
  std::cerr << "   voxels " << voxels << std::flush;
  std::cerr << " x gridvol " << GRIDVOL << std::endl;
  std::cerr << "  EXCLUDED VOL:  ";
  printVol(voxels);
  std::cerr << std::endl << "******************************************" << std::endl;

  return voxels;
};


/*********************************************
**********************************************
        GENERATE GRIDS / GRID CHANGERS
**********************************************
*********************************************/

//void expand (gridpt oldgrid[], gridpt newgrid[]);
//void contract (gridpt oldgrid[], gridpt newgrid[]);

/*********************************************/
/*********************************************/
/**
 * Function: trun_ExcludeGrid
 * Purpose:
 *   Truncates the excluded grid (`EXCgrid`) from the accessible grid (`ACCgrid`)
 *   based on a given probe size. It identifies regions that are accessible and
 *   contracts them into excluded regions while respecting grid boundaries.
 *
 * Inputs:
 *   - `probe` (float): The radius of the probe used to truncate the grid.
 *   - `ACCgrid` (gridpt[]): The input accessible grid (binary occupancy).
 *   - `EXCgrid` (gridpt[]): The output excluded grid (modified binary occupancy).
 *     If NULL, the function allocates memory for this grid.
 *
 * Notes:
 *   - The function assumes a 3D grid with linear indexing.
 *   - Grid traversal is limited to specific bounds (`imin`, `imax`, etc.) for efficiency.
 *   - This function is a significant time-limiting factor and is optimized to reduce unnecessary checks.
 *
 *********************************************/
void trun_ExcludeGrid(const float probe, const gridpt ACCgrid[], gridpt EXCgrid[]) { // contract

  // Define grid boundaries for limiting the search region.
  // These constants define the start and end indices for each axis.
  const int imin = 1;      // Minimum i index
  const int jmin = DX;     // Minimum j index (step size = DX)
  const int kmin = DXY;    // Minimum k index (step size = DXY)
  const int imax = DX;     // Maximum i index
  const int jmax = DXY;    // Maximum j index
  const int kmax = DXYZ;   // Maximum k index

  // Progress tracking variables for displaying the progress bar.
  float count = 0;                     // Counter for processed slices.
  const float cat = ((kmax - kmin) / DXY) / 60.0; // Calculate progress increment.
  float cut = cat;                     // Progress cutoff for printing.

  // Allocate memory for `EXCgrid` if it is NULL.
  if (EXCgrid == NULL) {
    std::cerr << "Allocating Grid..." << std::endl;
    EXCgrid = (gridpt*) std::malloc(NUMBINS);
    if (EXCgrid == NULL) {
      std::cerr << "GRID IS NULL" << std::endl;
      exit(1);
    }
  }

  // Copy the `ACCgrid` into `EXCgrid` as the starting point for modification.
  copyGridFromTo(ACCgrid, EXCgrid);

  // Inform the user about the operation being performed.
  std::cerr << "Truncating Excluded Grid from Accessible"
            << " Grid by Probe " << probe << "..." << std::endl;
  printBar(); // Print a progress bar to show the operation's progress.

  // Loop over the k-dimension in large steps (DXY) to limit the search range.
  for (int bigk = kmin; bigk < kmax; bigk += DXY) {
    count++; // Increment the slice counter.
    if (count > cut) {
      std::cerr << "^" << std::flush; // Print progress marker.
      cut += cat;                     // Update the next progress cutoff.
    }

    // Enable parallelization for processing the grid in the j-dimension.
    #pragma omp parallel for
    for (int bigj = jmin; bigj < jmax; bigj += DX) {
      // Loop over the i-dimension with unit step size.
      for (int i = imin; i < imax; i++) {
        const int pt = i + bigj + bigk;
        // Check if the current grid point is unoccupied in the accessible grid.
        if (!ACCgrid[i + bigj + bigk]) {
          // Check if the grid point has any filled neighbors in the accessible grid.
          if (hasFilledNeighbor(pt, ACCgrid)) {
            // Convert `bigj` and `bigk` to smaller indices for the actual 3D grid.
            const int j = bigj / DX;      // Convert bigj to small j index.
            const int k = bigk / DXY;     // Convert bigk to small k index.
            // Mark the grid point as excluded using the emptying function.
            empty_ExcludeGrid(i, j, k, probe, EXCgrid);
          }
        }
      }
    }
  }

  // Finalize and inform the user of completion.
  std::cerr << std::endl << "done" << std::endl << std::endl;
  return;
}


std::vector<int> computeOffsets(const int radius) {
  std::vector<int> offsets;
  const int radius_squared = radius*radius;
  for (int di = -radius; di <= radius; di++) {
    for (int dj = -radius; dj <= radius; dj++) {
      for (int dk = -radius; dk <= radius; dk++) {
        if (di * di + dj * dj + dk * dk <= radius_squared) {
          offsets.push_back(di + dj * DX + dk * DXY);
        }
      }
    }
  }
  return offsets;
}

void trun_ExcludeGrid_fast(const float probe, const gridpt ACCgrid[], gridpt EXCgrid[]) {
  const int imin = 1, jmin = DX, kmin = DXY;
  const int imax = DX, jmax = DXY, kmax = DXYZ;

  float count = 0;
  const float cat = ((kmax - kmin) / DXY) / 60.0;
  float cut = cat;

  // Allocate memory for EXCgrid if NULL
  if (EXCgrid == NULL) {
    std::cerr << "Allocating Grid..." << std::endl;
    EXCgrid = (gridpt *)std::malloc(NUMBINS);
    if (EXCgrid == NULL) {
      std::cerr << "GRID IS NULL" << std::endl;
      exit(1);
    }
  }

  // Copy ACCgrid to EXCgrid
  copyGridFromTo(ACCgrid, EXCgrid);

  // Precompute offsets for the given probe radius
  const int radius = static_cast<int>(probe / GRID + 1);
  std::vector<int> offsets = computeOffsets(radius);

  std::cerr << "Truncating Excluded Grid from Accessible Grid by Probe " << probe << "..." << std::endl;
  printBar();

  for (int bigk = kmin; bigk < kmax; bigk += DXY) {
    count++;
    if (count > cut) {
      std::cerr << "^" << std::flush;
      cut += cat;
    }

    #pragma omp parallel for
    for (int bigj = jmin; bigj < jmax; bigj += DX) {
      for (int i = imin; i < imax; i++) {
        const int pt = i + bigj + bigk;

        // Skip empty grid points
        if (!ACCgrid[pt]) {
          // If the point has filled neighbors, exclude it
          if (hasFilledNeighbor(pt, ACCgrid)) {
            empty_ExcludeGrid_fast(pt, offsets, EXCgrid);
          }
        }
      }
    }
  }

  std::cerr << std::endl << "done" << std::endl << std::endl;
}


/*********************************************/
void grow_ExcludeGrid (const float probe, const gridpt ACCgrid[], gridpt EXCgrid[]) {
//expands
//limit grid search
  //XMIN=(minmax[3] - MAXVDW - PROBE - 2*GRID);
  //XMAX=(minmax[3] + MAXVDW + PROBE + 2*GRID);
  const int imin = 1;
  const int jmin = DX;
  const int kmin = DXY;
  const int imax = DX;
  const int jmax = DXY;
  const int kmax = DXYZ;

  if (EXCgrid==NULL) {
    std::cerr << "Allocating Grid..." << std::endl;
    EXCgrid = (gridpt*) std::malloc (NUMBINS);
    if (EXCgrid==NULL) { std::cerr << "GRID IS NULL" << std::endl; exit (1); }
  }
  copyGrid(ACCgrid,EXCgrid);

//MUST USE COPYGRID BEFORE USING GROW_EXC


  float count = 0;
  const float cat = ((kmax-kmin)/DXY)/60.0;
  float cut = cat;

  std::cerr << std::endl << "Growing Excluded Grid from Accessible "
	<< "Grid by Probe " << probe << "..." << std::endl;
  printBar();

  for(int k=kmin; k<kmax; k+=DXY) {
  count++;
  if(count > cut) {
    std::cerr << "^" << std::flush;
    cut += cat;
  }
  #pragma omp parallel for
  for(int j=jmin; j<jmax; j+=DX) {
  for(int i=imin; i<imax; i++) {
      const int pt = i+j+k;
      if(ACCgrid[pt]) {
        //MUST USE COPYGRID BEFORE USING GROW_EXC
        if(hasEmptyNeighbor(pt,ACCgrid)) {
          const int k2 = k/DXY;
          const int j2 = j/DX;
          //ONLY use of fill_ExcludeGrid()
          fill_ExcludeGrid(i,j2,k2,probe,EXCgrid);
        }
      }
  }}}
  std::cerr << std::endl << "done" << std::endl << std::endl;
  return;
};

/*********************************************/
float *get_Point (gridpt grid[]) {
  int gp;
  int i,j,k;
  float *xyz = (float*) std::malloc ( sizeof(float)*3 );
  for(k=0; k<DZ; k++) {
    for(j=0; j<DY; j++) {
      for(i=0; i<DX; i++) {
        gp = ijk2pt(i,j,k);
        if(grid[gp] == 1) {
          std::cerr << std::endl << "grid point: " << gp << " value: " << grid[gp] << std::endl;
          std::cerr << std::endl << "i:" << i << " j:" << j << " k:" << k << std::endl;
          xyz[0] = float(i-0.5)*GRID + XMIN;
          xyz[1] = float(j-0.5)*GRID + YMIN;
          xyz[2] = float(k-0.5)*GRID + ZMIN;
          std::cerr << std::endl << "x:" << xyz[0] << " y:" << xyz[1] << " z:" << xyz[2] << std::endl;
          return xyz;
        }
      }
    }
  }
  return xyz;
};

/*********************************************/
int get_GridPoint (gridpt grid[]) {
  int gp;
  if (DEBUG > 0)
    std::cerr << "searching for first filled grid point... " << std::endl;
  for(gp=0; gp<DXYZ; gp++) {
    if(grid[gp] == 1) {
		if (DEBUG > 0)
      	cerr << "grid point: " << gp << " of " << DXYZ
              << "; value: " << grid[gp] << std::endl;
      return gp;
    }
  }
  return 0;
};

/*********************************************/
int get_Connected (gridpt grid[], gridpt connect[], const float x, const float y, const float z) {
  std::cerr << std::endl << "x:" << x << " y:" << y << " z:" << z << std::endl;
  const int gp = xyz2pt(x,y,z);
  if (DEBUG > 0)
    std::cerr << "gp: " << gp << " grid value: " << grid[gp] << std::endl;

  if (connect==NULL) {
    std::cerr << "Allocating Grid..." << std::endl;
    connect = (gridpt*) std::malloc (NUMBINS);
    if (connect==NULL) { std::cerr << "GRID IS NULL" << std::endl; exit (1); }
    zeroGrid(connect);
  }

  if (grid[gp] == 0) {
    std::cerr << "GetConnected: Point is NOT FILLED" << std::endl;
    int pt;
    const int delta = int(3.0/GRID);
    bool stop=0;
    int ip = int((x-XMIN)/GRID+0.5);
    int jp = int((y-YMIN)/GRID+0.5);
    int kp = int((z-ZMIN)/GRID+0.5);
    for(int id=-delta; !stop && id<=delta; id++) {
    for(int jd=-delta; !stop && jd<=delta; jd++) {
    for(int kd=-delta; !stop && kd<=delta; kd++) {
      pt = ijk2pt(ip+id,jp+jd,kp+kd);
      if(grid[pt]) {
        float xn,yn,zn;
        pt2xyz(pt, xn, yn, zn);
	     std::cerr << "nearest filled pt: " << xn << " " << yn << " " << zn << std::endl;
        stop=1;
      }
    }}}
  }

  const int max = NUMBINS;
  int steps=0;
  int connected=0;
  if(gp >= 0 && gp <= max && grid[gp]) {
    connect[gp] = 1;
    if (DEBUG > 0)
      std::cerr << "GetConnected..." << std::flush;
// defined in utils.h
//    #define MAXLIST 1048576 //2^20
//    #define MAXLIST 32768 //2^15
//    #define MAXLIST 8192  //2^13
    int LIST[MAXLIST];
    int last = 1;
    LIST[0] = gp;
    LIST[1] = 0;

    while(last != 0) {
    //cerr << "." << std::flush;
      int newlast = 0;
      int NEWLIST[MAXLIST];
      for(int n=0; n<last; n++) {
        steps++;
        int p = LIST[n];
        for(int i=-1; i<=1; i++) {
        for(int j=-DX; j<=DX; j+=DX) {
        for(int k=-DXY; k<=DXY; k+=DXY) {
          int pt = p + i + j + k;
          if(grid[pt] && !connect[pt]) {  //isClose2Tunnel???
            connect[pt] = 1;
            connected++;
            if(newlast < MAXLIST-10) {
              NEWLIST[newlast] = pt;
              newlast++;
            }
          }
        }}}
      }
      #pragma omp parallel for
      for(int n=0; n<newlast; n++) {
        LIST[n] = NEWLIST[n];
      }
      last=newlast;
      LIST[last]=0;
    }
    //cerr << std::endl;
    if(steps > 1) {
      if (DEBUG > 0)
        std::cerr << " performed " << steps << " steps" << std::endl;
    } else {
      if (DEBUG > 0)
        std::cerr << " done" << std::endl;
    }
  } else if(gp > 0 && gp < max) {
    if (DEBUG > 0)
      std::cerr << "GetConnected: Point is NOT FILLED" << std::endl;
  } else {
    if (DEBUG > 0)
      std::cerr << "GetConnected: Point OUT OF RANGE" << std::endl;
  }
  return connected;
};

/*********************************************/
int get_ConnectedRange (gridpt grid[], gridpt connect[], const float x, const float y, const float z) {
//Get selected point in grid
  int gp = xyz2pt(x,y,z);
  int ip,jp,kp;
  ip = int((x-XMIN)/GRID+0.5);
  jp = int((y-YMIN)/GRID+0.5);
  kp = int((z-ZMIN)/GRID+0.5);
  if (connect==NULL) {
    std::cerr << "Allocating Grid..." << std::endl;
    connect = (gridpt*) std::malloc (NUMBINS);
    if (connect==NULL) { std::cerr << "GRID IS NULL" << std::endl; exit (1); }
    zeroGrid(connect);
  }

//Oops selected point isn't open! Better get new one
  if(!grid[gp]) {
    const int delta = int(1.50/GRID);
    bool stop=0;
    int gd=gp;
    for(int id=-delta; !stop && id<=delta; id++) {
    for(int jd=-delta; !stop && jd<=delta; jd++) {
    for(int kd=-delta; !stop && kd<=delta; kd++) {
      gd = ijk2pt(ip+id,jp+jd,kp+kd);
      if(grid[gd]) {
        stop=1;
        gp=gd;
      }
    }}}
  }

  const int max = NUMBINS;
  int steps=0;
  int connected=0;
  if(gp >= 0 && gp <= max && grid[gp]) {
    connect[gp] = 1;
    if (DEBUG > 0)
      std::cerr << "GetConnected..." << std::flush;

//    #define MAXLIST 1048576 //2^20
//    #define MAXLIST 32768 //2^15
//    #define MAXLIST 8192  //2^13
    int LIST[MAXLIST];
    int last = 1;
    LIST[0] = gp;
    LIST[1] = 0;

    while(last != 0) {
    //cerr << "." << std::flush;
      int newlast = 0;
      int NEWLIST[MAXLIST];
      for(int n=0; n<last; n++) {
        steps++;
        int p = LIST[n];
        for(int i=-1; i<=1; i++) {
        for(int j=-DX; j<=DX; j+=DX) {
        for(int k=-DXY; k<=DXY; k+=DXY) {
          int pt = p + i + j + k;
          if(grid[pt] && !connect[pt]) {  //isClose2Tunnel???
            connect[pt] = 1;
            connected++;
            if(newlast < MAXLIST-10) {
              NEWLIST[newlast] = pt;
              newlast++;
            }
          }
        }}}
      }
      #pragma omp parallel for
      for(int n=0; n<newlast; n++) {
        LIST[n] = NEWLIST[n];
      }
      last=newlast;
      LIST[last]=0;
    }
    //cerr << std::endl;
    if(steps > 1) {
      if (DEBUG > 0)
        std::cerr << " performed " << steps << " steps" << std::endl;
    } else {
      if (DEBUG > 0)
        std::cerr << " done" << std::endl;
    }
  } else if(gp > 0 && gp < max) {
    if (DEBUG > 0)
      std::cerr << "GetConnected: Point OUT OF RANGE" << std::endl;
  } else {
    if (DEBUG > 0)
      std::cerr << "GetConnected: Point is NOT FILLED" << std::endl;
  }
  return connected;
};

/*********************************************/
int get_Connected_Point (gridpt grid[], gridpt connect[], const int gp) {
  if (DEBUG > 0)
    std::cerr << "Initialize Get Connected Point..." << std::endl;
  if (connect==NULL) {
    std::cerr << "Allocating Grid..." << std::endl;
    connect = (gridpt*) std::malloc (NUMBINS);
    if (connect==NULL) { std::cerr << "GRID IS NULL" << std::endl; exit (1); }
    zeroGrid(connect);
  }
  const int max = NUMBINS;
  int steps=0;
  int connected=0;
  if(gp >= 0 && gp <= max && grid[gp]) {
    connect[gp] = 1;
    if (DEBUG > 0)
      std::cerr << "Get Connected to Point..." << std::flush;

//    #define MAXLIST 1048576 //2^20
//    #define MAXLIST 32768 //2^15
//    #define MAXLIST 8192  //2^13
    int LIST[MAXLIST];
    int last = 1;
    LIST[0] = gp;
    LIST[1] = 0;


    while(last != 0) {
    //cerr << "." << std::flush;
      int newlast = 0;
      int NEWLIST[MAXLIST];
      for(int n=0; n<last; n++) {
        steps++;
        int p = LIST[n];
        for(int i=-1; i<=1; i++) {
        for(int j=-DX; j<=DX; j+=DX) {
        for(int k=-DXY; k<=DXY; k+=DXY) {
          int pt = p + i + j + k;
          if(grid[pt] && !connect[pt]) {  //isClose2Tunnel???
            connect[pt] = 1;
            connected++;
            if(newlast < MAXLIST-10) {
              NEWLIST[newlast] = pt;
              newlast++;
            }
          }
        }}}
      }
      #pragma omp parallel for
      for(int n=0; n<newlast; n++) {
        LIST[n] = NEWLIST[n];
      }
      last=newlast;
      LIST[last]=0;
    }
    //cerr << std::endl;
    if(steps > 1) {
      if (DEBUG > 0)
        std::cerr << " performed " << steps << " steps" << std::endl;
    } else {
      if (DEBUG > 0)
        std::cerr << " done" << std::endl;
    }
  }
  return connected;
};

/*********************************************/
int subt_Grids (gridpt biggrid[], gridpt smgrid[]) {
  /*
    Subtracts smgrid from biggrid and save to biggrid
    equivalent to B AND !S
		B = 1; S = 1 ==> B = 0
		B = 1; S = 0 ==> B = 1
		B = 0; S = 1 ==> B = 0
		B = 0; S = 0 ==> B = 0
  */
  int voxels=0;
  int error=0;
  //float count = 0;
  //const float cat = DZ/60.0;
  //float cut = cat;
  if (DEBUG > 0)
    std::cerr << "Subtracting Grids (Modifies biggrid)...  " << std::flush;
  //printBar();

  for(unsigned int pt=0; pt<NUMBINS; pt++) {
  /*count++;
  if(count > cut) {
    std::cerr << "^" << std::flush;
    cut += cat;
  }*/
      if(smgrid[pt]) {
        if(biggrid[pt]) {
          voxels++;
          biggrid[pt] = 0;
        } else {
          error++;
        }
      }
  }
  if (DEBUG > 0) {
    std::cerr << "done [ " << voxels << " vox changed ]" << std::endl;
  //cerr << "done [ " << error << " errors : " <<
//	int(1000.0*error/(voxels+error))/10.0 << "% ]" << std::endl;
    std::cerr << std::endl;
  }
  return voxels;
};

/*********************************************/
int intersect_Grids (gridpt grid1[], gridpt grid2[]) {
  /*
    intersect grid1 from grid2 and save to grid1
    equivalent to G1 AND G2
		G1 = 1; G2 = 1 ==> G1 = 1
		G1 = 1; G2 = 0 ==> G1 = 0
		G1 = 0; G2 = 1 ==> G1 = 0
		G1 = 0; G2 = 0 ==> G1 = 0
  */
//GRID1 will CHANGE
  int voxels=0;
  int changed=0;
  //float count = 0;
  //const float cat = NUMBINS/60.0;
  //float cut = cat;
  if (DEBUG > 0)
    std::cerr << "Intersecting Grids...  " << std::flush;
  //printBar();

  for(unsigned int pt=0; pt<NUMBINS; pt++) {
    /*count++;
    if(count > cut) {
      std::cerr << "^" << std::flush;
      cut += cat;
    }*/
    if(grid1[pt]) {
      if(!grid2[pt]) {
        changed++;
        grid1[pt] = 0;
      } else {
        voxels++;
      }
    }
  }
  //cerr << std::endl;
  if (DEBUG > 0) {
    std::cerr << "done [ " << changed << " vox changed ] " << std::flush;
    std::cerr << "[ " << voxels << " vox overlap :: " << std::flush;
    std::cerr << int(1000.0*voxels/(voxels+changed))/10.0 << "% ]" << std::flush;
    std::cerr << std::endl << std::endl;
  }
  return voxels;
};


/*********************************************/
int merge_Grids (gridpt grid1[], gridpt grid2[]) {
  /*
    intersect grid1 from grid2 and save to grid1
    equivalent to G1 AND G2
		G1 = 1; G2 = 1 ==> G1 = 1
		G1 = 1; G2 = 0 ==> G1 = 1
		G1 = 0; G2 = 1 ==> G1 = 1
		G1 = 0; G2 = 0 ==> G1 = 0
  */
//GRID1 will CHANGE
  int voxels=0;
  int changed=0;
  //float count = 0;
  //const float cat = NUMBINS/60.0;
  //float cut = cat;
  if (DEBUG > 0)
    std::cerr << "Merging Grids...  " << std::flush;
  //printBar();

  for(unsigned int pt=0; pt<NUMBINS; pt++) {
    /*count++;
    if(count > cut) {
      std::cerr << "^" << std::flush;
      cut += cat;
    }*/
    if(grid2[pt]) {
      if(!grid1[pt]) {
        changed++;
        grid1[pt] = 1;
      } else {
        voxels++;
      }
    }
  }
  //cerr << std::endl;
  if (DEBUG > 0) {
    std::cerr << "done [ " << changed << " vox changed ] " << std::flush;
    std::cerr << "[ " << voxels << " vox overlap :: " << std::flush;
    std::cerr << int(1000.0*voxels/(voxels+changed))/10.0 << "% ]" << std::flush;
    std::cerr << std::endl << std::endl;
  }
  return voxels;
};


/*********************************************
**********************************************
        POINT BASED FUNCTIONS
**********************************************
*********************************************/

/*********************************************/
int fill_AccessGrid (const float x, const float y, const float z,
	const float R, gridpt grid[]) {
// could redo to simplify
  const float cutoff = (R / GRID)*(R / GRID);

  const int imin = int((x - XMIN - R)/GRID - 1.0);
  const int jmin = int((y - YMIN - R)/GRID - 1.0);
  const int kmin = int((z - ZMIN - R)/GRID - 1.0);
  const int imax = int((x - XMIN + R)/GRID + 1.0);
  const int jmax = int((y - YMIN + R)/GRID + 1.0);
  const int kmax = int((z - ZMIN + R)/GRID + 1.0);

  const float xk = (x - XMIN)/GRID;
  const float yk = (y - YMIN)/GRID;
  const float zk = (z - ZMIN)/GRID;

  float distsq;
  int filled=0;
  #pragma omp parallel for
  for(int di=imin; di<=imax; di++) {
  for(int dj=jmin; dj<=jmax; dj++) {
  for(int dk=kmin; dk<=kmax; dk++) {
     distsq = (xk-di)*(xk-di) + (yk-dj)*(yk-dj) + (zk-dk)*(zk-dk);
     if(distsq < cutoff) {
       int pt = ijk2pt(di,dj,dk);
       if(!grid[pt]) {
         grid[pt] = 1;
         #pragma omp atomic
         filled++;
       }
     }
  }}}
  return filled;
};

/*********************************************/
/**
 * Function: empty_ExcludeGrid
 * Purpose:
 *   This function marks grid points within a given radius (probe size)
 *   of a specified grid location (i, j, k) as "excluded" by setting their
 *   occupancy to 0 in the provided grid array.
 *
 * Inputs:
 *   - `i, j, k` (int): The 3D grid indices representing the center of the exclusion sphere.
 *   - `probe` (float): The radius of the exclusion sphere in physical units.
 *   - `grid` (gridpt[]): The grid array (binary occupancy) to be updated.
 *
 * Outputs:
 *   - Updates the `grid` array by setting grid points within the probe radius to 0.
 *
 * Key Details:
 *   - The function calculates a spherical exclusion zone based on the `probe` size.
 *   - Boundary checks are performed to prevent out-of-bounds access.
 *   - Optimized to avoid unnecessary checks outside the exclusion radius.
 *
 * Notes:
 *   - This function is highly time-critical and should not be parallelized,
 *     as the calling function (`trun_ExcludeGrid`) handles parallelization.
 *   - Uses the squared distance (`distsq`) for computational efficiency, avoiding square root operations.
 *********************************************/
void empty_ExcludeGrid(const int i, const int j, const int k, const float probe, gridpt grid[]) {
  // Compute the scaled probe radius in grid units.
  const float R = probe / GRID;   // Convert the probe size to grid units.
  const int r = int(R + 1);       // Integer radius for limiting neighbor checks.
  const float cutoff = R * R;     // Squared radius for distance comparisons.

  // Variables for neighbor bounds within the grid.
  int nri, nrj, nrk;  // Negative radius limits for x, y, z.
  int pri, prj, prk;  // Positive radius limits for x, y, z.

  // Boundary checks to prevent overflow or underflow (stay within grid limits).
  // For negative bounds:
  if(i < r) { nri = -i; } else { nri = -r;}
  if(j < r) { nrj = -j; } else { nrj = -r;}
  if(k < r) { nrk = -k; } else { nrk = -r;}
  if(i + r >= DX) { pri = DX-i-1; } else { pri = r;}
  if(j + r >= DY) { prj = DY-j-1; } else { prj = r;}
  if(k + r >= DZ) { prk = DZ-k-1; } else { prk = r;}

  // Iterate over the neighboring points within the calculated bounds.
  // The loop covers a cubic region around (i, j, k), but checks for spherical distance.
  float distsq; // Squared distance of the current point from the center.
  int ind;      // Linear index of the grid point being evaluated.

  for (int di = nri; di <= pri; di++) {         // Loop over the x-axis neighbors.
    for (int dj = nrj; dj <= prj; dj++) {       // Loop over the y-axis neighbors.
      for (int dk = nrk; dk <= prk; dk++) {     // Loop over the z-axis neighbors.
        int ind = (i + di) + (j + dj) * DX + (k + dk) * DXY; // Inline ijk2pt
        if (grid[ind] && (di * di + dj * dj + dk * dk < cutoff)) {
          grid[ind] = 0;
        }
      }
    }
  }

  return;
};

void empty_ExcludeGrid_fast(const int pt, const std::vector<int> &offsets, gridpt grid[]) {
  // Iterate over precomputed offsets
  for (const int offset : offsets) {
    const int neighbor = pt + offset;

    // Skip out-of-bounds neighbors
    if (neighbor < 0 || neighbor >= NUMBINS) {
      std:cerr << "Major Out of Bounds Error" << std::endl;
      exit(1);
    }

    // Mark grid point as excluded if filled
    grid[neighbor] = 0;
  }
}

/*********************************************/
void fill_ExcludeGrid (const int i, const int j, const int k,
	const float probe, gridpt grid[]) {
//provides indexes (i,j,k) of grid where ijk2pt(i,j,k) = gridpt
  const float R = probe/GRID; //Aug 19: correction for oversize
  const int r = int(R+1);
  const float cutoff = R*R;
  int nri,nrj,nrk,pri,prj,prk;
//overflow checks (let's not go off the grid)
  if(i < r) { nri = -i; } else { nri = -r;}
  if(j < r) { nrj = -j; } else { nrj = -r;}
  if(k < r) { nrk = -k; } else { nrk = -r;}
  if(i + r >= DX) { pri = DX-i-1; } else { pri = r;}
  if(j + r >= DY) { prj = DY-j-1; } else { prj = r;}
  if(k + r >= DZ) { prk = DZ-k-1; } else { prk = r;}
  float distsq;
  int ind;
  // do not parallelize done in previous step
  for(int di=nri; di<=pri; di++) {
  for(int dj=nrj; dj<=prj; dj++) {
  for(int dk=nrk; dk<=prk; dk++) {
     ind = ijk2pt(i+di,j+dj,k+dk);
     if(!grid[ind]) {
       distsq = di*di + dj*dj + dk*dk;
       if(distsq < cutoff) {
         grid[ind] = 1;
       }
     }
  }}}
  return;
};

/*********************************************/
// Converts 3D grid indices `(i, j, k)` to a 1D linear index `pt`.
//
// Parameters:
// - `i`: Index along the x-axis (const).
// - `j`: Index along the y-axis (const).
// - `k`: Index along the z-axis (const).
//
// Returns:
// - A linear index `pt` corresponding to the 3D grid indices.
//
// Notes:
// - This function assumes a row-major order for the grid storage.
// - If the resulting `pt` exceeds the maximum valid index (`DXYZ`),
//   it reports an error and clamps `pt` to the highest valid index.
int ijk2pt(const int i, const int j, const int k) {
  int pt = int(i + j * DX + k * DXY);
  if (pt >= DXYZ) {  // Check for out-of-bounds access
    std::cerr << "Error: ijk2pt index out of bounds :: " << i << ", " << j << ", " << k << std::endl;
    return DXYZ - 1; // Clamp to the highest valid index
  }
  return pt;
};

/*********************************************/
// Converts a 1D linear index `pt` to 3D grid indices `(i, j, k)`.
//
// Parameters:
// - `pt`: The 1D linear index (const).
// - `i`: Reference to store the x-axis index.
// - `j`: Reference to store the y-axis index.
// - `k`: Reference to store the z-axis index.
//
// Notes:
// - This function uses the grid dimensions (`DX`, `DXY`) to calculate
//   the original 3D indices from the linear index.
void pt2ijk(const int pt, int &i, int &j, int &k) {
  i = pt % DX;           // Compute the x-axis index
  j = (pt % DXY) / DX;   // Compute the y-axis index
  k = pt / DXY;          // Compute the z-axis index
  return;
};

/*********************************************/
// Converts a 1D linear index `pt` to 3D physical coordinates `(x, y, z)`.
//
// Parameters:
// - `pt`: The 1D linear index (const).
// - `x`: Reference to store the x-coordinate in physical space.
// - `y`: Reference to store the y-coordinate in physical space.
// - `z`: Reference to store the z-coordinate in physical space.
//
// Notes:
// - Physical coordinates are calculated using grid spacing (`GRID`)
//   and origin offsets (`XMIN`, `YMIN`, `ZMIN`).
void pt2xyz(const int pt, float &x, float &y, float &z) {
  int i, j, k;            // Temporary variables for 3D grid indices
  pt2ijk(pt, i, j, k);    // Convert `pt` to grid indices
  x = float(i) * GRID + XMIN; // Compute x-coordinate in physical space
  y = float(j) * GRID + YMIN; // Compute y-coordinate in physical space
  z = float(k) * GRID + ZMIN; // Compute z-coordinate in physical space
  return;
};

/*********************************************/
// Converts 3D physical coordinates `(x, y, z)` to a 1D linear index `pt`.
//
// Parameters:
// - `x`: The x-coordinate in physical space (const).
// - `y`: The y-coordinate in physical space (const).
// - `z`: The z-coordinate in physical space (const).
//
// Returns:
// - The corresponding 1D linear index `pt`.
//
// Notes:
// - This function uses grid spacing (`GRID`) and origin offsets (`XMIN`,
//   `YMIN`, `ZMIN`) to compute the grid indices from physical coordinates.
// - Coordinates are rounded to the nearest grid point.
int xyz2pt(const float x, const float y, const float z) {
  int ip = int((x - XMIN) / GRID + 0.5); // Compute x-axis index
  int jp = int((y - YMIN) / GRID + 0.5); // Compute y-axis index
  int kp = int((z - ZMIN) / GRID + 0.5); // Compute z-axis index
  return ijk2pt(ip, jp, kp); // Convert to linear index
};


/*********************************************/
bool hasFilledNeighbor(const int pt, const gridpt grid[]) {
  short int count = 0; // Counter for the number of neighbors checked.

  // Check neighbors along the i-axis.
  for (int index = -1; index <= 1; index += 2) { // Iterate over -1 and +1.
    count++; // Increment the neighbor count.
    if (grid[pt + index]) {
      return true; // Return true if the neighbor is empty.
    }
  }

  // Check neighbors along the j-axis.
  for (int index = -1; index <= 1; index += 2) { // Iterate over -1 and +1.
    count++; // Increment the neighbor count.
    if (grid[pt + DX * index]) {
      return true; // Return true if the neighbor is empty.
    }
  }

  // Check neighbors along the k-axis.
  for (int index = -1; index <= 1; index += 2) { // Iterate over -1 and +1.
    count++; // Increment the neighbor count.
    if (grid[pt + DXY * index]) {
      return true; // Return true if the neighbor is empty.
    }
  }
  if (count != 6) {
    std::cerr << "Neighbor count " << count << " != 6" << std::endl;
  }
  return false;
};


/*********************************************/
// Function to determine if a grid point is an edge point using a "star" pattern.
// This function examines 6 neighbors along the principal axes (i, j, k).
// Returns true if at least one neighbor is empty (edge point), false otherwise.
bool hasEmptyNeighbor(const int pt, const gridpt grid[]) {
  short int count = 0; // Counter for the number of neighbors checked.

  // Check neighbors along the i-axis.
  for (int index = -1; index <= 1; index += 2) { // Iterate over -1 and +1.
    count++; // Increment the neighbor count.
    if (!grid[pt + index]) {
      return true; // Return true if the neighbor is empty.
    }
  }

  // Check neighbors along the j-axis.
  for (int index = -1; index <= 1; index += 2) { // Iterate over -1 and +1.
    count++; // Increment the neighbor count.
    if (!grid[pt + DX * index]) {
      return true; // Return true if the neighbor is empty.
    }
  }

  // Check neighbors along the k-axis.
  for (int index = -1; index <= 1; index += 2) { // Iterate over -1 and +1.
    count++; // Increment the neighbor count.
    if (!grid[pt + DXY * index]) {
      return true; // Return true if the neighbor is empty.
    }
  }

  // Debugging: Ensure all 6 neighbors were checked correctly.
  if (count != 6) {
    std::cerr << "EdgePoint count " << count << " != 6" << std::endl;
  }

  // If all neighbors are occupied, this is not an edge point.
  return false;
}

/*********************************************/
// Function to determine if a grid point has any empty neighbors within a 3x3x3 cube.
// This function examines all 26 neighbors (3x3x3 cube minus the center point).
// Returns true if at least one neighbor is empty, false otherwise.
bool hasEmptyNeighbor_Fill(const int pt, const gridpt grid[]) {
  short int count = 0; // Counter for the number of neighbors checked.

  // Iterate through neighbors along the i-axis.
  for (int di = -1; di <= 1; di++) {
    // Iterate through neighbors along the j-axis.
    for (int dj = -1; dj <= 1; dj++) {
      // Iterate through neighbors along the k-axis.
      for (int dk = -1; dk <= 1; dk++) {
        count++; // Increment the neighbor count.

        // Compute the neighbor index.
        int neighborPt = pt + di + dj * DX + dk * DXY;

        // Check if the neighbor is empty.
        if (!grid[neighborPt]) {
          return true; // Return true if the neighbor is empty.
        }
      }
    }
  }

  // Debugging: Ensure all 26 neighbors were checked correctly.
  if (count != 27) {
    std::cerr << "Neighbor count " << count << " != 27" << std::endl;
  }

  // If all neighbors are filled, this is not an edge point.
  return false;
}

//void expand_Point (const int pt, gridpt grid[]);
//void contract_Point (const int pt, gridpt grid[]);

/*********************************************/
void limitToTunnelArea(const float radius, gridpt grid[]) {
  std::cerr << "Limiting to Cylinder Around Exit Tunnel...  " << std::flush;

  #pragma omp parallel for
  for(int pt=0; pt<=DXYZ; pt++) {
    if(!isCloseToVector(radius,pt)) {
      grid[pt] = 0;
    }
  }
  if (DEBUG > 0)
    std::cerr << "done " << std::endl << std::endl;
  return;
};

/*********************************************/
bool isCloseToVector (const float radius, const int pt) {

//GET QUERY POINT
  const float x = int(pt % DX) * GRID + XMIN;
  const float y = int((pt % DXY)/ DX) * GRID + YMIN;
  const float z = int(pt / DXY) * GRID + ZMIN;

//GET DISTANCE
  float dist = distFromPt(x,y,z);

//RETURN
  if(dist < radius) {
    return 1;
  }
  return 0;
};

/*********************************************/
float distFromPt (const float x, const float y, const float z) {
//INIT POINT
  //const float xp = 53.652;
  //const float yp = 141.358;
  //const float zp = 66.460;
  const float xp = 58.920;
  const float yp = 140.063;
  const float zp = 80.060;

//VECTOR
  const float xv =  0.58092;
  const float yv = -0.60342;
  const float zv =  0.54627;

//DIFFERENCE VECTOR
  const float dx = x - xp;
  const float dy = y - yp;
  const float dz = z - zp;

  const float lensq = dx*dx + dy*dy + dz*dz;
  const float dot = dx*xv + dy*yv + dz*zv;

//GET CROSS PRODUCT
  const float cross = sqrt(lensq - dot*dot);

  return cross;
};

/*********************************************/
float crossSection (const real p, const vector v, const gridpt grid[])
{
  return crossSection(grid);
};

/*********************************************/
float crossSection (const gridpt grid[])
{
//INIT POINT
  struct real p;
  p.x =  77.0;
  p.y = 124.0;
  p.z =  99.0;

//TUNNEL VECTOR
  struct vector v;
  v.x = -0.58092;
  v.y =  0.60342;
  v.z = -0.54627;

//GENERATE 2D GRID
//  FIND 2 PERP VECTORS
  struct vector v1, v2;
  v1.x =  0.60342; // =v.y
  v1.y =  0.58092; // =-v.x
  v1.z =  0.00000; // =0
  v2.x = -0.31734; //
  v2.y =  0.32963;
  v2.z =  0.70159;

//  const float x = int(pt % DX) * GRID + XMIN;
//  const float y = int((pt % DXY)/ DX) * GRID + YMIN;
//  const float z = int(pt / DXY) * GRID + ZMIN;
//  return int(i+j*DX+k*DXY);

  struct real r;
  int pt;
  float count;
  //double mult = GRID*GRID*0.5*0.5*2.0/3.0;
  double mult = GRID*GRID/6.0;
  std::cerr << "stepping" << std::flush;
  for(float k=-5; k<100; k+=0.5) {
   k = int(k*4.0)/4.0;
   std::cerr << "." << std::flush;
   count = 0.0;
   float total = 0.0;
   for(float i=-200; i<=200; i+=GRID*0.5) {
    for(float j=-200; j<=200; j+=GRID*0.5) {
     r.x = p.x + v1.x*i + v2.x*j + v.x*k;
     r.y = p.y + v1.y*i + v2.y*j + v.y*k;
     r.z = p.z + v1.z*i + v2.z*j + v.z*k;
     if(r.x >= XMIN && r.x <= XMAX &&
	r.y >= YMIN && r.y <= YMAX &&
	r.z >= ZMIN && r.z <= ZMAX) {
       pt = xyz2pt(r.x, r.y, r.z);
       if(pt >= 0 && pt < DXYZ) {
         total++;
         if(grid[pt]) { count++; }
       }
     }
    }
   }

   //cerr << "crossSection:  " << k << " " << count << " of " << total << std::endl;
   std::cout << k << "\t" << count*mult << std::endl;
  }
   std::cerr << std::endl;

  return count;
};

/*********************************************
**********************************************
              STRING FUNCTIONS
**********************************************
*********************************************/

/*********************************************/
void padLeft(char a[], int n) {
  //len = 1 , n = 5
  int len = strlen(a);
  if(len < n) {
    for(int i=len; i<n; i++) {
      a[i] = ' ';
    }
    for(int i=1; i<=len; i++) {
      a[n-i] = a[len-i]; //a[5] = a[1], a[4] = a[0]
      a[len-i] = ' ';
    }
    a[n] = '\0';
  }
};

/*********************************************/
void padRight(char a[], int n) {
  int len = strlen(a);
  if(len < n) {
    while (len < n) {
      a[len] = ' ';
      len++;
    }
    a[len] = '\0';
  }
};

/*********************************************/
void printBar () {
  std::cerr << "|----+----+----+----+----+---<>---+----+----+----+----+----|" << std::endl;
  return;
};

/*********************************************/
void printVol (int vox) {
  //long double vol = vox*GRIDVOL;
  float tenp;
  tenp = 1000000.0;
  if(float(vox)*GRIDVOL > tenp) {
    int cut = int((float(vox)/tenp)*GRIDVOL);
      std::cerr << cut << "," << std::flush;
    vox = vox - int(cut*tenp/GRIDVOL);
  }
  tenp = 1000.0;
  if(float(vox)*GRIDVOL > tenp) {
    int cut = int((float(vox)/tenp)*GRIDVOL);
    if(cut >= 100) {
      std::cerr << cut << "," << std::flush;
    } else if(cut >= 10) {
      std::cerr << "0" << cut << "," << std::flush;
    } else if(cut >= 1) {
      std::cerr << "00" << cut << std::flush;
    } else {
      std::cerr << "000" << std::flush;
    }
    vox = vox - int(cut*tenp/GRIDVOL);
  }
  double cut = float(vox)*GRIDVOL;
  if(cut >= 100) {
    std::cerr << cut << std::flush;
  } else if(cut >= 10) {
    std::cerr << "0" << cut << std::flush;
  } else if(cut >= 1) {
    std::cerr << "00" << cut << std::flush;
  } else {
    std::cerr << "000" << std::flush;
  }
  return;
};

/*********************************************/
void printVolCout (int vox) {
  long double vol = float(vox)*GRIDVOL;
  long double tenp;
  tenp = 1000000.0; //Millions
  if(float(vox)*GRIDVOL > tenp) {
    int cut = int((float(vox)/tenp)*GRIDVOL);
      std::cout << cut << std::flush;
    vox = vox - int(cut*tenp/GRIDVOL);
  }
  tenp = 1000.0; //Thousands
  if(float(vox)*GRIDVOL > tenp) {
    int cut = int((float(vox)/tenp)*GRIDVOL);
    if(cut >= 100 || vol < 100000) {
      std::cout << cut << std::flush;
    } else if(cut >= 10) {
      std::cout << "0" << cut << std::flush;
    } else if(cut >= 1) {
      std::cout << "00" << cut << std::flush;
    } else {
      std::cout << "000" << std::flush;
    }
    vox = vox - int(cut*tenp/GRIDVOL);
  }
  double cut = float(vox)*GRIDVOL;
  if(cut >= 100 || vol < 1000) {
    std::cout << cut << std::flush;
  } else if(cut >= 10) {
    std::cout << "0" << cut << std::flush;
  } else if(cut >= 1) {
    std::cout << "00" << cut << std::flush;
  } else {
    std::cout << "000" << std::flush;
  }
  std::cout << "\t" << std::flush;
  return;
};

/*********************************************/
void basename (char str[], char base[]) {
  int loc=0;
  for(int i=0; str[i] != '\0'; i++) {
    if (str[i] == '/') {
      loc = i + 1;
    }
  }
  int max=0;
  for(int i=loc; str[i] != '\0'; i++) {
    base[i-loc] = str[i];
    max = i + 1 - loc;
  }
  base[max] = '\0';
  return;
}

/*********************************************
**********************************************
              SURFACE AREA
**********************************************
*********************************************/

/*********************************************/
int countEdgePoints (gridpt grid[]) {
  //Initialize Variables
  int type; // for return variables
  int edges = 0; //count types
  float count = 0;
  const float cat = DZ/60.0;
  float cut = cat;

  //cerr << "DXY: " << DXY << "\tDX: " << DX << std::endl;
  std::cerr << "Count Surface Voxels..." << std::endl;
  printBar();

  int sk=-1, sj=-1;
  for(int k=0; k<DXYZ; k+=DXY) {
    sk++;
    count++;
    if(count > cut) {
      std::cerr << "^" << std::flush;
      cut += cat;
    }
    sj=-1;
    for(int j=0; j<DXY; j+=DX) {
      sj++;
      for(int i=0; i<DX; i++) {
        int pt = i+j+k;
        if(grid[pt]) {
          type = classifyEdgePoint(pt,grid);
          if (type != 0)
	          edges++;
        }
  } } }
  return edges;
}


/*********************************************/
float surface_area (gridpt grid[]) {
  //Initialize Variables
  float surf=0.0;
  const float wt[] = { 0.0, 0.894, 1.3409, 1.5879, 4.0, 2.6667,
		      3.3333, 1.79, 2.68, 4.08, 0.0}; //weighting factors
/*
  wt[0]=0.0;   wt[1]=0.894; wt[2]=1.3409; wt[3]=1.5879;
  wt[4]=4.0;   wt[5]=2.6667; wt[6]=3.3333;
  wt[7]=1.79;  wt[8]=2.68;   wt[9]=4.08;
*/
  int type; // for return variables
  int edges[10]; //count types
  for(int i=0; i<=9; i++) { edges[i] = 0; }
  float count = 0;
  const float cat = DZ/60.0;
  float cut = cat;

  //cerr << "DXY: " << DXY << "\tDX: " << DX << std::endl;
  std::cerr << "Count Surface Voxels for Surface Area..." << std::endl;
  printBar();

  int sk=-1, sj=-1;
  for(int k=0; k<DXYZ; k+=DXY) {
    sk++;
    count++;
    if(count > cut) {
      std::cerr << "^" << std::flush;
      cut += cat;
    }
    sj=-1;
    for(int j=0; j<DXY; j+=DX) {
      sj++;
      for(int i=0; i<DX; i++) {
        int pt = i+j+k;
        if(grid[pt]) {
          //cerr << pt << "::";
          type = classifyEdgePoint(pt,grid);
          //surf += wt[type];
          edges[type]++;
        }
  } } }
  std::cerr << std::endl << "EDGES: ";
  float totedge=0;
  for(int i=1; i<=9; i++) {
    totedge += float(edges[i]);
  }
  for(int i=1; i<=9; i++) {
    std::cerr << "s" << i << ":" << int(float(100000*edges[i])/totedge)/1000.0 << " ";
    surf += edges[i]*wt[i];
  }
  std::cerr << std::endl << std::endl;
  return surf*GRID*GRID;
}

/*********************************************/
int classifyEdgePoint (const int pt, gridpt grid[]) {
  //look at neighbors
  short int count=0;
  short int nb=0; //num of empty neighbors
  for(int di=-1; di<=1; di+=2) {
    count++;
    if(!grid[pt+di]) {
      nb++;
    }
  }
  for(int dj=-DX; dj<=DX; dj+=2*DX) {
    count++;
    if(!grid[pt+dj]) {
      nb++;
    }
  }
  for(int dk=-DXY; dk<=DXY; dk+=2*DXY) {
    count++;
    if(!grid[pt+dk]) {
      nb++;
    }
  }
  //RETURN BASED ON NUMBER OF EMPTY NEIGHBORS (nb)
  if(count != 6) {
    std::cerr << "classifyEdgePoint count " << count << " != 6" << std::endl;
  }
  if(pt < DXY) {
    std::cerr << "pt < DXY " << pt << " < " << DXY << std::endl;
  }
  //if(pt + DXY > NUMBINS) {
  //cerr << "pt > NUMBINS " << pt << " > " << NUMBINS << std::endl;
  //}
  if(nb == 0 || nb == 1) {
    return nb;
  } else if(nb == 2) {
//CHECK FOR BOTH CASES
    //check for cross gaps
    if(!grid[pt+1] && !grid[pt-1]) {
      return 7;
    }
    if(!grid[pt+DX] && !grid[pt-DX]) {
      return 7;
    }
    if(!grid[pt+DXY] && !grid[pt-DXY]) {
      return 7;
    }
    //the normal case
    return 2;
  } else if(nb == 3) {
//CHECK FOR BOTH CASES
    //check for cross gaps
    if(!grid[pt+1] && !grid[pt-1]) {
      return 4;
    }
    if(!grid[pt+DX] && !grid[pt-DX]) {
      return 4;
    }
    if(!grid[pt+DXY] && !grid[pt-DXY]) {
      return 4;
    }
    //the normal case
    return 3;
  } else if(nb == 4) {
//CHECK FOR BOTH CASES
    //check for cross fills
    if(grid[pt+1] && grid[pt-1]) {
      return 8;
    }
    if(grid[pt+DX] && grid[pt-DX]) {
      return 8;
    }
    if(grid[pt+DXY] && grid[pt-DXY]) {
      return 8;
    }
    //the normal case
    return 5;
  } else if(nb == 5) {
    return 6;
  } else if(nb == 6) {
    return 9;
  }
  std::cerr << "classifyEdgePoint neighbor count " << nb << " is weird!" << std::endl;
  return 0;
};

/*********************************************
**********************************************
              NEW FEATURES
**********************************************
*********************************************/

/*********************************************/
int fill_cavities(gridpt grid[]) {

  gridpt *cavACC=NULL;
  cavACC = (gridpt*) std::malloc (NUMBINS);
  bounding_box(grid,cavACC);

//Create inverse access map
  subt_Grids(cavACC,grid); //modifies cavACC
  //int achanACC_voxels = countGrid(cavACC);

//Get first point
  bool stop = 1; int firstpt = 0;
  for(unsigned int pt=0; pt<NUMBINS && stop; pt++) {
    if(cavACC[pt]) { stop = 0; firstpt = pt;}
  }
  std::cerr << "FIRST POINT: " << firstpt << std::endl;
//LAST POINT
  stop = 1; int lastpt = 0;
  for(unsigned int pt=NUMBINS-10; pt>0 && stop; pt--) {
    if(cavACC[pt]) { stop = 0; lastpt = pt;}
  }
  std::cerr << "LAST  POINT: " << lastpt << std::endl;

//Pull channels out of inverse access map
  gridpt *chanACC=NULL;
  chanACC = (gridpt*) std::malloc (NUMBINS);
  zeroGrid(chanACC);
  get_Connected_Point(cavACC,chanACC,firstpt); //modifies chanACC
  get_Connected_Point(cavACC,chanACC,lastpt); //modifies chanACC
  //int chanACC_voxels = countGrid(chanACC);

//Subtract channels from access map leaving cavities
  subt_Grids(cavACC,chanACC); //modifies cavACC
  std::free (chanACC);
  int cavACC_voxels = countGrid(cavACC);


  int grid_before = countGrid(grid);
//Fill Cavities in grid[];
  #pragma omp parallel for
  for(unsigned int pt=0; pt<NUMBINS; pt++) {
    if(cavACC[pt]) { grid[pt]=1; }
  }
  int grid_after = countGrid(grid);
  std::free (cavACC);

  std::cerr << std::endl << "CAVITY VOLUME: ";
  printVol(cavACC_voxels);
  std::cerr << std::endl << "BEFORE VOLUME: ";
  printVol(grid_before);
  std::cerr << std::endl << "AFTER VOLUME:  ";
  printVol(grid_after);
  std::cerr << std::endl << "DIFFERENCE:    ";
  printVol(grid_after-grid_before);
  std::cerr << std::endl << std::endl;

  return cavACC_voxels;
};

/*********************************************/
void determine_MinMax(const gridpt grid[], int minmax[]) {
  //minmax MUST be an array of length 6
  if (DEBUG > 0)
    std::cerr << "Determining Minima and Maxima..." << std::flush;
  int xmin=DX, ymin=DXY, zmin=DXYZ;
  int xmax=0, ymax=0, zmax=0;
  for(int k=0; k<DXYZ; k+=DXY) {
    for(int j=0; j<DXY; j+=DX) {
      for(int i=0; i<DX; i++) {
        int pt = i+j+k;
        if(grid[pt]) {
          if(i < xmin) { xmin = i; }
          if(j < ymin) { ymin = j; }
          if(k < zmin) { zmin = k; }
          if(i > xmax) { xmax = i; }
          if(j > ymax) { ymax = j; }
          if(k > zmax) { zmax = k; }
        }
  } } }
  if (DEBUG > 0)
    std::cerr << "  DONE" << std::endl;;
  minmax[0] = xmin;
  minmax[1] = ymin;
  minmax[2] = zmin;
  minmax[3] = xmax;
  minmax[4] = ymax;
  minmax[5] = zmax;
  if (DEBUG > 0) {
    std::cerr << "X: " << xmin << " <> " << xmax << std::endl;
    std::cerr << "Y: " << ymin/DX << " <> " << ymax/DX << std::endl;
    std::cerr << "Z: " << zmin/DXY << " <> " << zmax/DXY << std::endl;
  }
  return;
};

/*********************************************/
int makerbot_fill(gridpt ingrid[], gridpt outgrid[]) {
  /*
  ** Since you cannot see inside a 3D print,
  ** points that are invisible in ingrid are
  ** converted to outgrid points
  ** This way the printer does not have switch
  ** colors on the invisible parts of the model

  ** in grid and out grid should not intersect
  ** changes both in grid and out grid
  ** in grid points are converted to out grid points
  */

  /*
  int ijk2pt(int i, int j, int k);
  void pt2ijk(int pt, int &i, int &j, int &k);
  */


  int minmax[6];
  determine_MinMax(outgrid, minmax);
  std::cerr << "makerbot fill" << std::endl;
//Get first point
  unsigned int iter = 0;
  unsigned int changed = 1;
  unsigned int totalChanged = 0;
  while (changed > 0) {
    changed = 0;
    iter++;
    #pragma omp parallel for
    for(unsigned int pt=0; pt<NUMBINS-1; pt++) {
      if(ingrid[pt]) {
        //if (isContainedPoint(pt, ingrid, outgrid, minmax)) {
      	if (!isNearEdgePoint(pt, ingrid, outgrid)) {
          ingrid[pt] = 0;
      	  outgrid[pt] = 1;
      	  #pragma omp atomic
      	  changed++;
      	} //}
      }
    }
    totalChanged += changed;
    std::cerr << "ITER: " << iter << " :: changed " << changed << std::endl;
  }
  std::cerr << "Total Changed: " << totalChanged << std::endl;

  return totalChanged;
};

/*********************************************/
bool isContainedPoint (const int pt, gridpt ingrid[], gridpt outgrid[], int minmax[]) {

  /*
  int ijk2pt(int i, int j, int k);
  void pt2ijk(int pt, int &i, int &j, int &k);
  */
  int ipt,jpt,kpt;
  int index, newpt;
  bool filled;
  int xmin, ymin, zmin;
  int xmax, ymax, zmax;
  xmin = minmax[0];
  ymin = minmax[1]/DX;
  zmin = minmax[2]/DXY;
  xmax = minmax[3];
  ymax = minmax[4]/DX;
  zmax = minmax[5]/DXY;

  pt2ijk(pt, ipt, jpt, kpt);
  unsigned int checked = 0;

  unsigned int fillCount = 0;
// X axis
  filled = 0;
  for(index=1; index<ipt-xmin  && filled==0; index++) {
    newpt = ijk2pt(ipt - index, jpt, kpt);
    checked++;
    if (outgrid[newpt] == 1) {
  	  fillCount++; filled=1;
    } else if (ingrid[newpt] == 0) {
      filled=1;
    }
  }
  filled = 0;
  for(index=1; index<xmax-ipt && filled==0; index++) {
    newpt = ijk2pt(ipt + index, jpt, kpt);
    checked++;
    if (outgrid[newpt] == 1) {
  	  fillCount++; filled=1;
    } else if (ingrid[newpt] == 0) {
      filled=1;
    }
  }

// Y axis
  filled = 0;
  for(index=1; index<jpt-ymin  && filled==0; index++) {
    newpt = ijk2pt(ipt, jpt - index, kpt);
    checked++;
    if (outgrid[newpt] == 1) {
  	  fillCount++; filled=1;
    } else if (ingrid[newpt] == 0) {
      filled=1;
    }
  }
  filled = 0;
  for(index=1; index<ymax-jpt && filled==0; index++) {
    newpt = ijk2pt(ipt, jpt + index, kpt);
    checked++;
    if (outgrid[newpt] == 1) {
  	  fillCount++; filled=1;
    } else if (ingrid[newpt] == 0) {
      filled=1;
    }
  }

// Z axis
  filled = 0;
  for(index=1; index<kpt-zmin && filled==0; index++) {
    newpt = ijk2pt(ipt, jpt, kpt - index);
    checked++;
    if (outgrid[newpt] == 1) {
  	  fillCount++; filled=1;
    } else if (ingrid[newpt] == 0) {
      filled=1;
    }
  }
  filled = 0;
  for(index=1; index<zmax-kpt && filled==0; index++) {
    newpt = ijk2pt(ipt, jpt, kpt + index);
    checked++;
    if (outgrid[newpt] == 1) {
  	  fillCount++; filled=1;
    } else if (ingrid[newpt] == 0) {
      filled=1;
    }
  }
  if (fillCount >= 3) {
    return 1;
  }
  return 0;
};

/*********************************************/
bool isNearEdgePoint (const int pt, gridpt ingrid[], gridpt outgrid[]) {
  int i,j,k;
  pt2ijk(pt, i, j, k);

//provides indexes (i,j,k) of grid where ijk2pt(i,j,k) = gridpt
  const float R = 3.0/GRID; //Aug 19: correction for oversize
  const int r = int(R+1);
  const float cutoff = R*R;
  int nri,nrj,nrk,pri,prj,prk;
//overflow checks (let's not go off the grid)
  if(i < r) { nri = -i; } else { nri = -r;}
  if(j < r) { nrj = -j; } else { nrj = -r;}
  if(k < r) { nrk = -k; } else { nrk = -r;}
  if(i + r >= DX) { pri = DX-i-1; } else { pri = r;}
  if(j + r >= DY) { prj = DY-j-1; } else { prj = r;}
  if(k + r >= DZ) { prk = DZ-k-1; } else { prk = r;}
  float distsq;
  int ind;
  // do not parallelize done in previous step
  for(int di=nri; di<=pri; di++) {
  for(int dj=nrj; dj<=prj; dj++) {
  for(int dk=nrk; dk<=prk; dk++) {
     ind = ijk2pt(i+di,j+dj,k+dk);
     if(!ingrid[ind] && !outgrid[ind]) {
       distsq = di*di + dj*dj + dk*dk;
       if(distsq < cutoff) {
         return 1;
       }
     }
  }}}
  return 0;
};


/*********************************************/
int bounding_box(gridpt grid[], gridpt bbox[]) {
  //find min x,y,z and max x,y,z
  zeroGrid(bbox);


//PART I: Determine Extrema
  int minmax[6];
  determine_MinMax(grid, minmax);
  int xmin, ymin, zmin;
  int xmax, ymax, zmax;
  xmin = minmax[0];
  ymin = minmax[1];
  zmin = minmax[2];
  xmax = minmax[3];
  ymax = minmax[4];
  zmax = minmax[5];

//Grow by one
/*
  xmin-=1;
  ymin-=DX;
  zmin-=DXY;
  xmax+=1;
  ymax+=DX;
  zmax+=DXY;
*/

//PART II: FILL BOX
  int vol=0;
  int count = 0;
  const float cat = DZ/60.0;
  float cut = cat;
  std::cerr << "Fill Box..." << std::endl;
  printBar();
  for(int k=zmin; k<=zmax; k+=DXY) {
    count++;
    if(count > cut) {
      std::cerr << "^" << std::flush;
      cut += cat;
    }
    for(int j=ymin; j<=ymax; j+=DX) {
      for(int i=xmin; i<=xmax; i++) {
        bbox[i+j+k] = 1;
        vol++;
  } } }
  std::cerr << std::endl << "BOX VOXELS: ";
  printVol(vol);
  std::cerr << std::endl << std::endl;

  return vol;
};
