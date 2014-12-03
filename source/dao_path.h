/*
// Dao Graphics Engine
// http://www.daovm.net
//
// Copyright (c) 2012-2014, Limin Fu
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
// OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
// SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
// OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#ifndef __DAO_PATH_H__
#define __DAO_PATH_H__


#include "dao_common.h"
#include "dao_triangulator.h"

#define DAOX_MAX_DASH    8
#define DAOX_PATH_UNIT   1024.0
#define DAOX_RESOLUTION  1024.0

typedef struct DaoxPathSegment    DaoxPathSegment;
typedef struct DaoxPathComponent  DaoxPathComponent;
typedef struct DaoxPath           DaoxPath;

typedef struct DaoxBezierPoint    DaoxBezierPoint;
typedef struct DaoxPathStyle      DaoxPathStyle;
typedef struct DaoxPathMesh       DaoxPathMesh;
typedef struct DaoxPathCache      DaoxPathCache;


enum DaoxPathCommandModes
{
	DAOX_PATH_CMD_ABS ,
	DAOX_PATH_CMD_REL
};
enum DaoxPathJunctions
{
	DAOX_JUNCTION_NONE ,
	DAOX_JUNCTION_FLAT ,
	DAOX_JUNCTION_SHARP ,
	DAOX_JUNCTION_ROUND
};
enum DaoxLineCaps
{
	DAOX_LINECAP_NONE ,
	DAOX_LINECAP_FLAT ,
	DAOX_LINECAP_SHARP ,
	DAOX_LINECAP_ROUND
};


struct DaoxPathSegment
{
	char  bezier;      /* 0: open; 1: linear; 2: quadratic; 3: cubic; */
	char  convexness;  /* 0: flat; 1: locally convex; -1: locally concave; */
	char  refined;
	char  subStart : 4;
	char  subEnd   : 4;

	DaoxVector2D  P1; /* start point; */
	DaoxVector2D  P2; /* end point; */
	DaoxVector2D  C1; /* first control point; */
	DaoxVector2D  C2; /* second control point; */

	DaoxPathSegment  *first;   /* first subdivided segment; */
	DaoxPathSegment  *second;  /* second subdivided segment; */
	DaoxPathSegment  *next;    /* next segment in the path; */
};

struct DaoxPathComponent
{
	DaoxPath           *path;
	DaoxPathSegment    *first;
	DaoxPathSegment    *last;
	DaoxPathSegment    *refinedFirst;
	DaoxPathSegment    *refinedLast;
	DaoxPathComponent  *next;
};

struct DaoxPath
{
	DAO_CSTRUCT_COMMON;

	uchar_t      mode;
	uchar_t      cached;
	uchar_t      hashed;
	uint_t       hash;
	float        length;
	DaoxOBBox2D  obbox;

	DaoxPathComponent  *first;
	DaoxPathComponent  *last;

	DaoxPathComponent  *freeComponents;
	DaoxPathSegment    *freeSegments;
};

extern DaoType* daox_type_path;

DaoxPathSegment* DaoxPathSegment_New();
void DaoxPathSegment_Delete( DaoxPathSegment *self );
double DaoxPathSegment_Length( DaoxPathSegment *self, float factor );

DaoxPath* DaoxPath_New();
void DaoxPath_Delete( DaoxPath *self );
void DaoxPath_Reset( DaoxPath *self );
void DaoxPath_Copy( DaoxPath *self, DaoxPath *other );

uint_t DaoxPath_Hash( DaoxPath *self );

DAO_DLL void DaoxPath_SetRelativeMode( DaoxPath *self, int relative );
DAO_DLL void DaoxPath_MoveTo( DaoxPath *self, float x, float y );
DAO_DLL void DaoxPath_Close( DaoxPath *self );
DAO_DLL void DaoxPath_LineTo( DaoxPath *self, float x, float y );
DAO_DLL void DaoxPath_QuadTo( DaoxPath *self, float cx, float cy, float x, float y );
DAO_DLL void DaoxPath_CubicTo( DaoxPath *self, float cx, float cy, float x, float y );
DAO_DLL void DaoxPath_CubicTo2( DaoxPath *self, float cx1, float cy1, float cx2, float cy2, float x2, float y2 );
DAO_DLL void DaoxPath_ArcTo( DaoxPath *self, float x, float y, float degrees );
DAO_DLL void DaoxPath_ArcTo2( DaoxPath *self, float x, float y, float degrees, float deg2 );
DAO_DLL void DaoxPath_ArcBy( DaoxPath *self, float cx, float cy, float degrees );

void DaoxPath_ImportPath( DaoxPath *self, DaoxPath *path, DaoxMatrix3D *transform );

void DaoxPath_Refine( DaoxPath *self, float maxlen, float maxdiff );

DaoxPathSegment* DaoxPath_LocateByDistance( DaoxPath *self, float dist, DaoxVector3D *pos );

void DaoxPathSegment_Divide( DaoxPathSegment *self, float at );




/*
// Point with texture coordinates for rendering cubic bezier curves
// using the procedural texturing technique as proposed by Loop and Blinn.
//
// Loop, Charles, and Jim Blinn:
// GPU Gems 3: Chapter 25. Rendering Vector Art on the GPU.
// http://http.developer.nvidia.com/GPUGems3/gpugems3_ch25.html
//
// Loop, Charles, and Jim Blinn. 2005:
// Resolution Independent Curve Rendering using Programmable Graphics Hardware.
// In ACM Transactions on Graphics (Proceedings of SIGGRAPH 2005) 24(3), pp. 1000–1008.
//
*/
struct DaoxBezierPoint
{
	DaoxVector2D  pos;
	DaoxVector3D  klm;
	float         offset;
};



struct DaoxPathStyle
{
	uchar_t  cap;
	uchar_t  dash;
	uchar_t  junction;
	float    width;
	float    dashes[DAOX_MAX_DASH];
};

void DaoxPathStyle_Init( DaoxPathStyle *self );
void DaoxPathStyle_SetDashes( DaoxPathStyle *self, int count, float lens[] );



/*
// Tessellation data for a path with given stroke style:
*/
struct DaoxPathMesh
{
	DAO_CSTRUCT_COMMON;
	
	uint_t   hash;

	DaoxPathStyle  strokeStyle;

	DaoxPath  *path;

	DArray  *workPoints;
	DArray  *workTriangles;
	DArray  *workBeziers;

	DArray  *fillPoints;
	DArray  *fillTriangles;
	DArray  *fillBeziers;

	DArray  *strokePoints;
	DArray  *strokeTriangles;
	DArray  *strokeBeziers;
};

DaoxPathMesh* DaoxPathMesh_New();
void DaoxPathMesh_Delete( DaoxPathMesh *self );

void DaoxPathMesh_Reset( DaoxPathMesh *self, DaoxPath *path, DaoxPathStyle *style );
void DaoxPathMesh_Preprocess( DaoxPathMesh *self, DaoxTriangulator *triangulator );
void DaoxMeshPath_ComputeStroke( DaoxPathMesh *self );



struct DaoxPathCache
{
	DAO_CSTRUCT_COMMON;

	DaoxPathCache  *main;

	DMap      *paths;
	DMap      *meshes;

	DaoxPath  *unitLine;
	DaoxPath  *unitRect;
	DaoxPath  *unitCircle1;
	DaoxPath  *unitCircle2;
	DaoxPath  *unitCircle3;

	DaoxTriangulator  *triangulator;

	int  pathCount, meshCount;
};

DaoxPathCache* DaoxPathCache_New();
void DaoxPathCache_Delete( DaoxPathCache *self );

DaoxPath* DaoxPathCache_FindPath( DaoxPathCache *self, DaoxPath *path );
DaoxPathMesh* DaoxPathCache_FindMesh( DaoxPathCache *self, DaoxPath *path, DaoxPathStyle *style );

#endif
