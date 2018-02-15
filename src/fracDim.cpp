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
  double PROBE=10.0;
  double GRID1=0.4;
  double GRID2=0.8;
  double NUMGRIDSTEP=10;
  
  
  while(argc > 1 && argv[1][0] == '-') {
    if(argv[1][1] == 'i') {
      sprintf(file,&argv[2][0]);
    } else if(argv[1][1] == 'p') {
      PROBE = atof(&argv[2][0]);
    } else if(argv[1][1] == 'g' && argv[1][2] == '1') {
      GRID1 = atof(&argv[2][0]);
    } else if(argv[1][1] == 'g' && argv[1][2] == '2') {
      GRID2 = atof(&argv[2][0]);            
    } else if(argv[1][1] == 'g' && argv[1][2] == 'n') {
      NUMGRIDSTEP = atof(&argv[2][0]);         
    } else if(argv[1][1] == 'h') {
      cerr << "./FracDim.exe -i <file> -p <probe_rad> " << endl
       << "  -g1 <grid 1> -g2 <grid 2> -gn <num grid step>" << endl;
      cerr << "FracDim.exe -- Calculate the fractional dimension for any probe radius" << endl;
      cerr << endl;
      return 1;
    }
    --argc; --argc;
    ++argv; ++argv;
  }

	double xsum=0, yAsum=0, xyAsum=0, yBsum=0, xyBsum=0, x2sum=0, yA2sum=0, yB2sum=0, N=0;

	//HEADER CHECK
	cerr << "Probe Radius: " << PROBE << endl;
	cerr << "Input file:   " << file << endl;

	//log(1/GRID1) - log(1/GRID2) = constant
	//log(GRID2) - log(GRID1) = constant
	//log(GRID1*A) - log(GRID1) = constant
	//log(GRID1) + log(A) - log(GRID1) = constant
	//log(A) = constant

	//GRID2 = GRID1 * GRIDSTEP ^ NUMGRIDSTEP
	//GRIDSTEP ^ NUMGRIDSTEP = GRID2/GRID1
	//GRIDSTEP = [ GRID2/GRID1 ] ^ ( 1/NUMGRIDSTEP )
	double GRIDSTEP = pow(GRID2/GRID1, 1.0/NUMGRIDSTEP);
	for (GRID=GRID1; GRID<=GRID2; GRID*=GRIDSTEP) {  
		//INITIALIZE GRID
		finalGridDims(PROBE);

		cerr << "Grid Spacing: " << GRID << endl;

		//FIRST PASS, MINMAX
		int numatoms = read_NumAtoms(file);

		//CHECK LIMITS & SIZE
		assignLimits();
//SLOPE	2.99953	2.10564 0.0
//SLOPE	2.83974	2.12005 0.5
//SLOPE	2.93329	2.08918 1.0
//SLOPE	2.93304	2.04321 1.5
//SLOPE	2.95198	2.03213 3.0
//SLOPE	2.95141	2.02238 4.5
//SLOPE	2.95626	2.02310 6.0
//SLOPE	2.96453	2.02655 7.5
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

		int edgeVoxels = countEdgePoints(EXCgrid);
//1.5	2.97079	0.999992	2.02719	0.999988 // big run
//1.5	2.75922	-0.999936	2.01569	-0.999939 // weighted

		//RELEASE TEMPGRID
		free (EXCgrid);

		//cout << GRID << "\t" << voxels << "\t" << edgeVoxels << endl;
		double x = -1.0*log(GRID);
		double yA = log(double(voxels));
		double yB = log(double(edgeVoxels));
		
		cerr << endl;
		//cout << x << "\t" << yA << "\t" << yB << "\t" << GRID << endl;
		
		double weight = 1.0/x - 1/GRID2 + 1e-6;
		xsum += weight*x;
		x2sum += weight*x*x;
		xyAsum += weight*x*yA;
		yAsum += weight*yA;
		yA2sum += weight*yA*yA;
		xyBsum += weight*x*yB;
		yBsum += weight*yB;
		yB2sum += weight*yB*yB;
		N += weight;
		cerr << endl << "Program Completed Sucessfully" << endl << endl;
	}
	double numer, denom;
	numer = ((N * xyAsum) - (xsum * yAsum));
    denom = ((N * x2sum - xsum * xsum)* (N * yA2sum - yAsum * yAsum));	
	double corrA = numer / sqrt(denom);
	numer = ((N * xyBsum) - (xsum * yBsum));
    denom = ((N * x2sum - xsum * xsum)* (N * yB2sum - yBsum * yBsum));	
	double corrB = numer / sqrt(denom);
	double slopeA = (xyAsum - xsum*yAsum/N)/(x2sum - xsum*xsum/N);
	double slopeB = (xyBsum - xsum*yBsum/N)/(x2sum - xsum*xsum/N);
	corrA = fabs(corrA);
	corrB = fabs(corrB);	
	//cout << PROBE << "\t" << slopeA << "\t" << corrA << "\t" << slopeB << "\t" << corrB << endl;
	cout << PROBE << "\t" << slopeA << "\t" << slopeB << endl;

	return 0;
};

