#include <stdlib.h>                   // for free, malloc, NULL
#include <iostream>                   // for char_traits, cerr, cout
#include "utils.h"                    // for endl, cerr, gridpt, copyGrid


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

printCompileInfo(argv[0]); // Replaces COMPILE_INFO;
printCitation(); // Replaces CITATION;

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
  double TRIMPROBE=4.0;
  double x=1000,y=1000,z=1000;

  while(argc > 1 && argv[1][0] == '-') {
    if(argv[1][1] == 'i') {
      sprintf(file,&argv[2][0]);
    } else if(argv[1][1] == 'b') {
      BIGPROBE = atof(&argv[2][0]);
    } else if(argv[1][1] == 's') {
      SMPROBE = atof(&argv[2][0]);
    } else if(argv[1][1] == 't') {
      TRIMPROBE = atof(&argv[2][0]);
    } else if(argv[1][1] == 'x') {
      x = atof(&argv[2][0]);
    } else if(argv[1][1] == 'y') {
      y = atof(&argv[2][0]);
    } else if(argv[1][1] == 'z') {
      z = atof(&argv[2][0]);
    } else if(argv[1][1] == 'i') {
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
      cerr << "./Channel.exe -i <file> -b <big_probe> -s <small_probe> -g <gridspace> " << endl
        << "\t-t <trim probe> -x <x-coord> -y <y-coord> -z <z-coord> " << endl
        << "\t-e <EZD outfile> -o <PDB outfile> -m <MRC outfile> " << endl;
      cerr << "Channel.exe -- Extracts a particular channel from the solvent" << endl;
      cerr << endl;

      return 1;
    }
    --argc; --argc;
    ++argv; ++argv;
  }

//INITIALIZE GRID
  finalGridDims(BIGPROBE);

//HEADER CHECK
  cerr << "Probe Radius: " << BIGPROBE << endl;
  cerr << "Grid Spacing: " << GRID << endl;
  cerr << "Resolution:      " << int(1000.0/float(GRIDVOL))/1000.0 << " voxels per A^3" << endl;
  cerr << "Resolution:      " << int(11494.0/float(GRIDVOL))/1000.0 << " voxels per water molecule" << endl;
  cerr << "Input file:   " << file << endl;

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
  trun_ExcludeGrid(TRIMPROBE,biggrid,trimgrid);
  free (biggrid);

  cout << "bg_prb\tsm_prb\tgrid\texcvol\tsurf\taccvol\tfile" << endl;

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
// SELECT PARTICULAR CHANNEL
// ***************************************************
    gridpt *channelACC;
    channelACC = (gridpt*) malloc (NUMBINS);
    if (channelACC==NULL) { cerr << "GRID IS NULL" << endl; return 1; }
    zeroGrid(channelACC);

//main channel
    get_Connected(solventACC,channelACC, x, y, z);
    free (solventACC);

// ***************************************************
// GETTING CONTACT CHANNEL
// ***************************************************
    gridpt *channelEXC;
    channelEXC = (gridpt*) malloc (NUMBINS);
    if (channelEXC==NULL) { cerr << "GRID IS NULL" << endl; return 1; }
    int channelACCvol = copyGrid(channelACC,channelEXC);
    cerr << "Accessible Channel Volume  ";
    printVol(channelACCvol);
    grow_ExcludeGrid(SMPROBE,channelACC,channelEXC);
    free (channelACC);

//limit growth to inside trimgrid
    intersect_Grids(channelEXC,trimgrid); //modifies channelEXC
    //free (trimgrid);

// ***************************************************
// OUTPUT RESULTS
// ***************************************************
    cout << BIGPROBE << "\t" << SMPROBE << "\t" << GRID << "\t" << flush;
    int channelEXCvol = countGrid(channelEXC);
    printVolCout(channelEXCvol);
    long double surf = surface_area(channelEXC);
    cout << "\t" << surf << "\t" << flush;
    printVolCout(channelACCvol);
    cout << "\t#" << file << endl;
    if(pdbfile[0] != '\0') {
      write_SurfPDB(channelEXC, pdbfile);
    }
    if(ezdfile[0] != '\0') {
      write_HalfEZD(channelEXC, ezdfile);
    }
    if(mrcfile[0] != '\0') {
      writeSmallMRCFile(channelEXC, mrcfile);
    }

//RELEASE TEMPGRID
    free (channelEXC);
    cerr << endl;

  free (trimgrid);
  cerr << endl << "Program Completed Sucessfully" << endl << endl;
  return 0;
};

