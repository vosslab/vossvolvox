/*
** utils-mrc-header.hpp
** Shared MRC/CCP4 header struct, constants, and write helpers.
*/
#pragma once

#include <cstdio>      // for FILE, fwrite
#include <cstdint>     // for int8_t, int32_t, uint8_t, uint16_t

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

// write helpers shared between MRC and CCP4 writers
int byteWrite(FILE* fp, const void* data, int number_of_elements, int element_size);
int writeMRCHeader(FILE* fp, MRCHeaderSt header);
