#include <iostream>
#include <string.h>
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

void getDirname(char path[], char dir[]) {
   if (DEBUG > 0)
	   cerr << "DIR: " << dir << " :: PATH: " << path << endl;
	if (strlen(path) < 3 || strrchr(path,'/') == NULL) {
		sprintf(dir, "./");
		return;
	}
	char* temp;
	temp = strrchr(path,'/');
	int end = strlen(path)-strlen(temp)+1;
   if (DEBUG > 0)
		printf ("Last occurence of '/' found at %d \n",end);

	strcpy(dir, path);
	dir[end] = '\0';
   if (DEBUG > 0)
		cerr << "DIR: " << dir << " :: PATH: " << path << endl;
	return;
};


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
	int numchan=0;

	while(argc > 1 && argv[1][0] == '-') {
		if(argv[1][1] == 'i') {
			sprintf(file,&argv[2][0]);
		} else if(argv[1][1] == 'b') {
			BIGPROBE = atof(&argv[2][0]);
		} else if(argv[1][1] == 's') {
			SMPROBE = atof(&argv[2][0]);
		} else if(argv[1][1] == 't') {
			TRIMPROBE = atof(&argv[2][0]);
		} else if(argv[1][1] == 'p') {
			minperc = atof(&argv[2][0]);
		} else if(argv[1][1] == 'm') {
			sprintf(mrcfile,&argv[2][0]);
		} else if(argv[1][1] == 'n') {
			numchan = int(atof(&argv[2][0]));
		} else if(argv[1][1] == 'v') {
			minvol = atof(&argv[2][0]);
		} else if(argv[1][1] == 'i') {
			sprintf(file,&argv[2][0]);
		} else if(argv[1][1] == 'g') {
			GRID = atof(&argv[2][0]);
		} else if(argv[1][1] == 'h') {
			cerr << "./AllChannel.exe -i <file> -b <big_probe> -s <small_probe> -g <gridspace> " << endl
			   << "\t-t <trim probe> -v <min-volume in A^3> -p <min-percent> -n <number of channels>" << endl;
			cerr << "AllChannel.exe -- Extracts all channels from the solvent above a cutoff" << endl;
			cerr << endl;
			return 1;
		}
		--argc; --argc;
		++argv; ++argv;
	}

	if (numchan > 0) {
		// set min volume really low and find biggest channels
		MINSIZE = 20;
	} else if (minvol > 0) {
		// set min volume to input, convert to voxels
		MINSIZE = int(minvol/GRID/GRID/GRID);
	} else if (minperc == 0) {
		// set min volume later
		minperc=0.01;
	}

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
	cerr << "MINSIZE:" << MINSIZE << endl;
	if (MINSIZE < 20) {
		cerr << "MINSIZE IS TOO SMALL, SETTING TO 20 VOXELS" << endl;
		MINSIZE = 20;
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
	//sprintf(mrcfile, "allsolvent.mrc");
	//writeMRCFile(solventEXC, mrcfile);
	free (solventEXC);

// ***************************************************
// SELECT PARTICULAR CHANNEL
// ***************************************************

	// initialize channel volume
	gridpt *channelACC;
	channelACC = (gridpt*) malloc (NUMBINS);
	if (channelACC==NULL) { cerr << "GRID IS NULL" << endl; return 1; }

	//main channel loop
	int numchannels=0;
	int allchannels=0;
	int connected;
	int maxvox=0, minvox=1000000, goodminvox=1000000;
	int solventACCvol = countGrid(solventACC); // initialize origSolventACC volume

	if (numchan > 0) {
      if (DEBUG > 0)
         cerr << "#######" << endl << "Starting NumChan Area" << endl 
              << "#######" << endl;
		gridpt *tempSolventACC;
		tempSolventACC = (gridpt*) malloc (NUMBINS);
		if (tempSolventACC==NULL) { cerr << "GRID IS NULL" << endl; return 1; }

		// set up a list
		int *vollist;
		vollist = (int*) malloc ((numchan+2)*sizeof(int));
		if (vollist==NULL) { cerr << "LIST IS NULL" << endl; return 1; }
		for (int i=0; i<numchan+2; i++) {
			vollist[i] = 0;
		}

		copyGrid(solventACC, tempSolventACC); // initialize tempSolventACC volume

		short int insert;
		while ( countGrid(tempSolventACC) > MINSIZE ) {
			// get channel volume
			zeroGrid(channelACC); // initialize channelACC volume

			// get first available filled point
			int gp = get_GridPoint(tempSolventACC);  //modify gp

         if (DEBUG > 0) {
				int tempSolventACCvol = countGrid(tempSolventACC);
            cerr << "Temp Solvent Volume ..." << tempSolventACCvol << endl;
			}

			// get all connected volume to point
         if (DEBUG > 0)
            cerr << "Time to crash..." << endl;
			connected = get_Connected_Point (tempSolventACC, channelACC, gp); //modify channelACC
         if (DEBUG > 0)
            cerr << "Connected voxel volume: " << connected << endl;

			// remove channel from total solvent
			subt_Grids(tempSolventACC, channelACC); //modify solventACC

			// get volume
			int channelACCvol = countGrid(channelACC);

			// skip volume if it is too small
			if (channelACCvol <= MINSIZE) {
				continue;
			}

			cerr << "Channel volume: " << int(channelACCvol*GRIDVOL) << " Angstroms^3" << endl;

			insert = 0;
			for (int i=0; i<numchan+2 && !insert; i++) {
				if (channelACCvol > 0 && channelACCvol > vollist[i]) {
					// shift elements over
					for (int j=numchan+2; j>i; j--) {
						vollist[j] = vollist[j-1];
					}
					// insert new guy
					vollist[i] = channelACCvol;
					insert = 1;
				}
			}
		}
		for (int i=0; i<numchan+2; i++) {
			cerr << "Vollist[] " << i << "\t" << vollist[i] << endl;
		}
		// set the minsize to be one less than a channel of 
		MINSIZE = vollist[numchan-1] - 1;

		free (tempSolventACC);
		free (vollist);
		if (MINSIZE < 10) {
         cerr << endl << "#######" << endl << "NO CHANNELS WERE FOUND" << endl 
              << "#######" << endl;
			return 1;
		}
		cerr << "Setting minimum volume size in voxels (MINSIZE) to: " 
           << MINSIZE << endl;
      if (DEBUG > 0)
         cerr << "#######" << endl << "Ending NumChan Area" << endl 
              << "#######" << endl;
	}

	gridpt *channelEXC;
	channelEXC = (gridpt*) malloc (NUMBINS);
	if (channelEXC==NULL) { cerr << "GRID IS NULL" << endl; return 1; }

	while ( countGrid(solventACC) > MINSIZE ) {
      if (DEBUG > 0)
         cerr << endl << "Loop: Solvent Volume (" << countGrid(solventACC) 
              << ") greater than MINSIZE (" << MINSIZE << ")" << endl << endl;
		allchannels++;

		// get channel volume
		zeroGrid(channelACC); // initialize channelACC volume

		// get first available filled point
		int gp = get_GridPoint(solventACC);  //return gp

		if (DEBUG > 0) {
			cerr << "Solvent Volume ... " << countGrid(solventACC) << endl;
			cerr << "Channel Volume ... " << countGrid(channelACC) << endl;
		}

      if (DEBUG > 0)
         cerr << "Time to crash..." << endl;

		// get all connected volume to point
		connected = get_Connected_Point(solventACC, channelACC, gp); //modify channelACC
      if (DEBUG > 0)
         cerr << "Connected voxel volume: " << connected << endl;

		// remove channel from total solvent
		subt_Grids(solventACC, channelACC); //modify solventACC

		// initialize volume for excluded surface
		int channelACCvol = copyGrid(channelACC, channelEXC); // initialize channelEXC volume

		// statistics
		if (channelACCvol > maxvox)
			maxvox = channelACCvol;
		if (channelACCvol < minvox and channelACCvol > 0)
			minvox = channelACCvol;

		// skip volume if it is too small
		if (channelACCvol <= MINSIZE) {
			// print message if it is worth it
			if (channelACCvol > 20) {
				cerr << "Skipping channel " << allchannels << ": " 
					<< int(channelACCvol*GRIDVOL) << " Angstroms^3" << endl;
			}
			continue;
		}

		cerr << "Channel volume: " << int(channelACCvol*GRIDVOL) << " Angstroms^3" << endl;

		// statistics
		if (channelACCvol < goodminvox)
			goodminvox = channelACCvol;
		numchannels++;

		// get excluded surface
		grow_ExcludeGrid(SMPROBE, channelACC, channelEXC);

		//limit growth to inside trimgrid
		int chanvox = intersect_Grids(channelEXC, trimgrid); //modifies channelEXC

		// output results
      if (DEBUG > 0)
			cerr << "MRC: " << mrcfile << " -- DIR: " << dirname << endl;

		if (strlen(dirname) < 3)
			getDirname(mrcfile, dirname);
      if (DEBUG > 0)
			cerr << "MRC: " << mrcfile << " -- DIR: " << dirname << endl;
		sprintf(mrcfile, "%schannel-%03d.mrc", dirname, numchannels);

		printVolCout(chanvox);
		cerr << endl;
		writeSmallMRCFile(channelEXC, mrcfile);
		//cerr << "---------------------------------------------" << endl;
	}

	if (numchannels == 0)
		goodminvox = 0;
	cerr << "Channel min size: " << int(minvox*GRIDVOL) << " A (all) " << int(goodminvox*GRIDVOL) << " A (good)" << endl;
	cerr << "Channel max size: " << int(maxvox*GRIDVOL) << " A " << endl;
	cerr << "Used " << numchannels << " of " << allchannels << " channels" << endl;
	if (allchannels > 0) {
		cerr << "Mean size: " << solventACCvol/float(allchannels)*GRIDVOL << " A " << endl;
	}
	cerr << "Cutoff size: " << MINSIZE << " voxels :: "<< MINSIZE*GRIDVOL << " Angstroms" << endl;
	free (channelACC);
	free (channelEXC);
	free (solventACC);
	free (trimgrid);
	cerr << endl << "Program Completed Sucessfully" << endl << endl;
	return 0;
};

