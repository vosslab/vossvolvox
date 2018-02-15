#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <cstring>
#include <cctype>

# define CITATION \
  fprintf(stderr,\
 "Citation: %s.\nDOI: %s\nE-mail: M Gerstein <%s> or NR Voss <%s>.\n\n",\
 "Neil R Voss, et al. J Mol Biol. v360 (4): 2006, pp. 893-906",\
 "http://dx.doi.org/10.1016/j.jmb.2006.05.023",\
 "mark.gerstein@yale.edu","vossman77@yahoo.com")

# define COMPILE_INFO \
  fprintf(stderr,\
 "Program: %s at line %d in source %s,\n which was compiled on %s at %s.\n\n",\
 argv[0],__LINE__,__FILE__,__DATE__,__TIME__)

//#define MAXPROBE   10.0
#define MAXVDW  2.0
//#define MAXBINS 1073741823 //2^30
#define MAXBINS 2147483647 //2^31
//#define MAXBINS 4294967295 //2^32
//#define MAXBINS 8589934591 //2^33
//#define MAXLIST 1048576 //2^20
#define MAXLIST 262144 //2^18
//#define MAXLIST 32768 //2^15
//#define MAXLIST 8192  //2^13
#define DEBUG 0

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

//using namespace std;
using std::cerr;
using std::endl;
using std::flush;
using std::ifstream;
using std::ofstream;
using std::cout;

//typedef unsigned short int gridpt;
#ifndef _GRID_PT
  typedef bool gridpt;
  #define _GRID_PT
#endif

#ifndef _GRID_STRUCT
  #define _GRID_STRUCT
  struct ind { int i; int j; int k; float b;};
  struct real { float x; float y; float z; };
  struct sphere { float x; float y; float z; float r; };
  struct vector { float x; float y; float z; };
#endif

//init functions
void finalGridDims (float maxprobe);
float getIdealGrid ();
void assignLimits ();
void testLimits (gridpt grid[]);

//grid util functions
int countGrid (gridpt grid[]);
void zeroGrid (gridpt grid[]);
int copyGridFromTo (gridpt oldgrid[], gridpt newgrid[]);
int copyGrid (gridpt oldgrid[], gridpt newgrid[]);
void inverseGrid (gridpt grid[]);

//file based functions
int read_NumAtoms (char file[]);
int fill_AccessGrid_fromFile (int numatoms, const float probe, char file[], gridpt grid[]);
int get_ExcludeGrid_fromFile (int numatoms, const float probe, char file[], gridpt EXCgrid[]);

//generate grids / grid changers
//void expand (gridpt oldgrid[], gridpt newgrid[]);
//void contract (gridpt oldgrid[], gridpt newgrid[]);
void trun_ExcludeGrid (const float probe, gridpt ACCgrid[], gridpt EXCgrid[]); //contract
void grow_ExcludeGrid (const float probe, gridpt ACCgrid[], gridpt EXCgrid[]); //expands
float *get_Point (gridpt grid[]);
int get_GridPoint (gridpt grid[]);
int get_Connected (gridpt grid[], gridpt connect[], const float x, const float y, const float z);
int get_ConnectedRange (gridpt grid[], gridpt connect[], const float x, const float y, const float z);
int get_Connected_Point (gridpt grid[], gridpt connect[], const int gp);
int subt_Grids (gridpt biggrid[], gridpt smgrid[]); //Modifies biggrid; returns difference
int intersect_Grids (gridpt grid1[], gridpt grid2[]); //Modifies grid1; returns final vox num
int merge_Grids (gridpt grid1[], gridpt grid2[]); //Modifies grid1; returns final vox num

//point based function
int fill_AccessGrid (const float x, const float y, const float z, const float r, gridpt grid[]);
void empty_ExcludeGrid (const int i, const int j, const int k, const float probe, gridpt grid[]);
void fill_ExcludeGrid (const int i, const int j, const int k, const float probe, gridpt grid[]);
bool isEdgePoint (const int i, const int j, const int k, gridpt grid[]);
int ijk2pt(int i, int j, int k);
void pt2ijk(int pt, int &i, int &j, int &k);
void pt2xyz(int pt, float &x, float &y, float &z);
int xyz2pt(float x, float y, float z);
bool isEdgePoint_Fill (const int pt, gridpt grid[]);
bool isEdgePoint_Star (const int pt, gridpt grid[]);
//void expand_Point (const int pt, gridpt grid[]);
//void contract_Point (const int pt, gridpt grid[]);
void ijk2pdb (char line[], int i, int j, int k, int n);
bool isContainedPoint (const int pt, gridpt ingrid[], gridpt outgrid[], int minmax[]);
bool isNearEdgePoint (const int pt, gridpt ingrid[], gridpt outgrid[]);


//special
bool isCloseToVector (const float radius, const int pt);
void limitToTunnelArea(const float radius, gridpt grid[]);
float distFromPt (const float x, const float y, const float z);
float crossSection (const real p, const vector v, const gridpt grid[]);
float crossSection (const gridpt grid[]);
void real2index (const real p, ind i);

//string functions
void padLeft(char a[], int n);
void padRight(char a[], int n);
void printBar ();
void printVol (int vox);
void printVolCout (int vox);
void basename(char str[], char base[]);

//surface area
int countEdgePoints (gridpt grid[]);
float surface_area (gridpt grid[]);
int classifyEdgePoint (const int pt, gridpt grid[]);

//other ideas
//int convex_hull(gridpt grid[], gridpt hull[]);
//int convex_hull(int numatoms, char file[], gridpt hull[]);
void determine_MinMax(gridpt grid[], int minmax[]);
int bounding_box(gridpt grid[], gridpt bbox[]);
//int bounding_box(int numatoms, char file[], gridpt bbox[]);
int fill_cavities(gridpt grid[]);
int makerbot_fill(gridpt ingrid[], gridpt outgrid[]);


/*************************************************
//output functions (in utils-output.cpp)
**************************************************/
void write_PDB (gridpt grid[], char outfile[]);
void write_SurfPDB (gridpt grid[], char outfile[]);
void write_EZD (gridpt grid[], char outfile[]);
void write_BlurEZD (gridpt grid[], char outfile[]);
void write_HalfEZD (gridpt grid[], char outfile[]);
void write_ThirdEZD (gridpt grid[], char outfile[]);
void write_FifthEZD (gridpt grid[], char outfile[]);

/*************************************************
//output functions (in utils-mrc.cpp)
**************************************************/
int writeMRCFile( gridpt data[], char filename[] );
int writeSmallMRCFile( gridpt data[], char filename[] );

