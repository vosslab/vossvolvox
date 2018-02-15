/*
** output.cpp
*/
#include "utils.h"
#include <sys/time.h>

const float INV_SQRT_2 = 0.7071;
const float INV_SQRT_3 = 0.5774;

/*********************************************
**********************************************
        OUTPUT FUNCTIONS
**********************************************
*********************************************/

void write_EZD (gridpt grid[], char outfile[]) {
  cerr << "Writing RIGID EZD to file:" << outfile << endl;
  //starts
  int is=NUMBINS,js=NUMBINS,ks=NUMBINS;
//ends
  int ie=0,je=0,ke=0;
//PARSE
  const float pcat = NUMBINS/60.0;
  float pcut = pcat;
  cerr << "Finding Limits of the GRID..." << endl;
  printBar();
  for(unsigned int ind=0; ind<NUMBINS; ind++) {
    if(float(ind) > pcut) {
      cerr << "^" << flush;
      pcut += pcat;
    }
    if(grid[ind]) {
      const int i = int(ind % DX);
      const int j = int((ind % DXY)/ DX);
      const int k = int(ind / DXY);
      if(i < is) { is = i; }
      if(j < js) { js = j; }
      if(k < ks) { ks = k; }
      if(i > ie) { ie = i; }
      if(j > je) { je = j; }
      if(k > ke) { ke = k; }
    }
  }
  cerr << endl;
//extend
  is--; js--; ks--; ie++; je++; ke++;
//minmaxs
  const float xmin = is * GRID + XMIN;
  const float ymin = js * GRID + YMIN;
  const float zmin = ks * GRID + ZMIN;
  const float xmax = ie * GRID + XMIN;
  const float ymax = je * GRID + YMIN;
  const float zmax = ke * GRID + ZMIN;

  std::ofstream out;
  time_t t;
  out.open(outfile);
  out << "EZD_MAP" << endl;
  out << "! EZD file (c) Neil Voss, 2005" << endl;
  time(&t);
  out << "! Grid: " << GRID << "\tGRIDVOL" << GRIDVOL << "\tWater_Res: " << WATER_RES
        << "\tMaxProbe: " << MAXPROBE << "\tCutoff: " << CUTOFF << endl;
  out << "! Input File: " << XYZRFILE << endl;
  out << "! Date: " << ctime(&t) << "! " << endl;
//CELL: cell size based on GRID NOT on EXTENT, given GRID 100 and
//Dens 0.1 CELL = 10.0 (or maybe not, might be vector pointing axes)
  out << "CELL " << int(xmax-xmin+1) << ".0 " << int(ymax-ymin+1)
        << ".0 " << int(zmax-zmin+1) << ".0 90.0 90.0 90.0" << endl;
//ORIGIN (x,y,z) in voxels (mult by $gridDens to get Angstroms)
//must cover min (x,y,z)
  int OX = int(float(xmin)/GRID-1.0);
  int OY = int(float(ymin)/GRID-1.0);
  int OZ = int(float(zmin)/GRID-1.0);
  out << "ORIGIN " << OX << " " << OY << " " << OZ << endl;
//EXTENT maximum dimensions of actual cell (in voxels)
//must cover max (x,y,z) //defines range(?)
  int MX = ie-is+1; //int(float(xmax-xmin)/GRID+1.0);
  int MY = je-js+1; //int(float(xmax-xmin)/GRID+1.0);
  int MZ = ke-ks+1; //int(float(xmax-xmin)/GRID+1.0);
  out << "EXTENT " << MX << " " << MY << " " << MZ << endl;
//  out << "EXTENT " << DX << " " << DY << " " << DZ << endl; //big version
  out << "GRID " << MX << " " << MY << " " << MZ << endl; //doesn't matter
  out << "SCALE 1.0" << endl;
  out << "MAP" << endl;
  int outcnt = 0;
  float count = 0;
  const float cat = MZ/60.0;
  float cut = cat;

  cerr << "Writing the Grid to [" << outfile << "]..." << endl;
  printBar();

  for(int k=ks; k<=ke; k++) {
  count++;
  if(count > cut) {
    cerr << "^" << flush;
    cut += cat;
  }
  for(int j=js; j<=je; j++) {
  for(int i=is; i<=ie; i++) {
      if(grid[i+j*DX+k*DXY]) {
        out << "1 ";
      } else {
        out << "0 ";
      }
      outcnt++;
      if(outcnt % 7 == 0) { out << endl; }
  }}}

  out << endl << "END" << endl;
  out.close();
  cerr << endl << "done [ wrote " << count << " lines ]" 
	<< endl << endl;
  return;
};

void write_HalfEZD (gridpt grid[], char outfile[]) {
  cerr << "Calculating HALVED EZD for file: " << outfile << endl;
//starts
  int is=NUMBINS,js=NUMBINS,ks=NUMBINS;
//ends
  int ie=0,je=0,ke=0;
//PARSE
  const float pcat = NUMBINS/60.0;
  const float newgrid = GRID*2.0;
  float pcut = pcat;
  cerr << "Finding Limits of the GRID..." << endl;
  printBar();
  for(unsigned int ind=0; ind<NUMBINS; ind++) {
    if(float(ind) > pcut) {
      cerr << "^" << flush;
      pcut += pcat;
    }
    if(grid[ind]) {
      const int i = int(ind % DX);
      const int j = int((ind % DXY)/ DX);
      const int k = int(ind / DXY);
      if(i < is) { is = i; }
      if(j < js) { js = j; }
      if(k < ks) { ks = k; }
      if(i > ie) { ie = i; }
      if(j > je) { je = j; }
      if(k > ke) { ke = k; }
    }
  }
  cerr << endl;
//extend
  is-=3; js-=3; ks-=3; ie+=3; je+=3; ke+=3;
//minmaxs
  const float xmin = is * GRID + XMIN;
  const float ymin = js * GRID + YMIN;
  const float zmin = ks * GRID + ZMIN;
  const float xmax = ie * GRID + XMIN;
  const float ymax = je * GRID + YMIN;
  const float zmax = ke * GRID + ZMIN;

  std::ofstream out;
  time_t t;
  out.open(outfile);
  out << "EZD_MAP" << endl;
  out << "! EZD file (c) Neil Voss, 2005" << endl;
  time(&t);

  out << "! Grid: " << GRID << "\tGRIDVOL" << GRIDVOL << "\tWater_Res: " << WATER_RES
        << "\tMaxProbe: " << MAXPROBE << "\tCutoff: " << CUTOFF << endl;
  out << "! Input File: " << XYZRFILE << endl;
  out << "! THIS GRID IS HALVED; NEW GRID SIZE: " << newgrid << endl;
  out << "! Date: " << ctime(&t) << "! " << endl;
//CELL: cell size based on GRID NOT on EXTENT, given GRID 100 and
//Dens 0.1 CELL = 10.0 (or maybe not, might be vector pointing axes)
  out << "CELL " << int(xmax-xmin+1) << ".0 " << int(ymax-ymin+1)
        << ".0 " << int(zmax-zmin+1) << ".0 90.0 90.0 90.0" << endl;
//ORIGIN (x,y,z) in voxels (mult by $gridDens to get Angstroms)
//must cover min (x,y,z)

  int OX = int((float(xmin)/GRID-1.0)/2.0+0.5);
  int OY = int((float(ymin)/GRID-1.0)/2.0+0.5);
  int OZ = int((float(zmin)/GRID-1.0)/2.0+0.5);
  out << "ORIGIN " << OX << " " << OY << " " << OZ << endl;
//EXTENT maximum dimensions of actual cell (in voxels)
//must cover max (x,y,z) //defines range(?)
  int MX = int((ie-is+1)/2.0+0.5); //int(float(xmax-xmin)/GRID+1.0);
  int MY = int((je-js+1)/2.0+0.5); //int(float(xmax-xmin)/GRID+1.0);
  int MZ = int((ke-ks+1)/2.0+0.5); //int(float(xmax-xmin)/GRID+1.0);
  out << "EXTENT " << MX << " " << MY << " " << MZ << endl;
//  out << "EXTENT " << DX << " " << DY << " " << DZ << endl; //big version
  out << "GRID " << MX << " " << MY << " " << MZ << endl; //doesn't matter
  out << "SCALE 1.0" << endl;
  out << "MAP" << endl;
  int outcnt = 0;
  float count = 0;
  const float cat = MZ/60.0;
  float cut = cat;

  cerr << "Halving GRID and Writing to EZD File: " << outfile << endl;
  cerr << "THIS MAY TAKE A LONG TIME" << endl;
  printBar();

  for(int k=ks; k<=ke; k+=2) {
  count++;
  if(count > cut) {
    cerr << "^" << flush;
    cut += cat;
  }
  for(int j=js; j<=je; j+=2) {
  for(int i=is; i<=ie; i+=2) {
      int index = i+j*DX+k*DXY;
//HARD CODE IT; 8 VOXELS --> 64 VOXELS
//MAIN EIGHT; TOTAL 8 POINTS
      long double sum = grid[index] + grid[index+1]
	+ grid[index+DX] + grid[index+DX+1]
	+ grid[index+DXY] + grid[index+DXY+1]
	+ grid[index+DX+DXY] + grid[index+DX+DXY+1];
//FIRST DIST, 24 VOXELS; TOTAL 12 POINTS
      sum += 0.2*(grid[index-1] + grid[index-1+DX]
	+ grid[index-DX] + grid[index-DX+1]
	+ grid[index+2] + grid[index+2+DX]
	+ grid[index+2*DX] + grid[index+2*DX+1]

	+ grid[index-1+DXY] + grid[index-1+DX+DXY]
	+ grid[index-DX+DXY] + grid[index-DX+1+DXY]
	+ grid[index+2+DXY] + grid[index+2+DX+DXY]
	+ grid[index+2*DX+DXY] + grid[index+2*DX+1+DXY]

	+ grid[index-DXY] + grid[index-DXY+1]
	+ grid[index-DXY+DX] + grid[index-DXY+DX+1]

	+ grid[index+2*DXY] + grid[index+2*DXY+1]
	+ grid[index+2*DXY+DX] + grid[index+2*DXY+DX+1]
	);
//SECOND DIST, 24 POINTS; TOTAL 8.48 POINTS
//THIRD DIST, 8 POINTS; TOTAL 2.31 POINTS
      //long double upper = 8.0 + 24.0*0.2;
      if(sum >= 0.5 && sum <= 12.7) {
        out << int(100000.0*sum/20.0+0.5)/100000.0 << " " << flush;
      } else if(sum > 12.7) {
        out << "1 " << flush;
      } else {
        out << "0 " << flush;
      }
      outcnt++;
      if(outcnt % 7 == 0) { out << endl; }
  }}}

  out << endl << "END" << endl;
  out.close();
  cerr << endl << "done [ wrote " << count << " lines ]" 
	<< endl << endl;
  return;
};


void write_BlurEZD (gridpt grid[], char outfile[]) {
  cerr << "Calculating SMOOTH EZD for file: " << outfile << endl;
//starts
  int is=NUMBINS,js=NUMBINS,ks=NUMBINS;
//ends
  int ie=0,je=0,ke=0;
//PARSE
  const float pcat = NUMBINS/60.0;
  float pcut = pcat;
  cerr << "Finding Limits of the GRID..." << endl;
  printBar();
  for(unsigned int ind=0; ind<NUMBINS; ind++) {
    if(float(ind) > pcut) {
      cerr << "^" << flush;
      pcut += pcat;
    }
    if(grid[ind]) {
      const int i = int(ind % DX);
      const int j = int((ind % DXY)/ DX);
      const int k = int(ind / DXY);
      if(i < is) { is = i; }
      if(j < js) { js = j; }
      if(k < ks) { ks = k; }
      if(i > ie) { ie = i; }
      if(j > je) { je = j; }
      if(k > ke) { ke = k; }
    }
  }
  cerr << endl;
//extend
  is-=3; js-=3; ks-=3; ie+=3; je+=3; ke+=3;
//minmaxs
  const float xmin = is * GRID + XMIN;
  const float ymin = js * GRID + YMIN;
  const float zmin = ks * GRID + ZMIN;
  const float xmax = ie * GRID + XMIN;
  const float ymax = je * GRID + YMIN;
  const float zmax = ke * GRID + ZMIN;

  std::ofstream out;
  time_t t;
  out.open(outfile);
  out << "EZD_MAP" << endl;
  out << "! EZD file (c) Neil Voss, 2005" << endl;
  time(&t);
  out << "! Grid: " << GRID << "\tGRIDVOL" << GRIDVOL << "\tWater_Res: " << WATER_RES
        << "\tMaxProbe: " << MAXPROBE << "\tCutoff: " << CUTOFF << endl;
  out << "! Input File: " << XYZRFILE << endl;
  out << "! Date: " << ctime(&t) << "! " << endl;
//CELL: cell size based on GRID NOT on EXTENT, given GRID 100 and
//Dens 0.1 CELL = 10.0 (or maybe not, might be vector pointing axes)
  out << "CELL " << int(xmax-xmin+1) << ".0 " << int(ymax-ymin+1)
        << ".0 " << int(zmax-zmin+1) << ".0 90.0 90.0 90.0" << endl;
//ORIGIN (x,y,z) in voxels (mult by $gridDens to get Angstroms)
//must cover min (x,y,z)
  int OX = int(float(xmin)/GRID-1.0);
  int OY = int(float(ymin)/GRID-1.0);
  int OZ = int(float(zmin)/GRID-1.0);
  out << "ORIGIN " << OX << " " << OY << " " << OZ << endl;
//EXTENT maximum dimensions of actual cell (in voxels)
//must cover max (x,y,z) //defines range(?)
  int MX = ie-is+1; //int(float(xmax-xmin)/GRID+1.0);
  int MY = je-js+1; //int(float(xmax-xmin)/GRID+1.0);
  int MZ = ke-ks+1; //int(float(xmax-xmin)/GRID+1.0);
  out << "EXTENT " << MX << " " << MY << " " << MZ << endl;
//  out << "EXTENT " << DX << " " << DY << " " << DZ << endl; //big version
  out << "GRID " << MX << " " << MY << " " << MZ << endl; //doesn't matter
  out << "SCALE 1.0" << endl;
  out << "MAP" << endl;
  int outcnt = 0;
  float count = 0;
  const float cat = MZ/60.0;
  float cut = cat;
  const float fill = 21.1044; //8*INV_SQRT_3 + 12*INV_SQRT_2 + 6*1 + 2;

  cerr << "Smoothing GRID and Writing to EZD File: " << outfile << endl;
  cerr << "THIS MAY TAKE A LONG TIME" << endl;
  printBar();

  for(int k=ks; k<=ke; k++) {
  count++;
  if(count > cut) {
    cerr << "^" << flush;
    cut += cat;
  }
  for(int j=js; j<=je; j++) {
  for(int i=is; i<=ie; i++) {
      int index = i+j*DX+k*DXY;
      float sum = 0.0;
      int dist;
      for(int di=-1; di<=1; di++) {
      for(int dj=-1; dj<=1; dj++) {
      for(int dk=-1; dk<=1; dk++) {
        if(grid[index+di+dj*DX+dk*DXY]) {
          dist = abs(di)+abs(dj)+abs(dk);
          if(dist == 0) {
            sum += 2.0;
          } else if (dist == 1) {
            sum += 1.0;
          } else if (dist == 2) {
            sum += INV_SQRT_2;
          } else if (dist == 3) {
            sum += INV_SQRT_3;
          }
        }
      }}}
      if(sum > 0.5 && sum < 21) {
        out << int(10000.0*sum/fill+0.5)/10000.0 << " " << flush;
      } else if(sum >= 21) {
        out << "1 " << flush;
      } else {
        out << "0 " << flush;
      }
      outcnt++;
      if(outcnt % 7 == 0) { out << endl; }
  }}}

  out << endl << "END" << endl;
  out.close();
  cerr << endl << "done [ wrote " << count << " lines ]" 
	<< endl << endl;
  return;
};

void write_PDB (gridpt grid[], char outfile[]) {
  cerr << "Writing FULL PDB to file:" << outfile << endl;
  std::ofstream out;
  time_t t;
  out.open(outfile);
  out << "REMARK (c) Neil Voss, 2005" << endl;
  out << "REMARK PDB file created from " << XYZRFILE << endl;
  out << "REMARK Grid: " << GRID << "\tGRIDVOL: " << GRIDVOL << "\tWater_Res: " << WATER_RES
        << "\tMaxProbe: " << MAXPROBE << "\tCutoff: " << CUTOFF << endl;
  time(&t);
  out << "REMARK Date: " << ctime(&t) << flush;
  float count = 0;
  int anum=0;
  const float cat = DZ/60.0;
  float cut = cat;
  char line[128];

  cerr << "Writing the grid to [ " << outfile << " ]..." << endl;
  printBar();
  int sk=-1, sj=-1;
  for(int k=0; k<DXYZ; k+=DXY) {
    sk++;
    count++;
    if(count > cut) {
      cerr << "^" << flush;
      cut += cat;
    }
    sj=-1;
    for(int j=0; j<DXY; j+=DX) {
      sj++;
      for(int i=0; i<DX; i++) {
        if(grid[i+j+k]) {
          anum++;
          ijk2pdb(line,i,sj,sk,anum);
          out << line << endl;
        }
  } } }

  out << endl;
  out.close();
  cerr << endl << "done wrote " << anum << " atoms" << endl << endl;
  return;
};


void write_SurfPDB (gridpt grid[], char outfile[]) {
  cerr << "Writing SURFACE PDB to file:" << outfile << endl;
  std::ofstream out;
  time_t t;
  out.open(outfile);
  out << "REMARK (c) Neil Voss, 2005" << endl;
  out << "REMARK PDB file created from " << XYZRFILE << endl;
  out << "REMARK Grid: " << GRID << "\tGRIDVOL: " << GRIDVOL 
	<< "\tWater_Res: " << WATER_RES
        << "\tMaxProbe: " << MAXPROBE << "\tCutoff: " << CUTOFF << endl;
  time(&t);
  out << "REMARK Date: " << ctime(&t) << flush;
  float count = 0;
  int anum=0, pnum=0;
  const float cat = DZ/60.0;
  float cut = cat;
  char line[128];

  cerr << "Writing the grid to [" << outfile << "]..." << endl;
  printBar();

  int sk=-1, sj=-1;
  for(int k=0; k<DXYZ; k+=DXY) {
    sk++;
    count++;
    if(count > cut) {
      cerr << "^" << flush;
      cut += cat;
    }
    sj=-1;
    for(int j=0; j<DXY; j+=DX) {
      sj++;
      for(int i=0; i<DX; i++) {
        int pt = i+j+k;
        if(grid[pt]) {
          pnum++;
          if(isEdgePoint_Star(pt,grid)) {
            anum++;
            ijk2pdb(line,i,sj,sk,anum);
            out << line << endl;
          }
        }
  } } }

  out << endl;
  out.close();
  cerr << endl << "done wrote " << anum << " of " << pnum << endl << endl;
  return;
};

void write_ThirdEZD (gridpt grid[], char outfile[]) {
  cerr << "Calculating THIRDED EZD for file: " << outfile << endl;
//starts
  int is=NUMBINS,js=NUMBINS,ks=NUMBINS;
//ends
  int ie=0,je=0,ke=0;
//PARSE
  const float pcat = NUMBINS/60.0;
  const float newgrid = GRID*3.0;
  float pcut = pcat;
  cerr << "Finding Limits of the GRID..." << endl;
  printBar();
  for(unsigned int ind=0; ind<NUMBINS; ind++) {
    if(float(ind) > pcut) {
      cerr << "^" << flush;
      pcut += pcat;
    }
    if(grid[ind]) {
      const int i = int(ind % DX);
      const int j = int((ind % DXY)/ DX);
      const int k = int(ind / DXY);
      if(i < is) { is = i; }
      if(j < js) { js = j; }
      if(k < ks) { ks = k; }
      if(i > ie) { ie = i; }
      if(j > je) { je = j; }
      if(k > ke) { ke = k; }
    }
  }
  cerr << endl;
//extend
  is-=4; js-=4; ks-=4; ie+=4; je+=4; ke+=4;
//minmaxs
  const float xmin = is * GRID + XMIN;
  const float ymin = js * GRID + YMIN;
  const float zmin = ks * GRID + ZMIN;
  const float xmax = ie * GRID + XMIN;
  const float ymax = je * GRID + YMIN;
  const float zmax = ke * GRID + ZMIN;

  std::ofstream out;
  time_t t;
  out.open(outfile);
  out << "EZD_MAP" << endl;
  out << "! EZD file (c) Neil Voss, 2005" << endl;
  time(&t);

  out << "! Grid: " << GRID << "\tGRIDVOL" << GRIDVOL << "\tWater_Res: " << WATER_RES
        << "\tMaxProbe: " << MAXPROBE << "\tCutoff: " << CUTOFF << endl;
  out << "! Input File: " << XYZRFILE << endl;
  out << "! THIS GRID IS THIRDED; NEW GRID SIZE: " << newgrid << endl;
  out << "! Date: " << ctime(&t) << "! " << endl;
//CELL: cell size based on GRID NOT on EXTENT, given GRID 100 and
//Dens 0.1 CELL = 10.0 (or maybe not, might be vector pointing axes)
  out << "CELL " << int(xmax-xmin+1) << ".0 " << int(ymax-ymin+1)
        << ".0 " << int(zmax-zmin+1) << ".0 90.0 90.0 90.0" << endl;
//ORIGIN (x,y,z) in voxels (mult by $gridDens to get Angstroms)
//must cover min (x,y,z)

  int OX = int((float(xmin)/GRID-1.0)/3.0+0.5);
  int OY = int((float(ymin)/GRID-1.0)/3.0+0.5);
  int OZ = int((float(zmin)/GRID-1.0)/3.0+0.5);
  out << "ORIGIN " << OX << " " << OY << " " << OZ << endl;
//EXTENT maximum dimensions of actual cell (in voxels)
//must cover max (x,y,z) //defines range(?)
  int MX = int((ie-is+1)/3.0+0.5); //int(float(xmax-xmin)/GRID+1.0);
  int MY = int((je-js+1)/3.0+0.5); //int(float(xmax-xmin)/GRID+1.0);
  int MZ = int((ke-ks+1)/3.0+0.5); //int(float(xmax-xmin)/GRID+1.0);
  out << "EXTENT " << MX << " " << MY << " " << MZ << endl;
//  out << "EXTENT " << DX << " " << DY << " " << DZ << endl; //big version
  out << "GRID " << MX << " " << MY << " " << MZ << endl; //doesn't matter
  out << "SCALE 1.0" << endl;
  out << "MAP" << endl;
  int outcnt = 0;
  float count = 0;
  const float cat = MZ/60.0;
  float cut = cat;

  cerr << "Thirding GRID and Writing to EZD File: " << outfile << endl;
  cerr << "THIS MAY TAKE A LONG TIME" << endl;
  printBar();

  for(int k=ks; k<=ke; k+=3) {
  count++;
  if(count > cut) {
    cerr << "^" << flush;
    cut += cat;
  }
  for(int j=js; j<=je; j+=3) {
  for(int i=is; i<=ie; i+=3) {
      int index = i+j*DX+k*DXY;
//INTERIOR:        3 vox * 3 vox    = 27 vox
//OUTSIDE FACES:   9 vox * 6 faces  = 54 vox
//OUTSIDE EDGES:   3 vox * 12 edges = 36 vox
//OUTSIDE CORNERS: 8 vox
      int interior = 0;
      int face = 0;
      int edge = 0;
      int corner = 0;
      for(int di=-2; di<=2; di++) {
      for(int dj=-2; dj<=2; dj++) {
      for(int dk=-2; dk<=2; dk++) {
        int dist = di*di + dj*dj + dk*dk;
        if(dist > 11) { //corner
          if(grid[index + di + dj*DX + dk*DXY]) { corner++; }
        } else if(dist > 7) { //edge
          if(grid[index + di + dj*DX + dk*DXY]) { edge++; }
        } else if(dist > 3) { //face
          if(grid[index + di + dj*DX + dk*DXY]) { face++; }
        } else { //interior
          if(grid[index + di + dj*DX + dk*DXY]) { interior++; }
        }
      }}}
      int total = corner + edge + face + interior;
//OUTPUT RANGE FROM ZERO TO ONE
      if(total == 0) {
        out << "0 " << flush;
      } else if(total >= 125) {
        out << "1 " << flush;
      } else {
//INTERIOR  = 6000/27 = 222.2 => 0.538
//FACE      = 2500/54 =  46.3 => 0.135
//EDGE      = 1300/36 =  36.1 => 0.100
//CORNER,   =  200/8  =  25.0 => 0.227
        long int sum = interior*2222 + face*463 + 
        	edge*361 + corner*250;
        out << double(sum)/100000.0 << " " << flush;
      }
      
      outcnt++;
      if(outcnt % 7 == 0) { out << endl; }
  }}}

  out << endl << "END" << endl;
  out.close();
  cerr << endl << "done [ wrote " << count << " lines ]" 
	<< endl << endl;
  return;
};

void write_FifthEZD (gridpt grid[], char outfile[]) {
  cerr << "Calculating FIFTHED EZD for file: " << outfile << endl;
//starts
  int is=NUMBINS,js=NUMBINS,ks=NUMBINS;
//ends
  int ie=0,je=0,ke=0;
//PARSE
  const float pcat = NUMBINS/60.0;
  const float newgrid = GRID*5.0;
  float pcut = pcat;
  cerr << "Finding Limits of the GRID..." << endl;
  printBar();
  for(unsigned int ind=0; ind<NUMBINS; ind++) {
    if(float(ind) > pcut) {
      cerr << "^" << flush;
      pcut += pcat;
    }
    if(grid[ind]) {
      const int i = int(ind % DX);
      const int j = int((ind % DXY)/ DX);
      const int k = int(ind / DXY);
      if(i < is) { is = i; }
      if(j < js) { js = j; }
      if(k < ks) { ks = k; }
      if(i > ie) { ie = i; }
      if(j > je) { je = j; }
      if(k > ke) { ke = k; }
    }
  }
  cerr << endl;
//extend
  is-=6; js-=6; ks-=6; ie+=6; je+=6; ke+=6;
//minmaxs
  const float xmin = is * GRID + XMIN;
  const float ymin = js * GRID + YMIN;
  const float zmin = ks * GRID + ZMIN;
  const float xmax = ie * GRID + XMIN;
  const float ymax = je * GRID + YMIN;
  const float zmax = ke * GRID + ZMIN;

  std::ofstream out;
  time_t t;
  out.open(outfile);
  out << "EZD_MAP" << endl;
  out << "! EZD file (c) Neil Voss, 2005" << endl;
  time(&t);

  out << "! Grid: " << GRID << "\tGRIDVOL" << GRIDVOL << "\tWater_Res: " << WATER_RES
        << "\tMaxProbe: " << MAXPROBE << "\tCutoff: " << CUTOFF << endl;
  out << "! Input File: " << XYZRFILE << endl;
  out << "! THIS GRID IS FIFTHED; NEW GRID SIZE: " << newgrid << endl;
  out << "! Date: " << ctime(&t) << "! " << endl;
//CELL: cell size based on GRID NOT on EXTENT, given GRID 100 and
//Dens 0.1 CELL = 10.0 (or maybe not, might be vector pointing axes)
  out << "CELL " << int(xmax-xmin+1) << ".0 " << int(ymax-ymin+1)
        << ".0 " << int(zmax-zmin+1) << ".0 90.0 90.0 90.0" << endl;
//ORIGIN (x,y,z) in voxels (mult by $gridDens to get Angstroms)
//must cover min (x,y,z)

  int OX = int((float(xmin)/GRID-1.0)/5.0+0.5);
  int OY = int((float(ymin)/GRID-1.0)/5.0+0.5);
  int OZ = int((float(zmin)/GRID-1.0)/5.0+0.5);
  out << "ORIGIN " << OX << " " << OY << " " << OZ << endl;
//EXTENT maximum dimensions of actual cell (in voxels)
//must cover max (x,y,z) //defines range(?)
  int MX = int((ie-is+1)/5.0+0.5); //int(float(xmax-xmin)/GRID+1.0);
  int MY = int((je-js+1)/5.0+0.5); //int(float(xmax-xmin)/GRID+1.0);
  int MZ = int((ke-ks+1)/5.0+0.5); //int(float(xmax-xmin)/GRID+1.0);
  out << "EXTENT " << MX << " " << MY << " " << MZ << endl;
//  out << "EXTENT " << DX << " " << DY << " " << DZ << endl; //big version
  out << "GRID " << MX << " " << MY << " " << MZ << endl; //doesn't matter
  out << "SCALE 1.0" << endl;
  out << "MAP" << endl;
  int outcnt = 0;
  float count = 0;
  const float cat = MZ/60.0;
  float cut = cat;

  cerr << "Thirding GRID and Writing to EZD File: " << outfile << endl;
  cerr << "THIS MAY TAKE A LONG TIME" << endl;
  printBar();

  for(int k=ks; k<=ke; k+=5) {
  count++;
  if(count > cut) {
    cerr << "^" << flush;
    cut += cat;
  }
  for(int j=js; j<=je; j+=5) {
  for(int i=is; i<=ie; i+=5) {
      int index = i+j*DX+k*DXY;
//INTERIOR:        5 * 5 vox - 8 + 6      = 123 vox
//OUTSIDE FACES:   25 v * 6 faces + 8 - 6 = 152 vox
//OUTSIDE EDGES:   5 vox * 12 edges       = 60 vox
//OUTSIDE CORNERS: 8 vox
      int interior = 0;
      int face = 0;
      int edge = 0;
      int corner = 0;
      for(int di=-3; di<=3; di++) {
      for(int dj=-3; dj<=3; dj++) {
      for(int dk=-3; dk<=3; dk++) {
          int dist = di*di + dj*dj + dk*dk;
          if(dist > 26) { //corner
            if(grid[index + di + dj*DX + dk*DXY]) { corner++; }
          } else if(dist > 17) { //edge
            if(grid[index + di + dj*DX + dk*DXY]) { edge++; }
          } else if(dist > 9) { //face
            if(grid[index + di + dj*DX + dk*DXY]) { face++; }
          } else { //interior, not perfect
            if(grid[index + di + dj*DX + dk*DXY]) { interior++; }
          }
      }}}
      int total = corner + edge + face + interior;
//OUTPUT RANGE FROM ZERO TO ONE
      if(total == 0) {
        out << "0 " << flush;
      } else if(total >= 343) {
        out << "1 " << flush;
      } else {
//INTERIOR  = 6000/123 = 48.8 => 0.538
//FACE      = 3000/152 = 19.7 => 0.135
//EDGE      =  900/60  = 15.0 => 0.100
//CORNER,   =  100/8   = 12.5 => 0.227
        long int sum = interior*488 + face*197 + 
        	edge*150 + corner*125;
        out << double(sum)/100000.0 << " " << flush;
      }
      
      outcnt++;
      if(outcnt % 7 == 0) { out << endl; }
  }}}

  out << endl << "END" << endl;
  out.close();
  cerr << endl << "done [ wrote " << count << " lines ]" 
	<< endl << endl;
  return;
};
