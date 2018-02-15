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

void printTun(const float probe, 
	const float surfEXC, const int tunnEXC_voxels, const int chanEXC_voxels,
	const float surfACC, const int tunnACC_voxels, const int chanACC_voxels,
	char file[]);
void defineTunnel(gridpt tunnel[], gridpt channels[]);

int main(int argc, char *argv[]) {
  cerr << endl;

  COMPILE_INFO;
  CITATION;

// ****************************************************
// INITIALIZATION :: REQUIRED
// ****************************************************

//HEADER INFO
  char file[256]; file[0] = '\0';
  char ezdfile[256]; ezdfile[0] = '\0';
  char pdbfile[256]; pdbfile[0] = '\0';
  char mrcfile[256]; mrcfile[0] = '\0';
  double shell_rad = 10.0;
  double tunnel_prb = 3.000;
  double trim_prb = 3.000;

  while(argc > 1 && argv[1][0] == '-') {
    if(argv[1][1] == 'i') {
      sprintf(file,&argv[2][0]);
    } else if(argv[1][1] == 'g') {
      GRID = atof(&argv[2][0]);
    } else if(argv[1][1] == 'b') {
      shell_rad = atof(&argv[2][0]);
    } else if(argv[1][1] == 's') {
      tunnel_prb = atof(&argv[2][0]);
    } else if(argv[1][1] == 't') {
      trim_prb = atof(&argv[2][0]);
    } else if(argv[1][1] == 'e') {
      sprintf(ezdfile,&argv[2][0]);
    } else if(argv[1][1] == 'm') {
      sprintf(mrcfile,&argv[2][0]);
    } else if(argv[1][1] == 'o') {
      sprintf(pdbfile,&argv[2][0]);
    } else if(argv[1][1] == 'h') {
      cerr << "./Tunnel.exe -i <file> -g <grid spacing> -s <small tunnel probe radius>" << endl
        << "\t-e <EZD outfile> -o <PDB outfile> -m <MRC outfile>" << endl
        << "\t-b <big shell radius> -t <trim radius>" << endl;
      cerr << "Tunnel.exe -- Extracts the ribosomal exit tunnel from the H. marismortui structure" << endl;
      cerr << endl;
      return 1;
    }
    --argc; --argc;
    ++argv; ++argv;
  }

//INITIALIZE GRID
  finalGridDims(shell_rad);
//FIRST PASS, MINMAX
  int numatoms = read_NumAtoms(file);
//CHECK LIMITS & SIZE
  assignLimits();

//HEADER CHECK
  cerr << "Grid Spacing: " << GRID << endl;
  cerr << "Resolution:      " << int(1000.0/float(GRIDVOL))/1000.0 << " voxels per A^3" << endl;
  cerr << "Resolution:      " << int(11494.0/float(GRIDVOL))/1000.0 << " voxels per water molecule" << endl;
  cerr << "Input file:   " << file << endl;

// ****************************************************
// BUSINESS PART
// ****************************************************

//Compute Shell
  gridpt *shellACC=NULL;
  shellACC = (gridpt*) malloc (NUMBINS);
  fill_AccessGrid_fromFile(numatoms,shell_rad,file,shellACC);
  fill_cavities(shellACC);

  gridpt *shellEXC=NULL;
  shellEXC = (gridpt*) malloc (NUMBINS);
  trun_ExcludeGrid(shell_rad,shellACC,shellEXC);
  free (shellACC);

//Trim Shell
  if(trim_prb > 0.0) {
    gridpt *trimEXC;
    trimEXC = (gridpt*) malloc (NUMBINS);
    copyGrid(shellEXC,trimEXC);
    trun_ExcludeGrid(trim_prb,shellEXC,trimEXC);  // TRIMMING PART
    zeroGrid(shellEXC);
    copyGrid(trimEXC,shellEXC);
    free (trimEXC);
  }

//Get Shell Volume
  int shell_vol = countGrid(shellEXC);
  printVol(shell_vol); cerr << endl;

//Get Access Volume for "probe"
  gridpt *access;
  access = (gridpt*) malloc (NUMBINS);
  fill_AccessGrid_fromFile(numatoms,tunnel_prb,file,access);

//Get Channels for "probe"
  gridpt *chanACC;
  chanACC = (gridpt*) malloc (NUMBINS);
  copyGrid(shellEXC,chanACC);
  subt_Grids(chanACC,access);
  free (access);
  intersect_Grids(chanACC,shellEXC); //modifies chanACC
  int chanACC_voxels = countGrid(chanACC);
  printVol(chanACC_voxels); cerr << endl;

//Extract Tunnel
  gridpt *tunnACC;
  tunnACC = (gridpt*) malloc (NUMBINS);
  defineTunnel(tunnACC, chanACC);
  //writeMRCFile(chanACC, mrcfile);
  free (chanACC);
  int tunnACC_voxels = countGrid(tunnACC);
  cerr << "ACCESSIBLE TUNNEL VOLUME: ";
  printVol(tunnACC_voxels); cerr << endl << endl;
  //float surfACC = surface_area(tunnACC);
  float surfACC = 0;
  if (tunnACC_voxels*GRIDVOL > 2000000) {
    cerr << "ERROR: Accessible volume of tunnel is too large to be valid" << endl;
    free (shellEXC);
    //writeMRCFile(chanACC, mrcfile);
    free (tunnACC);
    return 0;
  }

//Grow Tunnel
  gridpt *tunnEXC;
  tunnEXC = (gridpt*) malloc (NUMBINS);
  grow_ExcludeGrid(tunnel_prb,tunnACC,tunnEXC);
  free (tunnACC);

//Intersect Grown Tunnel with Shell
  intersect_Grids(tunnEXC,shellEXC); //modifies tunnEXC
  free (shellEXC);

//Get EXC Props
  int tunnEXC_voxels = countGrid(tunnEXC);
  cerr << "TUNNEL VOLUME: ";
  printVol(tunnEXC_voxels); cerr << endl << endl;
  //shell volume at 6A probe 9732148*0.6^3 = 2,102,144 A^3
  //solvent volume at 1.4A probe 2465252*0.6^3 = 532,494 A^3
  if (tunnEXC_voxels*GRIDVOL > 1800000) {
    cerr << "ERROR: Excluded volume of tunnel is too large to be valid" << endl;
    free (tunnEXC);
    return 0;
  }
  float surfEXC = surface_area(tunnEXC);

//Output
  if(pdbfile[0] != '\0') {
    write_SurfPDB(tunnEXC, pdbfile);
  }
  if(ezdfile[0] != '\0') {
    write_HalfEZD(tunnEXC, ezdfile);
  }
  if(mrcfile[0] != '\0') {
    //writeMRCFile(tunnEXC, mrcfile);
    writeSmallMRCFile(tunnEXC, mrcfile);
  }

  free (tunnEXC);

  printTun(trim_prb,surfEXC,tunnEXC_voxels,0,
	surfACC,tunnACC_voxels,chanACC_voxels,file);

  cerr << endl << "Program Completed Sucessfully" << endl << endl;
  return 0;
};

//***********************************************************//
//***********************************************************//
//***********************************************************//

void defineTunnel(gridpt tunnel[], gridpt channels[])
{
//NEW IDEAL TUNNEL POINTS
  zeroGrid(tunnel);
//  get_Connected(chanACC,tunnACC,77.2,116.0,109.2); //tRNA cleft
  get_Connected(channels, tunnel, 74.8,130.0,83.6); //highest tunnel pt
  get_Connected(channels, tunnel, 68.3,132.2,85.6); //largest area
  get_Connected(channels, tunnel, 53.6,144.8,69.6); //below main
  get_Connected(channels, tunnel, 49.9,151.8,67.3); //2nd largest & low 
  get_Connected(channels, tunnel, 38.4,160.4,63.6); //low blob point
  get_Connected(channels, tunnel, 35.6,163.6,61.6); //lowest pt
//OLD POINTS ; CAN'T HURT
  get_Connected(channels, tunnel, 53.6,141.3,66.4);
  get_Connected(channels, tunnel, 71.5,120.4,97.3);
  get_Connected(channels, tunnel, 71.5,125.0,98.1);
  get_Connected(channels, tunnel, 70.3,131.2,81.9);
  get_Connected(channels, tunnel, 55.7,140.2,73.8);
  get_Connected(channels, tunnel, 44.6,153.2,68.7);

  //get_Connected(channels,tunnel,0.0,0.0,0.0);
  return;
};

//***********************************************************//
//***********************************************************//
//***********************************************************//

void printTun(const float probe, 
	const float surfEXC, const int tunnEXC_voxels, const int chanEXC_voxels,
	const float surfACC, const int tunnACC_voxels, const int chanACC_voxels,
        char file[])
{
    float perACC = 100*float(tunnACC_voxels) / (float(chanACC_voxels) + 0.01);
    float perEXC = 100*float(tunnEXC_voxels) / (float(chanEXC_voxels) + 0.01);

    cout << probe << "\t";
//EXCLUDE
    printVolCout(tunnEXC_voxels);
    printVolCout(chanEXC_voxels);
    cout << perEXC << "\t";
    cout << surfEXC << "\t";
//ACCESS
    printVolCout(tunnACC_voxels);
    printVolCout(chanACC_voxels);
    cout << perACC << "\t";
    cout << surfACC << "\t";
    cout << GRID << endl;

    return;
};
