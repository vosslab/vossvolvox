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

int main(int argc, char *argv[]) {
  cerr << endl;

  COMPILE_INFO;
  CITATION;

// ****************************************************
// INITIALIZATION
// ****************************************************

//HEADER INFO
  char file[256]; file[0] = '\0';
  char ezdfile[256]; ezdfile[0] = '\0';
  char pdbfile[256]; pdbfile[0] = '\0';
  char mrcfile[256]; mrcfile[0] = '\0';
  double PROBE=0.0;

  while(argc > 1 && argv[1][0] == '-') {
    if(argv[1][1] == 'i') {
      sprintf(file,&argv[2][0]);
    } else if(argv[1][1] == 'o') {
      sprintf(pdbfile,&argv[2][0]);
    } else if(argv[1][1] == 'e') {
      sprintf(ezdfile,&argv[2][0]);
    } else if(argv[1][1] == 'm') {
      sprintf(mrcfile,&argv[2][0]);
    } else if(argv[1][1] == 'g') {
      GRID = atof(&argv[2][0]);
    } else if(argv[1][1] == 'h') {
      cerr << "./VDW.exe -i <file> -g <gridspacing> " << endl
        << "\t-e <EZD outfile> -o <PDB outfile> -m <MRC outfile>" << endl;
      cerr << "VDW.exe -- Calculate the VDW volume and surface area" << endl;
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
  cerr << "Resolution:      " << int(1000.0/float(GRIDVOL))/1000.0 << " voxels per A^3" << endl;
  cerr << "Resolution:      " << int(11494.0/float(GRIDVOL))/1000.0 << " voxels per water molecule" << endl;
  cerr << "Input file:   " << file << endl;

//FIRST PASS, MINMAX
  int numatoms = read_NumAtoms(file);

//CHECK LIMITS & SIZE
  assignLimits();

// ****************************************************
// STARTING FIRST FILE
// ****************************************************
//READ FILE INTO SASGRID
  gridpt *EXCgrid;
  EXCgrid = (gridpt*) malloc (NUMBINS);
  if (EXCgrid==NULL) { cerr << "GRID IS NULL" << endl; exit (1); }
  zeroGrid(EXCgrid);
  int voxels;
  if(PROBE > 0.0) { 
    voxels = get_ExcludeGrid_fromFile(numatoms,PROBE,file,EXCgrid);
  } else {
    voxels = fill_AccessGrid_fromFile(numatoms,0.0,file,EXCgrid);
  }
  long double surf;
  surf = surface_area(EXCgrid);

  if(ezdfile[0] != '\0') {
    write_HalfEZD(EXCgrid, ezdfile);
  }
  if(pdbfile[0] != '\0') {
    write_SurfPDB(EXCgrid, pdbfile);
  }
  if(mrcfile[0] != '\0') {
    writeMRCFile(EXCgrid, mrcfile);
  }

//RELEASE TEMPGRID
  free (EXCgrid);

  cout << PROBE << "\t" << GRID << "\t" << flush;
  printVolCout(voxels);
  cout << "\t" << surf << "\t#" << file << endl;

  cerr << endl << "Program Completed Sucessfully" << endl << endl;
  return 0;
};

