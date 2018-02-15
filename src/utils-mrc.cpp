#include "utils.h"
#include <stdint.h>

#define MRC_MODE_BYTE            0
#define MRC_MODE_SHORT           1
#define MRC_MODE_FLOAT           2
#define MRC_MODE_SHORT_COMPLEX   3
#define MRC_MODE_FLOAT_COMPLEX   4
#define MRC_MODE_UNSIGNED_SHORT  5

#define MRC_COUNT          856    /* Number of freads for a complete header */
#define MRC_USERS           25
#define MRC_LABEL_SIZE      80
#define MRC_NUM_LABELS      10

typedef   int8_t		s08;
typedef   uint8_t		u08;
typedef   int16_t		s16;
typedef   uint16_t	u16;
typedef   int32_t		s32;
typedef   uint32_t	u32;
typedef   float		f32;

/*********************************************/
typedef struct MRCHeaderSt {
	s32    nx;                    /* Number of columns */
	s32    ny;                    /* Number of rows */
	s32    nz;                    /* Number of sections */
	s32    mode;                    /* See modes above. */
	s32    nxstart;                /* No. of first column in map   default 0.*/
	s32    nystart;                /* No. of first row in map  default 0.*/
	s32    nzstart;                /* No. of first section in map  default 0.*/
	s32    mx;                    /* Number of intervals along X. */
	s32    my;                    /* Number of intervals along Y. */
	s32    mz;                    /* Number of intervals along Z. */
	f32    x_length;            /* Cell dimensions (Angstroms). */
	f32    y_length;            /* Cell dimensions (Angstroms). */
	f32    z_length;            /* Cell dimensions (Angstroms). */
	f32    alpha;                /* Cell angles (Degrees). */
	f32    beta;                    /* Cell angles (Degrees). */
	f32    gamma;                /* Cell angles (Degrees). */
	s32    mapc;                    /* Which axis corresponds to Columns.  */
	s32    mapr;                    /* Which axis corresponds to Rows. */
	s32    maps;                    /* Which axis corresponds to Sections. */
	f32    amin;                    /* Minimum density value. */
	f32    amax;                    /* Maximum density value. */
	f32    amean;                /* Mean density value.*/
	s32    ispg;                    /* Space group number (0 for images) */
	s32    nsymbt;                /* Number of bytes used for storing symmetry operators */
	s32    extra[MRC_USERS];    /* For user, all set to zero by default */
	f32    xorigin;                /* X origin */
	f32    yorigin;                /* Y origin */
	f32    zorigin;                /* Z origin */
	s32    map;                    /* Identify file type */
	s32    mach;                    /* Machine Stamp */
	f32    rms;                    /* Standard Deviation */
	s32    nlabl;                /* Number of labels being used. */
	s08    label[MRC_NUM_LABELS][MRC_LABEL_SIZE];     /* 10 text labels of 80 characters each. */
} MRCHeaderSt;

typedef MRCHeaderSt * MRCHeaderP;

//FROM utils.h external variables
extern float XMIN, YMIN, ZMIN;
//extern float XMAX, YMAX, ZMAX;
extern int DX, DY, DZ;
//extern int DXY, DXYZ;
extern unsigned int NUMBINS;
extern float GRID;
//extern float GRIDVOL;
//END utils.h


/*********************************************/
int byteWrite( FILE * fp, void * data, int number_of_elements, int element_size ) {
	int count = fwrite(data, element_size, number_of_elements, fp);
	return count;
}

/*********************************************/
int byteSwapWriteGrid( FILE * fp, gridpt * data, int number_of_elements, int element_size ) {
	int count = fwrite(data, element_size, number_of_elements, fp);
	return count;
	
}

/*********************************************/
int writeMRCHeader( FILE * fp, MRCHeaderSt header ) {
	
	int count = 0;
	
	int users = MRC_USERS;
	int label = MRC_LABEL_SIZE * MRC_NUM_LABELS;
	
	count += byteWrite(fp, &header.nx,        1, 4 );
	count += byteWrite(fp, &header.ny,        1, 4 );
	count += byteWrite(fp, &header.nz,        1, 4 );
	count += byteWrite(fp, &header.mode,      1, 4 );
	count += byteWrite(fp, &header.nxstart,   1, 4 );
	count += byteWrite(fp, &header.nystart,   1, 4 );
	count += byteWrite(fp, &header.nzstart,   1, 4 );
	count += byteWrite(fp, &header.mx,        1, 4 );
	count += byteWrite(fp, &header.my,        1, 4 );
	count += byteWrite(fp, &header.mz,        1, 4 );
	count += byteWrite(fp, &header.x_length,  1, 4 );
	count += byteWrite(fp, &header.y_length,  1, 4 );
	count += byteWrite(fp, &header.z_length,  1, 4 );
	count += byteWrite(fp, &header.alpha,     1, 4 );
	count += byteWrite(fp, &header.beta,      1, 4 );
	count += byteWrite(fp, &header.gamma,     1, 4 );
	count += byteWrite(fp, &header.mapc,      1, 4 );
	count += byteWrite(fp, &header.mapr,      1, 4 );
	count += byteWrite(fp, &header.maps,      1, 4 );
	count += byteWrite(fp, &header.amin,      1, 4 );
	count += byteWrite(fp, &header.amax,      1, 4 );
	count += byteWrite(fp, &header.amean,     1, 4 );
	count += byteWrite(fp, &header.ispg,      1, 4 );
	count += byteWrite(fp, &header.nsymbt,    1, 4 );
	count += byteWrite(fp, &header.extra, users, 4 );
	count += byteWrite(fp, &header.xorigin,   1, 4 );
	count += byteWrite(fp, &header.yorigin,   1, 4 );
	count += byteWrite(fp, &header.zorigin,   1, 4 );
	count += byteWrite(fp, &header.map,       1, 4 );
	count += byteWrite(fp, &header.mach,      1, 4 );
	count += byteWrite(fp, &header.rms,       1, 4 );
	count += byteWrite(fp, &header.nlabl,     1, 4 );
	count += byteWrite(fp, &header.label, label, 1 );
	
	if ( count != MRC_COUNT ) return -1;
	else return 1;
	
}

/*********************************************/
int writeMRCFile( gridpt data[], char filename[] ) {
	int volume = countGrid(data);
	if (volume == 0) {
		cerr << "volume is empty not writing mrc file" << endl;
		return 0;
	}
	cerr << "MRC dims: " << DX << " x " << DY << " x " << DZ << endl;
	cerr << "writing complete grid to MRC file: " << filename << endl;
/*
//PART I: Determine Extrema
	int minmax[6];
	int xmin, ymin, zmin;
	int xmax, ymax, zmax;
	determine_MinMax(data, minmax);
	xmin = minmax[0]-1;
	ymin = minmax[1]/DX-1;
	zmin = minmax[2]/DXY-1;
	xmax = minmax[3]+1;
	ymax = minmax[4]/DX+1;
	zmax = minmax[5]/DXY+1;
	cerr << "Minima: " << xmin << " , " << ymin << " , " << zmin << endl;
	cerr << "Maxima: " << xmax << " , " << ymax << " , " << zmax << endl;

//PART II: Calculate new dimensions
	int xdim, ydim, zdim;
	xdim = int((xmax-xmin)/4.0 +1.0)*4;
	ydim = int((ymax-ymin)/4.0 +1.0)*4;
	zdim = int((zmax-zmin)/4.0 +1.0)*4;
	cerr << "Old dimensions: " << DX << " , " << DY << " , " << DZ << endl;
	cerr << "New dimensions: " << xdim << " , " << ydim << " , " << zdim << endl;

	cerr << "PDB Minima: " << XMIN << " , " << YMIN << " , " << ZMIN << endl;
	cerr << "PDB Minima: " << XMIN/GRID << " , " << YMIN/GRID << " , " << ZMIN/GRID << endl;
*/

//PART III: Use old dimensions
	FILE * fp = fopen(filename, "wb");
	if ( fp == NULL ) return -1;
	
	// Documentation, http://bio3d.colorado.edu/imod/doc/mrc_format.txt
	MRCHeaderSt header;
	//cerr << XMIN << "\t" << YMIN << "\t" << ZMIN << endl;
	//cerr << XMAX << "\t" << YMAX << "\t" << ZMAX << endl;
	//cerr << DX << "\t" << DY << "\t" << DZ << endl;
	//cerr << (XMAX-XMIN)/2.0 << "\t" << (YMAX-YMIN)/2.0 << "\t" << (ZMAX-ZMIN)/2.0 << endl;
	//cerr << (DX-1.0)/2.0*GRID+XMIN << "\t" << (DY-1.0)/2.0*GRID+YMIN << "\t" << (DZ-1.0)/2.0*GRID+ZMIN << endl;
	header.nx           = DX;
	header.ny           = DY;
	header.nz           = DZ;
	header.mx           = DX;
	header.my           = DY;
	header.mz           = DZ;
	header.x_length     = DX*GRID;
	header.y_length     = DY*GRID;
	header.z_length     = DZ*GRID;
	header.nxstart      = DX/-2;
	header.nystart      = DY/-2;
	header.nzstart      = DZ/-2;
	header.mapc         = 1;
	header.mapr         = 2;
	header.maps         = 3;
	header.alpha        = 90;
	header.beta         = 90;
	header.gamma        = 90;
	header.amean        = 0;
	header.amax         = 0;
	header.amin         = 0;
	header.rms          = 0;
	header.mach         = time(NULL);
	// "MAP " from ascii to little-endian integer:  
	//   ord('M')=77, ord('A')=65, ord('P')=80, ord(' ')=32
	//   reverse order and powers of 256
	//   32*(256**3) + 80*(256**2) + 65*(256) + 77
	header.map          = 542130509;
	header.xorigin      = float(XMIN);
	header.yorigin      = float(YMIN);
	header.zorigin      = float(ZMIN);
	header.ispg         = 0;
	header.nsymbt       = 0;
	header.nlabl        = 0;
	header.mode         = MRC_MODE_BYTE;
	
	if ( header.nz == 0 ) header.nz = 1;

	if (DEBUG > 0) {
      cerr << "Standard MRC write" << endl;
		cerr << "N.START: " << header.nxstart << " , " << header.nystart << " , " << header.nzstart << endl;
		cerr << "ORIGIN: " << header.xorigin << " , " << header.yorigin << " , " << header.zorigin << endl;
	}

	int k;
	for(k=0;k<MRC_USERS;k++) header.extra[k] = 0;
	for(k=0;k<MRC_NUM_LABELS;k++) header.label[k][0] = '\0';
	
	if ( writeMRCHeader(fp,header) == -1 ) return -1;

	int count = byteWrite( fp, data, NUMBINS, 1 );
	
	if ( count != 17 ) return -1;
	
	fclose(fp);

	return 1;

}

/*********************************************/
int ijk2pt2(const int i, const int j, const int k, const int xdim, const int ydim) {
  return int(i+j*xdim+k*xdim*ydim);
};


/*********************************************/
int writeSmallMRCFile( gridpt data[], char filename[]) {
	int volume = countGrid(data);
	if (volume == 0) {
		cerr << "volume is empty not writing mrc file" << endl;
		return 0;
	}
	cerr << "Volume: " << volume*GRIDVOL << " Angstroms" << endl;
	cerr << "Writing trimmed grid to MRC file: " << filename << endl << endl;

//PART I: Determine Extrema
	int minmax[6];
	int xmin, ymin, zmin;
	int xmax, ymax, zmax;
	determine_MinMax(data, minmax);
	xmin = minmax[0]-1;
	ymin = minmax[1]/DX-1;
	zmin = minmax[2]/DXY-1;
	xmax = minmax[3]+1;
	ymax = minmax[4]/DX+1;
	zmax = minmax[5]/DXY+1;


//PART II: Calculate new dimensions
	int xdim, ydim, zdim;
	xdim = int((xmax-xmin)/4.0 +1.0)*4;
	ydim = int((ymax-ymin)/4.0 +1.0)*4;
	zdim = int((zmax-zmin)/4.0 +1.0)*4;
	if (DEBUG > 0) {
		cerr << "Minima: " << xmin << " , " << ymin << " , " << zmin << endl;
		cerr << "Maxima: " << xmax << " , " << ymax << " , " << zmax << endl;
		cerr << "Old dimensions: " << DX << " , " << DY << " , " << DZ << endl;
		cerr << "New dimensions: " << xdim << " , " << ydim << " , " << zdim << endl;
		cerr << "PDB Minima: " << (XMIN/GRID)+xmin << " , " << (YMIN/GRID)+ymin 
			<< " , " << (ZMIN/GRID)+zmin << endl;
		cerr << "PDB Maxima: " << (XMIN/GRID)+xmax << " , " << (YMIN/GRID)+ymax 
			<< " , " << (ZMIN/GRID)+zmax << endl;
		cerr << "Center: " << XMIN/GRID+(xmin+xmax)/2 << " , " << YMIN/GRID+(ymin+ymax)/2 
			<< " , " << ZMIN/GRID+(zmin+zmax)/2 << endl;
	}


//PART III: Trim the grid
	int oldpt, newpt;
	unsigned int numbins = xdim*ydim*zdim;
	gridpt *smdata;
	if (DEBUG > 0)
		cerr << NUMBINS << " --> " << numbins << endl;
	smdata = (gridpt*) malloc (numbins);
	#pragma omp parallel for
	for(unsigned int pt=0; pt<numbins; pt++) {
		smdata[pt] = 0;
	}
	if (DEBUG > 0)
	  cerr << "Trimming the grid..." << endl;
	for(int k=0; k<DZ; k++) {
		for(int j=0; j<DY; j++) {
			for(int i=0; i<DX; i++) {
				oldpt = ijk2pt2(i, j, k, DX, DY);
				//cerr << oldpt << endl;
				if(data[oldpt]) {
					newpt = ijk2pt2(i-xmin, j-ymin, k-zmin, xdim, ydim);
					//cerr << oldpt << " --> " << newpt << endl;
					smdata[newpt] = 1;
				}
	} } }
	if (DEBUG > 0)
	  cerr << endl << "DONE";

//PART IV: Use new dimensions
	FILE * fp = fopen(filename, "wb");
	if ( fp == NULL ) return -1;
	
	MRCHeaderSt header;
	//cerr << XMIN << "\t" << YMIN << "\t" << ZMIN << endl;
	//cerr << XMAX << "\t" << YMAX << "\t" << ZMAX << endl;
	//cerr << xdim << "\t" << ydim << "\t" << zdim << endl;
	//cerr << (XMAX-XMIN)/2.0 << "\t" << (YMAX-YMIN)/2.0 << "\t" << (ZMAX-ZMIN)/2.0 << endl;
	//cerr << (xdim-1.0)/2.0*GRID+XMIN << "\t" << (ydim-1.0)/2.0*GRID+YMIN << "\t" << (zdim-1.0)/2.0*GRID+ZMIN << endl;
	header.nx           = xdim;
	header.ny           = ydim;
	header.nz           = zdim;
	header.mx           = xdim;
	header.my           = ydim;
	header.mz           = zdim;
	header.x_length     = xdim*GRID;
	header.y_length     = ydim*GRID;
	header.z_length     = zdim*GRID;
	header.nxstart      = DX/-2;
	header.nystart      = DY/-2;
	header.nzstart      = DZ/-2;
	header.mapc         = 1;
	header.mapr         = 2;
	header.maps         = 3;
	header.alpha        = 90;
	header.beta         = 90;
	header.gamma        = 90;
	header.amean        = 0;
	header.amax         = 0;
	header.amin         = 0;
	header.rms          = 0;
	header.mach         = time(NULL);
	// "MAP " from ascii to little-endian integer:  
	//   ord('M')=77, ord('A')=65, ord('P')=80, ord(' ')=32
	//   reverse order and powers of 256
	//   32*(256**3) + 80*(256**2) + 65*(256) + 77
	header.map          = 542130509;
	header.xorigin      = XMIN+GRID*xmin; //check this
	header.yorigin      = YMIN+GRID*ymin;
	header.zorigin      = ZMIN+GRID*zmin;
	header.ispg         = 0;
	header.nsymbt       = 0;
	header.nlabl        = 0;
	header.mode         = MRC_MODE_BYTE;

	if (DEBUG > 0) {
      cerr << "Trimmed MRC write" << endl;
		cerr << "N.START: " << header.nxstart << " , " << header.nystart << " , " << header.nzstart << endl;
      cerr << "MINS:   " << xmin << " , " << ymin << " , " << zmin;
		cerr << "ORIGIN: " << header.xorigin << " , " << header.yorigin << " , " << header.zorigin << endl;
	}

	if ( header.nz == 0 ) header.nz = 1;

	int k;
	for(k=0;k<MRC_USERS;k++) header.extra[k] = 0;
	for(k=0;k<MRC_NUM_LABELS;k++) header.label[k][0] = '\0';
	
	if ( writeMRCHeader(fp,header) == -1 ) return -1;


	int count = byteWrite( fp, smdata, numbins, 1 );
	free (smdata);
	if ( count != 17 ) return -1;
	
	fclose(fp);

	return 1;

}

