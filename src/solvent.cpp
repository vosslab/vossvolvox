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
  double BIGPROBE=9.0;
  double SMPROBE=1.5;
  double TRIMPROBE=1.5; //HEADER INFO
 
  while(argc > 1 && argv[1][0] == '-') {
    if(argv[1][1] == 'i') {
      sprintf(file,&argv[2][0]);   
    } else if(argv[1][1] == 's') {
      SMPROBE = atof(&argv[2][0]);
    } else if(argv[1][1] == 'b') {
      BIGPROBE = atof(&argv[2][0]);
    } else if(argv[1][1] == 't') {
      TRIMPROBE = atof(&argv[2][0]);
    } else if(argv[1][1] == 'e') {
      sprintf(ezdfile,&argv[2][0]);
    } else if(argv[1][1] == 'm') {
      sprintf(mrcfile,&argv[2][0]);
    } else if(argv[1][1] == 'o') {
      sprintf(pdbfile,&argv[2][0]);
    } else if(argv[1][1] == 'g') {
      GRID = atof(&argv[2][0]);
    } else if(argv[1][1] == 'h') {
      cerr << "./Solvent.exe -i <file> -s <sm_probe_rad> -b <big_probe_rad>" << endl
        << "\t-t <trim_probe_rad> -g <grid spacing> " << endl
        << "\t-e <EZD outfile> -o <PDB outfile> -m <MRC outfile>" << endl;
      cerr << "Solvent.exe -- Extracts the all of the solvent" << endl;
      cerr << endl;
      return 1;
    }
    --argc; --argc;
    ++argv; ++argv;
  }
 
//INITIALIZE GRID
  finalGridDims(BIGPROBE);

//HEADER CHECK
  if(SMPROBE > BIGPROBE) { cerr << "ERROR: SMPROBE > BIGPROBE" << endl; return 1; }
  cerr << "Small Probe Radius: " << SMPROBE << endl;
  cerr << " Big  Probe Radius: " << BIGPROBE << endl;
  cerr << "Trim  Probe Radius: " << TRIMPROBE << endl;
  cerr << "Grid Spacing: " << GRID << endl;
  cerr << "Input file:   " << file << endl;
  cerr << "Resolution:   " << int(1000.0/float(GRIDVOL))/1000.0 << " voxels per A^3" << endl;
  cerr << "Resolution:   " << int(11494.0/float(GRIDVOL))/1000.0 << " voxels per water molecule" << endl;


//FIRST PASS, MINMAX
  int numatoms = read_NumAtoms(file);

//CHECK LIMITS & SIZE
  assignLimits();

// ****************************************************
// STARTING LARGE PROBE
// **************************************************** 
  gridpt *biggrid;
  biggrid = (gridpt*) malloc (NUMBINS);
  if (biggrid==NULL) { cerr << "GRID IS NULL" << endl; return 1; }
  zeroGrid(biggrid);
  int bigvox;
  if(BIGPROBE > 0.0) { 
    bigvox = get_ExcludeGrid_fromFile(numatoms,BIGPROBE,file,biggrid);
  } else {
    cerr << "BIGPROBE <= 0" << endl;
    return 1;
  }


// ****************************************************
// TRIM LARGE PROBE SURFACE
// ****************************************************
  gridpt *trimgrid;
  trimgrid = (gridpt*) malloc (NUMBINS);
  if (trimgrid==NULL) { cerr << "GRID IS NULL" << endl; return 1; }
  copyGrid(biggrid,trimgrid);
  if(TRIMPROBE > 0) {
    trun_ExcludeGrid(TRIMPROBE,biggrid,trimgrid);
  }
  free (biggrid);

  //cout << "bg_prb\tsm_prb\tgrid\texcvol\tsurf\taccvol\tfile" << endl;

// ****************************************************
// STARTING SMALL PROBE
// ****************************************************
    gridpt *smgrid;
    smgrid = (gridpt*) malloc (NUMBINS);
    if (smgrid==NULL) { cerr << "GRID IS NULL" << endl; return 1; }
    zeroGrid(smgrid);
    int smvox;
    smvox = fill_AccessGrid_fromFile(numatoms,SMPROBE,file,smgrid);

// ****************************************************
// GETTING ACCESSIBLE CHANNELS
// ****************************************************
    gridpt *solventACC;
    solventACC = (gridpt*) malloc (NUMBINS);
    if (solventACC==NULL) { cerr << "GRID IS NULL" << endl; return 1; }
    copyGrid(trimgrid,solventACC); //copy trimgrid into solventACC
    subt_Grids(solventACC,smgrid); //modify solventACC
    free (smgrid);

// ***************************************************
// GETTING CONTACT CHANNEL
// ***************************************************
    gridpt *solventEXC;
    solventEXC = (gridpt*) malloc (NUMBINS);
    if (solventEXC==NULL) { cerr << "GRID IS NULL" << endl; return 1; }
    int solventACCvol = copyGrid(solventACC,solventEXC);
    cerr << "Accessible Channel Volume  ";
    printVol(solventACCvol);
    grow_ExcludeGrid(SMPROBE,solventACC,solventEXC);
    free (solventACC);

//limit growth to inside trimgrid
    intersect_Grids(solventEXC,trimgrid); //modifies solventEXC
    free (trimgrid);

// ***************************************************
// OUTPUT RESULTS
// ***************************************************
    cout << BIGPROBE << "\t" << SMPROBE << "\t" << GRID << "\t" << flush;
    int solventEXCvol = countGrid(solventEXC);
    printVolCout(solventEXCvol);
    long double surf = surface_area(solventEXC);
    cout << "\t" << surf << "\t" << flush;
    //printVolCout(solventACCvol);
    cout << file << endl;
    if(pdbfile[0] != '\0') {
      write_SurfPDB(solventEXC, pdbfile);
    }
    if(ezdfile[0] != '\0') {
      write_HalfEZD(solventEXC, ezdfile);
    }
    if(mrcfile[0] != '\0') {
      writeMRCFile(solventEXC, mrcfile);
    }

    free (solventEXC);

  cerr << endl << "Program Completed Sucessfully" << endl << endl;
  return 0;
};
