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
  double BIGPROBE=10.0;
  double PROBESTEP=0.1;
  double TRIMPROBE=1.5; //HEADER INFO

  while(argc > 1 && argv[1][0] == '-') {
    if(argv[1][1] == 'i') {
      sprintf(file,&argv[2][0]);
    } else if(argv[1][1] == 's') {
      PROBESTEP = atof(&argv[2][0]);
    } else if(argv[1][1] == 'b') {
      BIGPROBE = atof(&argv[2][0]);
    } else if(argv[1][1] == 't') {
      TRIMPROBE = atof(&argv[2][0]);
    } else if(argv[1][1] == 'g') {
      GRID = atof(&argv[2][0]);
    } else if(argv[1][1] == 'o') {
      sprintf(pdbfile,&argv[2][0]);
    } else if(argv[1][1] == 'e') {
      sprintf(ezdfile,&argv[2][0]);
    } else if(argv[1][1] == 'm') {
      sprintf(mrcfile,&argv[2][0]);
    } else if(argv[1][1] == 'h') {
      cerr << "./FsvCalc.exe -i <file> -b <big_probe> -s <probe_step> " << endl
        << "\t-t <trim probe> -g <gridspace> " << endl;
      //  << "\t-e <EZD outfile> -o <PDB outfile> -m <MRC outfile> " << endl;
      cerr << "FsvCalc.exe -- Calculates the Fractional Solvent Volume" << endl;
      cerr << endl;
      return 1;
    }
    --argc; --argc;
    ++argv; ++argv;
  }


//INITIALIZE GRID
  finalGridDims(BIGPROBE);

//HEADER CHECK
  cerr << "Grid Spacing: " << GRID << endl;
  cerr << "Resolution:      " << int(1000.0/float(GRIDVOL))/1000.0 << " voxels per A^3" << endl;
  cerr << "Resolution:      " << int(11494.0/float(GRIDVOL))/1000.0 << " voxels per water molecule" << endl;
  cerr << "Complexity:      " << int(8000000/float(GRIDVOL))/1000.0 << endl;
  cerr << "Input file:   " << file << endl;

//FIRST PASS, MINMAX
  int numatoms = read_NumAtoms(file);

//CHECK LIMITS & SIZE
  assignLimits();

// ****************************************************
// STARTING FIRST FILE
// ****************************************************

//GET SHELL
  gridpt *shell;
  shell = (gridpt*) malloc (NUMBINS);
  if (shell==NULL) { cerr << "GRID IS NULL" << endl; exit (1); }
  zeroGrid(shell);
  int shellvol = get_ExcludeGrid_fromFile(numatoms,BIGPROBE,file,shell);

//INIT NEW smShellACC GRID
  cerr << "Trimming Radius: " << TRIMPROBE << endl;
  gridpt *smShell;
  smShell = (gridpt*) malloc (NUMBINS);
  if (smShell==NULL) { cerr << "GRID IS NULL" << endl; exit (1); }

//COPY AND TRUNCATE (IF NECESSARY)
  copyGrid(shell,smShell);
  if(TRIMPROBE > 0) {
    trun_ExcludeGrid(TRIMPROBE,shell,smShell);
  }
  free (shell);

// ****************************************************
// STARTING LOOP OVER PROBE SIZE
// ****************************************************

  cout << "probe\tshell_vol\tsolvent_vol\tfsv\tfile" << endl;

  for (double SMPROBE=0.0; SMPROBE<BIGPROBE; SMPROBE+=PROBESTEP) {
	//COPY SMSHELL INTO CHANACC
	  gridpt *solventACC;
	  solventACC = (gridpt*) malloc (NUMBINS);
	  if (solventACC==NULL) { cerr << "GRID IS NULL" << endl; exit (1); }
	  int smshellvol = copyGrid(smShell,solventACC);

	//SUBTRACT PROBE_ACC FROM SHELL TO GET ACC CHANNELS
	  gridpt *probeACC;
	  probeACC = (gridpt*) malloc (NUMBINS);
	  if (probeACC==NULL) { cerr << "GRID IS NULL" << endl; exit (1); }
	  zeroGrid(probeACC);
	  fill_AccessGrid_fromFile(numatoms, SMPROBE, file, probeACC);
	  subt_Grids(solventACC, probeACC);
	  free (probeACC);

	//INIT NEW solventEXC GRID
	  gridpt *solventEXC;
	  solventEXC = (gridpt*) malloc (NUMBINS);
	  if (solventEXC==NULL) { cerr << "GRID IS NULL" << endl; exit (1); }

	//GROW EXCLUDED SURFACE FROM ACCESSIBLE
	  //copyGrid(solventACC,solventEXC);
	  zeroGrid(solventEXC);
	  grow_ExcludeGrid(SMPROBE, solventACC, solventEXC);
	  free (solventACC);

	//INTERSECT
	  intersect_Grids(solventEXC, smShell); //modifies solventEXC

	//OUTPUT
	  int solventvol = countGrid(solventEXC);
	  cout << SMPROBE << "\t";
	  printVolCout(shellvol);
	  printVolCout(solventvol);
	  double fsv = double(solventvol) / double(shellvol);
	  cout << fsv << "\t" << file << endl;

    /*
	  if(pdbfile[0] != '\0') {
		write_SurfPDB(solventEXC, pdbfile);
	  }
	  if(ezdfile[0] != '\0') {
		write_HalfEZD(solventEXC, ezdfile);
	  }
	  if(mrcfile[0] != '\0') {
		writeMRCFile(solventEXC, mrcfile);
	  }
	*/
	  free (solventEXC);

  }
  free (smShell);

  cerr << endl << "Program Completed Sucessfully" << endl << endl;
  return 0;
};

