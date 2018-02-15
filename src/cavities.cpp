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
  double shell_rad = 10.0;
  double probe_rad = 3.0;
  double trim_rad = 3.0;

  while(argc > 1 && argv[1][0] == '-') {
    if(argv[1][1] == 'i') {
      sprintf(file,&argv[2][0]);
    } else if(argv[1][1] == 'g') {
      GRID = atof(&argv[2][0]);
    } else if(argv[1][1] == 'b') {
      shell_rad = atof(&argv[2][0]);
    } else if(argv[1][1] == 's') {
      probe_rad = atof(&argv[2][0]);
    } else if(argv[1][1] == 't') {
      trim_rad = atof(&argv[2][0]);
    } else if(argv[1][1] == 'e') {
      sprintf(ezdfile,&argv[2][0]);
    } else if(argv[1][1] == 'm') {
      sprintf(mrcfile,&argv[2][0]);
    } else if(argv[1][1] == 'o') {
      sprintf(pdbfile,&argv[2][0]);
    } else if(argv[1][1] == 'h') {
      cerr << "./Cavities.exe -i <file> -g <grid spacing> -b <big/shell radius> " << endl
        << "\t-s <small/probe radius> -t <trim_probe_rad>  " << endl
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
  finalGridDims(shell_rad*2);
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
  fill_AccessGrid_fromFile(numatoms,shell_rad,file,shellACC);
  fill_cavities(shellACC);

  gridpt *shellEXC=NULL;
  shellEXC = (gridpt*) malloc (NUMBINS);
  trun_ExcludeGrid(shell_rad,shellACC,shellEXC);

// ****************************************************
// STARTING MAIN PROGRAM
// ****************************************************

  getCavitiesBothMeth(probe_rad,shellACC,shellEXC,numatoms,file,ezdfile,pdbfile, mrcfile);

// ****************************************************
// CLEAN UP AND QUIT
// ****************************************************

  free (shellACC);
  free (shellEXC);

  cerr << endl << "Program Completed Sucessfully" << endl << endl;
  return 0;
};

int getCavitiesBothMeth(const float probe, gridpt shellACC[], gridpt shellEXC[],
	const int natoms, char file[], char ezdfile[], char pdbfile[], char mrcfile[])
{
/* THIS USES THE ACCESSIBLE SHELL AS THE BIG SURFACE */
/*******************************************************
Accessible Process
*******************************************************/

//Create access map
  gridpt *access=NULL;
  access = (gridpt*) malloc (NUMBINS);
  fill_AccessGrid_fromFile(natoms,probe,file,access);

//Create inverse access map
  gridpt *cavACC=NULL;
  cavACC = (gridpt*) malloc (NUMBINS);
  copyGrid(shellACC,cavACC);
  subt_Grids(cavACC,access); //modifies cavACC
  free (access);
  int achanACC_voxels = countGrid(cavACC);

// EXTRA STEPS TO REMOVE SURFACE CAVITIES???

//Get first point
  bool stop = 1; int firstpt = 0;
  for(unsigned int pt=1; pt<NUMBINS && stop; pt++) {
    if(cavACC[pt]) { stop = 0; firstpt = pt;}
  }
  cerr << "FIRST POINT: " << firstpt << endl;
//LAST POINT
  stop = 1; int lastpt = NUMBINS-1;
  for(unsigned int pt=NUMBINS-1; pt>0 && stop; pt--) {
    if(cavACC[pt]) { stop = 0; lastpt = pt;}
  }
  cerr << "LAST  POINT: " << lastpt << endl;
//  get_Connected_Point(cavACC,chanACC,lastpt);


//Pull channels out of inverse access map
  gridpt *chanACC=NULL;
  chanACC = (gridpt*) malloc (NUMBINS);
  zeroGrid(chanACC);
  cerr << "Getting Connected Next" << endl;
  get_Connected_Point(cavACC,chanACC,firstpt); //modifies chanACC
  get_Connected_Point(cavACC,chanACC,lastpt); //modifies chanACC
  int chanACC_voxels = countGrid(chanACC);
//Subtract channels from access map leaving cavities
  subt_Grids(cavACC,chanACC); //modifies cavACC
  free (chanACC);
  int cavACC_voxels = countGrid(cavACC);

//Grow Access Cavs
  gridpt *ecavACC=NULL;
  ecavACC = (gridpt*) malloc (NUMBINS);
  grow_ExcludeGrid(probe,cavACC,ecavACC);
  free (cavACC);
  
//Intersect Grown Access Cavities with Shell
  int scavACC_voxels = countGrid(ecavACC);
  int ecavACC_voxels = intersect_Grids(ecavACC,shellEXC); //modifies ecavACC

  //float surfEXC = surface_area(ecavACC);
  free (ecavACC);

/*******************************************************
Excluded Process
*******************************************************/

//Create access map
  gridpt *access2=NULL;
  access2 = (gridpt*) malloc (NUMBINS);
  fill_AccessGrid_fromFile(natoms,probe,file,access2);

//Create exclude map
  gridpt *exclude=NULL;
  exclude = (gridpt*) malloc (NUMBINS);
  trun_ExcludeGrid(probe,access2,exclude);
  free (access2);

//Create inverse exclude map
  gridpt *cavEXC=NULL;
  cavEXC = (gridpt*) malloc (NUMBINS);
  copyGrid(shellEXC,cavEXC);
  subt_Grids(cavEXC,exclude); //modifies cavEXC
  free (exclude);
  int echanEXC_voxels = countGrid(cavEXC);

//Get first point
  stop = 1; firstpt = 0;
  for(unsigned int pt=1; pt<NUMBINS && stop; pt++) {
    if(cavEXC[pt]) { stop = 0; firstpt = pt;}
  }
  cerr << "FIRST POINT: " << firstpt << endl;
//LAST POINT
  stop = 1; lastpt = NUMBINS-1;
  for(unsigned int pt=NUMBINS-1; pt>0 && stop; pt--) {
    if(cavEXC[pt]) { stop = 0; lastpt = pt;}
  }
  cerr << "LAST  POINT: " << lastpt << endl;

//Pull channels out of inverse excluded map
  gridpt *chanEXC=NULL;
  chanEXC = (gridpt*) malloc (NUMBINS);
  zeroGrid(chanEXC);
  cerr << "Getting Connected Next" << endl;
  get_Connected_Point(cavEXC,chanEXC,firstpt); //modifies chanEXC
  get_Connected_Point(cavEXC,chanEXC,lastpt); //modifies chanEXC
  int chanEXC_voxels = countGrid(chanEXC);
//Subtract channels from exclude map leaving cavities
  subt_Grids(cavEXC,chanEXC); //modifies cavEXC
  free (chanEXC);
  int cavEXC_voxels = countGrid(cavEXC);

//Write out exclude cavities
  if(pdbfile[0] != '\0') {
    write_SurfPDB(cavEXC,pdbfile);
  }
  if(ezdfile[0] != '\0') {
    write_HalfEZD(cavEXC,ezdfile);
  }
  if(mrcfile[0] != '\0') {
    writeMRCFile(cavEXC, mrcfile);
  }
  //float surfEXC = surface_area(cavEXC);
  free (cavEXC);

  cerr << endl;
  cerr << "achanACC_voxels = " << achanACC_voxels << endl
       << "chanACC_voxels  = " << chanACC_voxels << endl
       << "cavACC_voxels   = " << cavACC_voxels << endl
       << "scavACC_voxels  = " << scavACC_voxels << endl
       << "-------------------------------------" << endl
       << "ecavACC_voxels  = " << ecavACC_voxels << endl << endl;
  cerr << "echanEXC_voxels = " << echanEXC_voxels << endl
       << "chanEXC_voxels  = " << chanEXC_voxels << endl
       << "-------------------------------------" << endl
       << "cavEXC_voxels   = " << cavEXC_voxels << endl << endl << endl;
  cout << probe << "\t" << GRID << "\t" ;
  printVolCout(ecavACC_voxels);
  cout << "\t";
  printVolCout(cavEXC_voxels);
  //cout << "\t\t";
  //printVolCout(scavACC_voxels);
  //cout << "\t";
  //printVolCout(cavACC_voxels);
  cout << "\t" << natoms << "\t" << file;
  cout << "\tprobe,grid,cav_meth1,cav_meth2,num_atoms,file";
  cout << endl;
  
//  float perACC = 100*float(tunnACC_voxels) / float(chanACC_voxels);
//  float perEXC = 100*float(tunnEXC_voxels) / float(chanEXC_voxels);
  
  return cavACC_voxels+ecavACC_voxels;
};
