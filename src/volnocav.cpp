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
	const int natoms, char file[], char ezdfile[], char pdbfile[], char mrcfile[]);

int main(int argc, char *argv[]) {
  cerr << endl;
// ****************************************************
// USER INPUT
// ****************************************************

  COMPILE_INFO;
  CITATION;

  char file[256]; file[0] = '\0';
  char ezdfile[256]; ezdfile[0] = '\0';
  char pdbfile[256]; pdbfile[0] = '\0';
  char mrcfile[256]; mrcfile[0] = '\0';
  double PROBE = 1.5;

  while(argc > 1 && argv[1][0] == '-') {
    if(argv[1][1] == 'i') {
      sprintf(file,&argv[2][0]);
    } else if(argv[1][1] == 'g') {
      GRID = atof(&argv[2][0]);
    } else if(argv[1][1] == 'p') {
      PROBE = atof(&argv[2][0]);
    } else if(argv[1][1] == 'e') {
      sprintf(ezdfile,&argv[2][0]);
    } else if(argv[1][1] == 'm') {
      sprintf(mrcfile,&argv[2][0]);
    } else if(argv[1][1] == 'o') {
      sprintf(pdbfile,&argv[2][0]);
    } else if(argv[1][1] == 'h') {
      cerr << "./Cavities.exe -i <file> -g <grid spacing> -p <probe radius> " << endl
        << "\t-e <EZD outfile> -o <PDB outfile> -m <MRC outfile>" << endl;
      cerr << "Cavities.exe -- Extracts the cavities for a given probe radius" << endl;
      cerr << endl;
      return 1;
    }
    --argc; --argc;
    ++argv; ++argv;
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
  cerr << "Input file:   " << file << endl;
//FIRST PASS, MINMAX
  int numatoms = read_NumAtoms(file);
//CHECK LIMITS & SIZE
  assignLimits();

// ****************************************************
// STARTING FILE READ-IN
// ****************************************************


  gridpt *shellACC=NULL;
  shellACC = (gridpt*) malloc (NUMBINS);
  fill_AccessGrid_fromFile(numatoms,PROBE,file,shellACC);
  int voxels1 = countGrid(shellACC);
  fill_cavities(shellACC);
  int voxels2 = countGrid(shellACC);
  cerr << "Fill Cavities: " << voxels2 - voxels1 << " voxels filled" << endl;

  gridpt *EXCgrid=NULL;
  EXCgrid = (gridpt*) malloc (NUMBINS);
  trun_ExcludeGrid(PROBE,shellACC,EXCgrid);

  free (shellACC);
  int voxels = countGrid(EXCgrid);



  long double surf;
  surf = surface_area(EXCgrid);

  if(mrcfile[0] != '\0') {
    writeMRCFile(EXCgrid, mrcfile);
  }
  if(ezdfile[0] != '\0') {
    write_HalfEZD(EXCgrid,ezdfile);
  }
  if(pdbfile[0] != '\0') {
    write_SurfPDB(EXCgrid,pdbfile);
  }

//RELEASE TEMPGRID
  free (EXCgrid);

  cout << PROBE << "\t" << GRID << "\t" << flush;
  printVolCout(voxels);
  cout << "\t" << surf << "\t#" << file << endl;

  cerr << endl << "Program Completed Sucessfully" << endl << endl;
  return 0;



};
