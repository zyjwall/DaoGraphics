/*
// Dao Graphics Engine
// http://www.daovm.net
//
// Copyright (c) 2012,2013, Limin Fu
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


#ifndef __DAO_IMAGE_H__
#define __DAO_IMAGE_H__

#include "daoStdtype.h"


typedef struct DaoxImage  DaoxImage;


/*
// Image depths including the alpha channel:
*/
enum DaoxImageDepth
{
	DAOX_IMAGE_BIT8 ,
	DAOX_IMAGE_BIT16 ,
	DAOX_IMAGE_BIT24 ,
	DAOX_IMAGE_BIT32
};


/*
// DaoxImage supports only RGBA with different depth.
// Each channel is encoded in the same number of bits.
*/
struct DaoxImage
{
	DAO_CSTRUCT_COMMON;

	int       depth;
	int       width;
	int       height;

	int       widthStep;
	int       imageSize;
	uchar_t  *imageData;
};
DAO_DLL DaoType *daox_type_image;


DaoxImage* DaoxImage_New();
void DaoxImage_Delete( DaoxImage *self );

void DaoxImage_Resize( DaoxImage *self, int width, int height );


int DaoxImage_LoadBMP( DaoxImage *self, const char *file );
int DaoxImage_SaveBMP( DaoxImage *self, const char *file );

int DaoxImage_LoadPNG( DaoxImage *self, const char *file );
int DaoxImage_SavePNG( DaoxImage *self, const char *file );

#endif
