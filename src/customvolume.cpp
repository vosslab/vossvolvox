#include <iostream>
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
  cerr << endl;

  COMPILE_INFO;
  CITATION;

// ****************************************************
// INITIALIZATION
// ****************************************************

//HEADER INFO
  char rnafile[256]; rnafile[0] = '\0';
  char aminofile[256]; aminofile[0] = '\0';
  double PROBE=10.0;

  while(argc > 1 && argv[1][0] == '-') {
    if(argv[1][1] == 'r') {
      sprintf(rnafile,&argv[2][0]);
    } else if(argv[1][1] == 'a') {
      sprintf(aminofile,&argv[2][0]);
    } else if(argv[1][1] == 'p') {
      PROBE = atof(&argv[2][0]);
    } else if(argv[1][1] == 'g') {
      GRID = atof(&argv[2][0]);
    } else if(argv[1][1] == 'h') {
      cerr << "./Volume.exe -i <file> -g <gridspacing> -p <probe_rad> " << endl;
      cerr << "Volume.exe -- Calculate the volume and surface area for any probe radius" << endl;
      cerr << endl;
      return 1;
    }
    --argc; --argc;
    ++argv; ++argv;
  }


//INITIALIZE GRID
  finalGridDims(PROBE);

//HEADER CHECK
  cerr << "Probe Radius: " << PROBE << endl;
  cerr << "Grid Spacing: " << GRID << endl;
  cerr << "Resolution:   " << int(1000.0/float(GRIDVOL))/1000.0 << " voxels per A^3" << endl;
  cerr << "Resolution:   " << int(11494.0/float(GRIDVOL))/1000.0 << " voxels per water molecule" << endl;
  cerr << "RNA file:     " << rnafile << endl;
  cerr << "Amino file:   " << aminofile << endl;

//FIRST PASS, MINMAX
  int rnanumatoms = read_NumAtoms(rnafile);
  int aminonumatoms = read_NumAtoms(aminofile);

//CHECK LIMITS & SIZE
  assignLimits();

// ****************************************************
// STARTING FIRST FILE
// ****************************************************
//READ FILE INTO RNAgrid
  gridpt *RNAgrid;
  RNAgrid = (gridpt*) malloc (NUMBINS);
  if (RNAgrid==NULL) { cerr << "GRID IS NULL" << endl; exit (1); }
  zeroGrid(RNAgrid);
  int rnavoxels = get_ExcludeGrid_fromFile(rnanumatoms,PROBE,rnafile,RNAgrid);

//READ FILE INTO AminoGrid
  gridpt *AminoGrid;
  AminoGrid = (gridpt*) malloc (NUMBINS);
  if (AminoGrid==NULL) { cerr << "GRID IS NULL" << endl; exit (1); }
  zeroGrid(AminoGrid);
  int aminovoxels = get_ExcludeGrid_fromFile(aminonumatoms,PROBE,aminofile,AminoGrid);

//Subtract the protein grid
  subt_Grids(RNAgrid, AminoGrid);

  trimYAxis(AminoGrid);
  trimYAxis(RNAgrid);

  writeMRCFile(RNAgrid, "rna.mrc");
  writeMRCFile(AminoGrid, "amino.mrc");

//RELEASE TEMPGRID
  free (AminoGrid);
  free (RNAgrid);

  cerr << endl << "Program Completed Sucessfully" << endl << endl;
  return 0;
};

