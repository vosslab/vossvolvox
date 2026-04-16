/*
** utils-ccp4.cpp
** CCP4 volume writer: placement via NSTART grid indices, ORIGIN zeroed.
** Compatibility export for viewers that expect CCP4/NSTART semantics
** (e.g. PyMOL format=ccp4). Use .ccp4 or .map extension.
*/
#include <cstdio>      // for fclose, fopen, snprintf
#include <cstdlib>     // for std::free, std::malloc
#include <cstring>     // for memset
#include <iostream>    // for cerr, endl
#include "utils.hpp"     // for cerr, endl, DEBUG, gridpt, countGrid
#include "utils-mrc-header.hpp"  // for MRCHeaderSt, byteWrite, writeMRCHeader

//FROM utils.h external variables
extern float XMIN, YMIN, ZMIN;
extern int DX, DY, DZ;
extern unsigned int NUMBINS;
extern float GRID;

// ijk2pt2 is also defined in utils-mrc.cpp; local copy for CCP4 trimming
/*********************************************/
static int ijk2pt2(const int i, const int j, const int k, const int xdim, const int ydim) {
	return int(i+j*xdim+k*xdim*ydim);
}

/*********************************************/
int writeCCP4File(const gridpt data[], const char filename[]) {
	int volume = countGrid(data);
	if (volume == 0) {
		cerr << "volume is empty not writing ccp4 file" << endl;
		return 0;
	}
	cerr << "CCP4 dims: " << DX << " x " << DY << " x " << DZ << endl;
	cerr << "writing complete grid to CCP4 file: " << filename << endl;

	FILE * fp = fopen(filename, "wb");
	if ( fp == NULL ) return -1;

	MRCHeaderSt header;
	header.nx           = DX;
	header.ny           = DY;
	header.nz           = DZ;
	header.mx           = DX;
	header.my           = DY;
	header.mz           = DZ;
	header.x_length     = DX*GRID;
	header.y_length     = DY*GRID;
	header.z_length     = DZ*GRID;
	// CCP4: placement via NSTART; ORIGIN zeroed
	// grid bounds are snapped to multiples of 4*GRID, so XMIN/GRID is exact
	header.nxstart      = int(XMIN/GRID);
	header.nystart      = int(YMIN/GRID);
	header.nzstart      = int(ZMIN/GRID);
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
	// MRC2014 machine stamp: 0x44 0x44 0x00 0x00 for little-endian
	header.mach         = 0x00004144;
	// "MAP " identifier
	header.map          = 542130509;
	// CCP4: ORIGIN zeroed; placement is in NSTART fields
	header.xorigin      = 0;
	header.yorigin      = 0;
	header.zorigin      = 0;
	header.ispg         = 1;  // single volume
	header.nsymbt       = 0;
	header.mode         = MRC_MODE_BYTE;

	if ( header.nz == 0 ) header.nz = 1;

	if (DEBUG > 0) {
		cerr << "Standard CCP4 write" << endl;
		cerr << "N.START: " << header.nxstart << " , " << header.nystart << " , " << header.nzstart << endl;
		cerr << "ORIGIN: " << header.xorigin << " , " << header.yorigin << " , " << header.zorigin << endl;
	}

	int k;
	for(k=0;k<MRC_USERS;k++) header.extra[k] = 0;
	// NVERSION at word 28 = extra[3]: MRC2014 format
	header.extra[3]     = 20140;
	// label describing placement convention
	memset(header.label, 0, MRC_NUM_LABELS * MRC_LABEL_SIZE);
	snprintf(reinterpret_cast<char*>(header.label[0]), MRC_LABEL_SIZE,
		"CCP4: NSTART used for placement; ORIGIN zeroed");
	header.nlabl        = 1;

	if ( writeMRCHeader(fp,header) == -1 ) return -1;

	int count = byteWrite( fp, data, NUMBINS, 1 );

	if ( count != 17 ) return -1;

	fclose(fp);

	return 1;
}

/*********************************************/
int writeSmallCCP4File(const gridpt data[], const char filename[]) {
	int volume = countGrid(data);
	if (volume == 0) {
		cerr << "volume is empty not writing ccp4 file" << endl;
		return 0;
	}
	cerr << "Volume: " << volume*GRIDVOL << " Angstroms" << endl;
	cerr << "Writing trimmed grid to CCP4 file: " << filename << endl << endl;

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
	}

//PART III: Trim the grid
	int oldpt, newpt;
	unsigned int numbins = xdim*ydim*zdim;
	gridpt *smdata;
	if (DEBUG > 0)
		cerr << NUMBINS << " --> " << numbins << endl;
	smdata = (gridpt*) std::malloc (numbins);
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
				if(data[oldpt]) {
					newpt = ijk2pt2(i-xmin, j-ymin, k-zmin, xdim, ydim);
					smdata[newpt] = 1;
				}
	} } }
	if (DEBUG > 0)
		cerr << endl << "DONE";

//PART IV: Use new dimensions
	FILE * fp = fopen(filename, "wb");
	if ( fp == NULL ) return -1;

	MRCHeaderSt header;
	header.nx           = xdim;
	header.ny           = ydim;
	header.nz           = zdim;
	header.mx           = xdim;
	header.my           = ydim;
	header.mz           = zdim;
	header.x_length     = xdim*GRID;
	header.y_length     = ydim*GRID;
	header.z_length     = zdim*GRID;
	// CCP4: placement via NSTART; ORIGIN zeroed
	// grid bounds are snapped to multiples of 4*GRID, so division is exact
	header.nxstart      = int((XMIN+GRID*xmin)/GRID);
	header.nystart      = int((YMIN+GRID*ymin)/GRID);
	header.nzstart      = int((ZMIN+GRID*zmin)/GRID);
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
	// MRC2014 machine stamp: 0x44 0x44 0x00 0x00 for little-endian
	header.mach         = 0x00004144;
	header.map          = 542130509;
	// CCP4: ORIGIN zeroed; placement is in NSTART fields
	header.xorigin      = 0;
	header.yorigin      = 0;
	header.zorigin      = 0;
	header.ispg         = 1;  // single volume
	header.nsymbt       = 0;
	header.mode         = MRC_MODE_BYTE;

	if (DEBUG > 0) {
		cerr << "Trimmed CCP4 write" << endl;
		cerr << "N.START: " << header.nxstart << " , " << header.nystart << " , " << header.nzstart << endl;
		cerr << "ORIGIN: " << header.xorigin << " , " << header.yorigin << " , " << header.zorigin << endl;
	}

	if ( header.nz == 0 ) header.nz = 1;

	int k;
	for(k=0;k<MRC_USERS;k++) header.extra[k] = 0;
	// NVERSION at word 28 = extra[3]: MRC2014 format
	header.extra[3]     = 20140;
	// label describing placement convention
	memset(header.label, 0, MRC_NUM_LABELS * MRC_LABEL_SIZE);
	snprintf(reinterpret_cast<char*>(header.label[0]), MRC_LABEL_SIZE,
		"CCP4: NSTART used for placement; ORIGIN zeroed");
	header.nlabl        = 1;

	if ( writeMRCHeader(fp,header) == -1 ) return -1;

	int count = byteWrite( fp, smdata, numbins, 1 );
	std::free (smdata);
	if ( count != 17 ) return -1;

	fclose(fp);

	return 1;
}
