#include <iostream>
#include "utils.h"

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

int getCavitiesBothMeth(const float probe, gridpt shellACC[], gridpt shellEXC[],
	const int natoms, char file1[], char file2[], char mrcfile1[], char mrcfile2[]);

int main(int argc, char *argv[]) {
  cerr << endl;
// ****************************************************
// USER INPUT
// ****************************************************

  COMPILE_INFO;
  CITATION;

  char file1[256]; file1[0] = '\0';
  char mrcfile1[256]; mrcfile1[0] = '\0';
  char file2[256]; file2[0] = '\0';
  char mrcfile2[256]; mrcfile2[0] = '\0';
  double PROBE1 = -1;
  double PROBE2 = -1;  
  unsigned int merge=0, fill=0;
  
  while(argc > 1 && argv[1][0] == '-') {
	if(argv[1][1] == 'g') {
      GRID = atof(&argv[2][0]);
    } else if(argv[1][1] == 'p' && argv[1][2] == '1') {
      PROBE1 = atof(&argv[2][0]);
    } else if(argv[1][1] == 'p' && argv[1][2] == '2') {
      PROBE2 = atof(&argv[2][0]);      
    } else if(argv[1][1] == 'i'  && argv[1][2] == '1') {
      sprintf(file1,&argv[2][0]);
    } else if(argv[1][1] == 'i' && argv[1][2] == '2') {
      sprintf(file2,&argv[2][0]);
    } else if(argv[1][1] == 'm' && argv[1][2] == '1') {
      sprintf(mrcfile1,&argv[2][0]);
    } else if(argv[1][1] == 'm' && argv[1][2] == '2') {
      sprintf(mrcfile2,&argv[2][0]);
    } else if(argv[1][1] == 'm' && argv[1][2] == 'e') {
      merge = atoi(&argv[2][0]);
    } else if(argv[1][1] == 'f' && argv[1][2] == 'i') {
      fill = atoi(&argv[2][0]);      
    } else if(argv[1][1] == 'h') {
      cerr << "./TwoVol.exe -i1 <file1> -i2 <file2> -g <grid spacing> -p1 <probe 1 radius> " << endl
        << "\t-p2 <probe 2 radius> -m1 <MRC outfile 1> -m2 <MRC outfile 2> -merge <0,1,2> -fill <0,1,2>" << endl;
      cerr << "TwoVol.exe -- Produces two volumes on the same grid for 3d printing" << endl;
      cerr << endl;
      return 1;
    }
    --argc; --argc;
    ++argv; ++argv;
  }
  
  if (PROBE1 < 0 && PROBE2 > 0) {
  	PROBE1 = PROBE2;
  } else if (PROBE2 < 0 && PROBE1 > 0) {
    PROBE2 = PROBE1;
  } else if (PROBE1 < 0 && PROBE2 < 0) {
  	  cerr << "Error please define a probe radius, example: -p1 1.5" << endl;
      cerr << endl;
      return 1;    
  }
  double maxPROBE, minPROBE;
  if (PROBE1 > PROBE2) {
  	maxPROBE = PROBE1;
  	minPROBE = PROBE2;
  } else {
    maxPROBE = PROBE2;
    minPROBE = PROBE1;
  }
// ****************************************************
// INITIALIZATION
// ****************************************************

//INITIALIZE GRID
  finalGridDims(maxPROBE*2);
//HEADER CHECK
  cerr << "Grid Spacing: " << GRID << endl;
  cerr << "Resolution:      " << int(1000.0/float(GRIDVOL))/1000.0 << " voxels per A^3" << endl;
  cerr << "Resolution:      " << int(11494.0/float(GRIDVOL))/1000.0 << " voxels per water molecule" << endl;
  cerr << "Input file 1:   " << file1 << endl;
  cerr << "Input file 2:   " << file2 << endl;
  cerr << "DIMENSIONS:   " << DX << ", " << DY << ", " << DZ << endl;

//FIRST PASS, MINMAX
  int numatoms1 = read_NumAtoms(file1);
  int numatoms2 = read_NumAtoms(file2);

//CHECK LIMITS & SIZE
  assignLimits();

  gridpt *shellACC=NULL, *shellACC2=NULL, *EXCgrid1=NULL, *EXCgrid2=NULL;
  int voxels1, voxels2;

// ****************************************************
// STARTING FILE 1 READ-IN
// ****************************************************

  shellACC = (gridpt*) malloc (NUMBINS);
  fill_AccessGrid_fromFile(numatoms1,PROBE1,file1,shellACC);
  if(merge == 1) {
    shellACC2 = (gridpt*) malloc (NUMBINS);
    fill_AccessGrid_fromFile(numatoms2,minPROBE,file2,shellACC2);
    cerr << "Merge Volumes 1->2" << endl;
    merge_Grids(shellACC, shellACC2);
    free (shellACC2);
  }
  voxels1 = countGrid(shellACC);
  fill_cavities(shellACC);
  voxels2 = countGrid(shellACC);
  cerr << "Fill Cavities: " << voxels2 - voxels1 << " voxels filled" << endl;

  EXCgrid1 = (gridpt*) malloc (NUMBINS);
  trun_ExcludeGrid(PROBE1,shellACC,EXCgrid1);

  free (shellACC);
  
// ****************************************************
// STARTING FILE 2 READ-IN
// ****************************************************

  shellACC = (gridpt*) malloc (NUMBINS);
  fill_AccessGrid_fromFile(numatoms2,PROBE2,file2,shellACC);
  if(merge == 2) {
    shellACC2 = (gridpt*) malloc (NUMBINS);
    fill_AccessGrid_fromFile(numatoms2,minPROBE,file1,shellACC2);
    cerr << "Merge Volumes 2->1" << endl;
    merge_Grids(shellACC, shellACC2);
    free (shellACC2);
  }  
  voxels1 = countGrid(shellACC);
  fill_cavities(shellACC);
  voxels2 = countGrid(shellACC);
  cerr << "Fill Cavities: " << voxels2 - voxels1 << " voxels filled" << endl;

  EXCgrid2 = (gridpt*) malloc (NUMBINS);
  trun_ExcludeGrid(PROBE2,shellACC,EXCgrid2);

  free (shellACC);
  
// ****************************************************
// SUBTRACT AND SAVE
// ****************************************************

  //voxels1 = countGrid(EXCgrid1);

  cerr << "subtract grids" << endl;
  if(merge == 1) {
    subt_Grids(EXCgrid1, EXCgrid2); //modifies first grid
  } else {
    subt_Grids(EXCgrid2, EXCgrid1); //modifies first grid
  }
  
  cerr << "makerbot fill" << endl;
  if(fill == 1) {
  	makerbot_fill(EXCgrid2, EXCgrid1);
  } else if(fill == 2) {
    makerbot_fill(EXCgrid1, EXCgrid2);
  } else {
  	cerr << "no fill" << endl;
  }
  /* 
  ** makerbot_fill()
  ** Since you cannot see inside a 3D print, 
  ** points that are invisible in ingrid are
  ** converted to outgrid points
  ** This way the printer does not have switch 
  ** colors on the invisible (internal) parts of the model
  ** So outgrid gets bigger
  */

  if(mrcfile1[0] != '\0') {
    writeMRCFile(EXCgrid1, mrcfile1);
  }

//RELEASE TEMPGRID
  free (EXCgrid1);

  if(mrcfile2[0] != '\0') {
    writeMRCFile(EXCgrid2, mrcfile2);
  }

//RELEASE TEMPGRID
  free (EXCgrid2);

  //cout << PROBE1 << "\t" << PROBE2 << "\t" << GRID << "\t" << endl;


  cerr << endl << "Program Completed Sucessfully" << endl << endl;
  return 0;



};
