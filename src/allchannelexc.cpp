#include <iostream>
#include <string>
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
	char dirname[256]; dirname[0] = '\0';
	char mrcfile[256]; mrcfile[0] = '\0';
	double BIGPROBE=9.0;
	double SMPROBE=1.5;
	double TRIMPROBE=4.0;
	int MINSIZE=20;
	double minvol=0;
	double minperc=0;

	while(argc > 1 && argv[1][0] == '-') {
		if(argv[1][1] == 'i') {
			sprintf(file,&argv[2][0]);
		} else if(argv[1][1] == 'b') {
			BIGPROBE = atof(&argv[2][0]);
		} else if(argv[1][1] == 's') {
			SMPROBE = atof(&argv[2][0]);
		} else if(argv[1][1] == 't') {
			TRIMPROBE = atof(&argv[2][0]);
		} else if(argv[1][1] == 'm') {
			sprintf(mrcfile,&argv[2][0]);
		} else if(argv[1][1] == 'v') {
			minvol = atof(&argv[2][0]);
		} else if(argv[1][1] == 'p') {
			minperc = atof(&argv[2][0]);
		} else if(argv[1][1] == 'i') {
			sprintf(file,&argv[2][0]);
		} else if(argv[1][1] == 'g') {
			GRID = atof(&argv[2][0]);
		} else if(argv[1][1] == 'h') {
			cerr << "./AllChannelExc.exe -i <file> -b <big_probe> -s <small_probe> -g <gridspace> " << endl
			   << "\t-t <trim probe> -v <min-volume in A^3> -p <min-percent in %>" << endl;
			cerr << "AllChannelExc.exe -- Extracts all channels from the solvent above a cutoff" << endl;
			cerr << endl;
			return 1;
		}
		--argc; --argc;
		++argv; ++argv;
	}

	if (minvol > 0)
		MINSIZE = int(minvol/GRIDVOL);
	else if (minperc == 0)
		minperc=0.01;

	//INITIALIZE GRID
	finalGridDims(BIGPROBE);

	//HEADER CHECK
	cerr << "Probe Radius: " << BIGPROBE << endl;
	cerr << "Grid Spacing: " << GRID << endl;
	cerr << "Resolution:      " << int(1000.0/float(GRIDVOL))/1000.0 << " voxels per A^3" << endl;
	cerr << "Resolution:      " << int(11494.0/float(GRIDVOL))/1000.0 << " voxels per water molecule" << endl;
	cerr << "Input file:   " << file << endl;
	cerr << "Minimum size: " << MINSIZE << " voxels" << endl;

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

	if (minperc > 0) {
		while (minperc > 1)
			minperc /= 100;
		MINSIZE = int(bigvox*minperc);
	}
	cerr << "Minimum size: " << MINSIZE << " voxels" << endl;

	// ****************************************************
	// TRIM LARGE PROBE SURFACE
	// ****************************************************
	gridpt *trimgrid;
	trimgrid = (gridpt*) malloc (NUMBINS);
	if (trimgrid==NULL) { cerr << "GRID IS NULL" << endl; return 1; }
	copyGrid(biggrid,trimgrid);
	trun_ExcludeGrid(TRIMPROBE,biggrid,trimgrid);
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
	copyGrid(trimgrid, solventACC); //copy trimgrid into solventACC
	subt_Grids(solventACC, smgrid); //modify solventACC
	free (smgrid);


// ***************************************************
// CALCULATE TOTAL SOLVENT
// ***************************************************	
	gridpt *solventEXC;
	solventEXC = (gridpt*) malloc (NUMBINS);
	if (solventEXC==NULL) { cerr << "GRID IS NULL" << endl; return 1; }
	grow_ExcludeGrid(SMPROBE, solventACC, solventEXC);
	intersect_Grids(solventEXC, trimgrid); //modifies solventEXC
	sprintf(mrcfile, "allsolvent.mrc");
	writeMRCFile(solventEXC, mrcfile);
	free (solventACC);

// ***************************************************
// SELECT PARTICULAR CHANNEL
// ***************************************************

	// initialize channel volume
	gridpt *channelEXC;
	channelEXC = (gridpt*) malloc (NUMBINS);
	if (channelEXC==NULL) { cerr << "GRID IS NULL" << endl; return 1; }
	int solventEXCvol = countGrid(solventEXC);
	//main channel loop
	int numchannels=0;
	int allchannels=0;
	int connected;
	int maxvox=0, minvox=1000000, goodminvox=1000000;
	cerr << "MIN SIZE: " << MINSIZE << " voxels" << endl;
	cerr << "MIN SIZE: " << MINSIZE*GRIDVOL << " Angstroms" << endl;

	while ( countGrid(solventEXC) > MINSIZE ) {
		allchannels++;

		// get channel volume
		zeroGrid(channelEXC); // initialize channelEXC volume
		int gp = get_GridPoint(solventEXC);  //modify x,y,z
		connected = get_Connected_Point(solventEXC, channelEXC, gp); //modify channelEXC
		subt_Grids(solventEXC, channelEXC); //modify solventEXC

		// ***************************************************
		// GETTING CONTACT CHANNEL
		// ***************************************************
		int channelEXCvol = countGrid(channelEXC); // initialize channelEXC volume
		if (channelEXCvol > maxvox)
			maxvox = channelEXCvol;
		if (channelEXCvol < minvox and channelEXCvol > 0)
			minvox = channelEXCvol;
		if (channelEXCvol <= MINSIZE) {
			cerr << "SKIPPING CHANNEL" << endl;
			cerr << "---------------------------------------------" << endl;
			continue;
		}
		if (channelEXCvol < goodminvox)
			goodminvox = channelEXCvol;

		numchannels++;

		// ***************************************************
		// OUTPUT RESULTS
		// ***************************************************
		cout << BIGPROBE << "\t" << SMPROBE << "\t" << GRID << "\t" << flush;
		printVolCout(channelEXCvol);
		long double surf = surface_area(channelEXC);
		cout << "\t" << surf << "\t" << flush;
		cout << "\t#" << file << endl;
		sprintf(mrcfile, "channel-%03d.mrc", numchannels);
		writeSmallMRCFile(channelEXC, mrcfile);
		cerr << "---------------------------------------------" << endl;
	}
	if (numchannels == 0)
		goodminvox = 0;
	cerr << "Channel min size: " << int(minvox*GRIDVOL) << " A (all) " << int(goodminvox*GRIDVOL) << " A (good)" << endl;
	cerr << "Channel max size: " << int(maxvox*GRIDVOL) << " A " << endl;
	cerr << "Used " << numchannels << " of " << allchannels << " channels" << endl;
	if (allchannels > 0) {
		cerr << "Mean size: " << solventEXCvol/float(allchannels)*GRIDVOL << " A " << endl;
	}
	cerr << "Cutoff size: " << MINSIZE << " voxels :: "<< MINSIZE*GRIDVOL << " Angstroms" << endl;
	free (channelEXC);
	free (solventEXC);
	free (trimgrid);
	cerr << endl << "Program Completed Sucessfully" << endl << endl;
	return 0;
};

