/*
// Dao Graphics Engine
// http://www.daovm.net
//
// Copyright (c) 2012-2016, Limin Fu
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

#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "dao_opengl.h"
#include "dao_particle.h"
#include "dao_scene.h"






DaoxTexture* DaoxTexture_New()
{
	DaoxTexture *self = (DaoxTexture*) dao_calloc( 1, sizeof(DaoxTexture) );
	DaoCstruct_Init( (DaoCstruct*) self, daox_type_texture );
	return self;
}
void DaoxTexture_Delete( DaoxTexture *self )
{
	DaoCstruct_Free( (DaoCstruct*) self );
	DaoGC_DecRC( (DaoValue*) self->image );
	dao_free( self );
}
void DaoxTexture_SetImage( DaoxTexture *self, DaoImage *image )
{
	self->changed = 1;
	GC_Assign( & self->image, image );
}
void DaoxTexture_LoadImage( DaoxTexture *self, const char *file )
{
	DaoImage *image = self->image;
	int ok = 0;
	self->changed = 1;
	if( image == NULL || image->refCount > 1 ){
		image = _DaoImage_New( _DaoImage_Type( DaoType_GetVmSpace( self->ctype ) ) );
		DaoxTexture_SetImage( self, image );
	}
	if( ok == 0 ) ok = _DaoImage_LoadPNG( self->image, file );
	if( ok == 0 ) ok = _DaoImage_LoadBMP( self->image, file );
}





DaoxMaterial* DaoxMaterial_New()
{
	DaoxMaterial *self = (DaoxMaterial*) dao_calloc( 1, sizeof(DaoxMaterial) );
	DaoCstruct_Init( (DaoCstruct*) self, daox_type_material );
	self->shininess = 2.0;
	self->ambient = daox_black_color;
	self->diffuse = daox_black_color;
	self->specular = daox_black_color;
	self->emission = daox_black_color;
	self->name = DString_New();
	return self;
}
void DaoxMaterial_Delete( DaoxMaterial *self )
{
	DString_Delete( self->name );
	DaoGC_DecRC( (DaoValue*) self->diffuseTexture );
	DaoGC_DecRC( (DaoValue*) self->emissionTexture );
	DaoGC_DecRC( (DaoValue*) self->bumpTexture );
	DaoCstruct_Free( (DaoCstruct*) self );
	dao_free( self );
}
void DaoxMaterial_CopyFrom( DaoxMaterial *self, DaoxMaterial *other )
{
	/* Do not copy name! */
	memcpy( & self->ambient, & other->ambient, 4*sizeof(DaoxColor) );
	memcpy( & self->lighting, & other->lighting, 6*sizeof(uint_t) );
	GC_Assign( & self->diffuseTexture, other->diffuseTexture );
	GC_Assign( & self->emissionTexture, other->emissionTexture );
	GC_Assign( & self->bumpTexture, other->bumpTexture );
}
void DaoxMaterial_SetTexture( DaoxMaterial *self, DaoxTexture *texture, int which )
{
	switch( which ){
	case DAOX_DIFFUSE_TEXTURE : GC_Assign( & self->diffuseTexture, texture ); break;
	case DAOX_EMISSION_TEXTURE : GC_Assign( & self->emissionTexture, texture ); break;
	case DAOX_BUMP_TEXTURE : GC_Assign( & self->bumpTexture, texture ); break;
	}
}





void DaoxViewFrustum_Normalize( DaoxViewFrustum *self )
{
	DaoxVector3D vector;

	self->viewDirection = DaoxVector3D_Normalize( & self->viewDirection ); 
	self->topLeftEdge = DaoxVector3D_Normalize( & self->topLeftEdge ); 
	self->topRightEdge = DaoxVector3D_Normalize( & self->topRightEdge ); 
	self->bottomLeftEdge = DaoxVector3D_Normalize( & self->bottomLeftEdge ); 
	self->bottomRightEdge = DaoxVector3D_Normalize( & self->bottomRightEdge ); 
	self->leftPlaneNorm = DaoxVector3D_Normalize( & self->leftPlaneNorm ); 
	self->rightPlaneNorm = DaoxVector3D_Normalize( & self->rightPlaneNorm ); 
	self->topPlaneNorm = DaoxVector3D_Normalize( & self->topPlaneNorm ); 
	self->bottomPlaneNorm = DaoxVector3D_Normalize( & self->bottomPlaneNorm ); 

	vector = DaoxVector3D_Scale( & self->viewDirection, self->near + 0.11 );
	self->axisOrigin = DaoxVector3D_Add( & self->cameraPosition, & vector );
}
/*
// View direction: -z;
// Up direction:    y;
// Right direction: x;
*/
void DaoxViewFrustum_Init( DaoxViewFrustum *self, DaoxCamera *camera )
{
	DaoxVector3D cameraPosition = {0.0, 0.0, 0.0};
	DaoxVector3D viewDirection = {0.0, 0.0, -1.0};
	DaoxVector3D nearViewCenter = {0.0, 0.0, 0.0};
	DaoxVector3D farViewCenter = {0.0, 0.0, 0.0};
	DaoxVector3D topLeft = cameraPosition;
	DaoxVector3D topRight = cameraPosition;
	DaoxVector3D bottomLeft = cameraPosition;
	DaoxVector3D bottomRight = cameraPosition;
	DaoxVector3D leftPlaneNorm;
	DaoxVector3D rightPlaneNorm;
	DaoxVector3D topPlaneNorm;
	DaoxVector3D bottomPlaneNorm;
	DaoxMatrix4D objectToWorld = DaoxSceneNode_GetWorldTransform( & camera->base );
	float xtan = tan( 0.5 * camera->fovAngle * M_PI / 180.0 );

	self->right = camera->nearPlane * xtan;
	self->left = - self->right;
	self->top = self->right / camera->aspectRatio;
	self->bottom = - self->top;
	self->near = camera->nearPlane;
	self->far = camera->farPlane;

	topLeft.x = self->left;
	topLeft.y = self->top;
	topLeft.z = - camera->nearPlane;

	topRight.x = self->right;
	topRight.y = self->top;
	topRight.z = - camera->nearPlane;

	bottomLeft.x = self->left;
	bottomLeft.y = self->bottom;
	bottomLeft.z = - camera->nearPlane;

	bottomRight.x = self->right;
	bottomRight.y = self->bottom;
	bottomRight.z = - camera->nearPlane;

	nearViewCenter.z = - camera->nearPlane;
	farViewCenter.z = - camera->farPlane;

	leftPlaneNorm = DaoxVector3D_Cross( & topLeft, & bottomLeft );
	rightPlaneNorm = DaoxVector3D_Cross( & bottomRight, & topRight );
	topPlaneNorm = DaoxVector3D_Cross( & topRight, & topLeft );
	bottomPlaneNorm = DaoxVector3D_Cross( & bottomLeft, & bottomRight );

	self->cameraPosition = DaoxMatrix4D_MulVector( & objectToWorld, & cameraPosition, 1.0 );
	self->nearViewCenter = DaoxMatrix4D_MulVector( & objectToWorld, & nearViewCenter, 1.0 );
	self->farViewCenter = DaoxMatrix4D_MulVector( & objectToWorld, & farViewCenter, 1.0 );
	self->viewDirection = DaoxMatrix4D_MulVector( & objectToWorld, & viewDirection, 0.0 );
	self->topLeftEdge = DaoxMatrix4D_MulVector( & objectToWorld, & topLeft, 0.0 );
	self->topRightEdge = DaoxMatrix4D_MulVector( & objectToWorld, & topRight, 0.0 );
	self->bottomLeftEdge = DaoxMatrix4D_MulVector( & objectToWorld, & bottomLeft, 0.0 );
	self->bottomRightEdge = DaoxMatrix4D_MulVector( & objectToWorld, & bottomRight, 0.0 );
	self->leftPlaneNorm = DaoxMatrix4D_MulVector( & objectToWorld, & leftPlaneNorm, 0.0 );
	self->rightPlaneNorm = DaoxMatrix4D_MulVector( & objectToWorld, & rightPlaneNorm, 0.0 );
	self->topPlaneNorm = DaoxMatrix4D_MulVector( & objectToWorld, & topPlaneNorm, 0.0 );
	self->bottomPlaneNorm = DaoxMatrix4D_MulVector( & objectToWorld, & bottomPlaneNorm, 0.0 );
	DaoxViewFrustum_Normalize( self );
}
DaoxViewFrustum DaoxViewFrustum_Transform( DaoxViewFrustum *self, DaoxMatrix4D *matrix )
{
	DaoxViewFrustum frustum = *self;
	frustum.cameraPosition = DaoxMatrix4D_MulVector( matrix, & self->cameraPosition, 1.0 );
	frustum.nearViewCenter = DaoxMatrix4D_MulVector( matrix, & self->nearViewCenter, 1.0 );
	frustum.farViewCenter = DaoxMatrix4D_MulVector( matrix, & self->farViewCenter, 1.0 );
	frustum.viewDirection = DaoxMatrix4D_MulVector( matrix, & self->viewDirection, 0.0 );
	frustum.topLeftEdge = DaoxMatrix4D_MulVector( matrix, & self->topLeftEdge, 0.0 );
	frustum.topRightEdge = DaoxMatrix4D_MulVector( matrix, & self->topRightEdge, 0.0 );
	frustum.bottomLeftEdge = DaoxMatrix4D_MulVector( matrix, & self->bottomLeftEdge, 0.0 );
	frustum.bottomRightEdge = DaoxMatrix4D_MulVector( matrix, & self->bottomRightEdge, 0.0 );
	frustum.leftPlaneNorm = DaoxMatrix4D_MulVector( matrix, & self->leftPlaneNorm, 0.0 );
	frustum.rightPlaneNorm = DaoxMatrix4D_MulVector( matrix, & self->rightPlaneNorm, 0.0 );
	frustum.topPlaneNorm = DaoxMatrix4D_MulVector( matrix, & self->topPlaneNorm, 0.0 );
	frustum.bottomPlaneNorm = DaoxMatrix4D_MulVector( matrix, & self->bottomPlaneNorm, 0.0 );
	DaoxViewFrustum_Normalize( & frustum );
	return frustum;
}
double DaoxViewFrustum_Difference( DaoxViewFrustum *self, DaoxViewFrustum *other )
{
	double d1, d2, d3, d4, d5, max = 0.0;
	d1 = DaoxVector3D_Difference( & self->cameraPosition, & other->cameraPosition);
	d2 = DaoxVector3D_Difference( & self->leftPlaneNorm, & other->leftPlaneNorm);
	d3 = DaoxVector3D_Difference( & self->rightPlaneNorm, & other->rightPlaneNorm);
	d4 = DaoxVector3D_Difference( & self->topPlaneNorm, & other->topPlaneNorm);
	d5 = DaoxVector3D_Difference( & self->bottomPlaneNorm, & other->bottomPlaneNorm);
	if( d1 > max ) max = d1;
	if( d2 > max ) max = d2;
	if( d3 > max ) max = d3;
	if( d4 > max ) max = d4;
	if( d5 > max ) max = d5;
	return max;
}
void DaoxViewFrustum_Print( DaoxViewFrustum *self )
{
	printf( "DaoxViewFrustum:\n" );
	DaoxVector3D_Print( & self->cameraPosition );
	DaoxVector3D_Print( & self->leftPlaneNorm );
	DaoxVector3D_Print( & self->rightPlaneNorm );
	DaoxVector3D_Print( & self->topPlaneNorm );
	DaoxVector3D_Print( & self->bottomPlaneNorm );
}


/*
// This function take a plane (passing "point" with normal "norm"),
// and a line segment (connecting P1 and P2) as parameter. It returns:
// -1, if the line segment P1P2 lies in the negative side of the plane;
// 0,  if the line segment cross the plane;
// 1,  if the line segment lies in the positive side of the plane;
*/
static int CheckLine( DaoxVector3D point, DaoxVector3D norm, DaoxVector3D P1, DaoxVector3D P2 )
{
	double dot1, dot2;
	P1 = DaoxVector3D_Sub( & P1, & point );
	P2 = DaoxVector3D_Sub( & P2, & point );
	dot1 = DaoxVector3D_Dot( & P1, & norm );
	dot2 = DaoxVector3D_Dot( & P2, & norm );
	if( dot1 * dot2 <= EPSILON ) return 0;
	if( dot1 < 0.0 ) return -1;
	return 1;
}
static int CheckBox( DaoxVector3D point, DaoxVector3D norm, DaoxOBBox3D *box )
{
	DaoxVector3D dX, dY, dZ, XY, YZ, ZX, XYZ;
	int C1, C2, C3, C4;

	dX = DaoxVector3D_Sub( & box->X, & box->O );
	dY = DaoxVector3D_Sub( & box->Y, & box->O );
	dZ = DaoxVector3D_Sub( & box->Z, & box->O );

	XY = DaoxVector3D_Add( & box->X, & dY );
	C1 = CheckLine( point, norm, box->Z, XY );
	if( C1 == 0 ) return 0;

	YZ = DaoxVector3D_Add( & box->Y, & dZ );
	C2 = CheckLine( point, norm, box->X, YZ );
	if( C2 == 0 ) return 0;

	ZX = DaoxVector3D_Add( & box->Z, & dX );
	C3 = CheckLine( point, norm, box->Y, ZX );
	if( C3 == 0 ) return 0;

	XYZ = DaoxVector3D_Add( & ZX, & dY );
	C4 = CheckLine( point, norm, box->O, XYZ );

	return C4;
}
int  DaoxViewFrustum_SphereCheck( DaoxViewFrustum *self, DaoxOBBox3D *box )
{
	DaoxVector3D C = DaoxVector3D_Sub( & box->C, & self->cameraPosition );
	double D0 = DaoxVector3D_Dot( & C, & self->viewDirection );
	double D1, D2, D3, D4, margin = box->R + EPSILON;
	if( D0 > (self->far + margin) ) return -1;
	if( D0 < (self->near - margin) ) return -1;
	if( (D1 = DaoxVector3D_Dot( & C, & self->leftPlaneNorm )) > margin ) return -1;
	if( (D2 = DaoxVector3D_Dot( & C, & self->rightPlaneNorm )) > margin ) return -1;
	if( (D3 = DaoxVector3D_Dot( & C, & self->topPlaneNorm )) > margin ) return -1;
	if( (D4 = DaoxVector3D_Dot( & C, & self->bottomPlaneNorm )) > margin ) return -1;
	if( D0 > (self->near + margin) && D0 < (self->far - margin) && D1 < -margin && D2 < -margin && D3 < -margin && D4 < -margin ) return 1;
	return 0;
}
int  DaoxViewFrustum_Visible( DaoxViewFrustum *self, DaoxOBBox3D *box )
{
	int C1, C2, C3, C4, C5, C6;
	int C0 = DaoxViewFrustum_SphereCheck( self, box );
	if( C0 != 0 ) return C0;

	C1 = C2 = C3 = C4 = C5 = C6 = 0;
	if( (C1 = CheckBox( self->nearViewCenter, self->viewDirection, box )) < 0 ) return -1;
	if( (C2 = CheckBox( self->farViewCenter, self->viewDirection, box )) > 0 ) return -1;
	if( (C3 = CheckBox( self->cameraPosition, self->leftPlaneNorm, box )) > 0 ) return -1;
	if( (C4 = CheckBox( self->cameraPosition, self->rightPlaneNorm, box )) > 0 ) return -1;
	if( (C5 = CheckBox( self->cameraPosition, self->topPlaneNorm, box )) > 0 ) return -1;
	if( (C6 = CheckBox( self->cameraPosition, self->bottomPlaneNorm, box )) > 0 ) return -1;
	if( C1 >= 0 && C2 <= 0 && C3 <= 0 && C4 <= 0 && C5 <= 0 && C6 <= 0 ) return 1;
	return 0;
}




typedef struct DaoxPointable  DaoxPointable;

struct DaoxPointable
{
	DaoxSceneNode  base;
	DaoxVector3D   targetPosition;
};



DaoxController* DaoxController_New()
{
	DaoxController *self = (DaoxController*) dao_calloc( 1, sizeof(DaoxController) );
	self->transform = DaoxMatrix4D_Identity();
	return self;
}
void DaoxController_Delete( DaoxController *self )
{
	dao_free( self );
}
void DaoxController_Update( DaoxController *self, float dtime )
{
	int i;
	if( self->animations == NULL ) return;
	for(i=0; i<self->animations->size; ++i){
		DaoxAnimation *anim = self->animations->items.pAnimation[i];
		DaoxAnimation_Update( anim, dtime );
	}
}
DaoxMatrix4D DaoxController_GetTransform( DaoxController *self )
{
	DaoxMatrix4D trans;
	int i;

	if( self->animations == NULL ) return self->transform;

	trans = DaoxMatrix4D_Identity();
	for(i=0; i<self->animations->size; ++i){
		DaoxAnimation *anim = self->animations->items.pAnimation[i];
		trans = DaoxMatrix4D_Product( & anim->transform, & trans );
	}
	return trans;
}



void DaoxSceneNode_Init( DaoxSceneNode *self, DaoType *type, int renderable )
{
	DaoCstruct_Init( (DaoCstruct*) self, type );
	self->renderable = renderable;
	self->parent = NULL;
	self->controller = NULL;
	self->children = DList_New( DAO_DATA_VALUE );
	self->scale = DaoxVector3D_XYZ( 1.0, 1.0, 1.0 );
	self->rotation = self->translation = DaoxVector3D_XYZ( 0.0, 0.0, 0.0 );
}
void DaoxSceneNode_Free( DaoxSceneNode *self )
{
	DaoCstruct_Free( (DaoCstruct*) self );
	DaoGC_DecRC( (DaoValue*) self->parent );
	DList_Delete( self->children );
	if( self->controller && self->controller->animations ){
		DList_Clear( self->controller->animations );
	}
}
DaoxSceneNode* DaoxSceneNode_New()
{
	DaoxSceneNode *self = (DaoxSceneNode*) dao_calloc( 1, sizeof(DaoxSceneNode) );
	DaoxSceneNode_Init( self, daox_type_scene_node, 0 );
	return self;
}
void DaoxSceneNode_Delete( DaoxSceneNode *self )
{
	DaoxSceneNode_Free( self );
	if( self->controller ) DaoxController_Delete( self->controller );
	dao_free( self );
}
void DaoxSceneNode_MoveByXYZ( DaoxSceneNode *self, float dx, float dy, float dz )
{
	self->translation.x += dx;
	self->translation.y += dy;
	self->translation.z += dz;
}
void DaoxSceneNode_MoveXYZ( DaoxSceneNode *self, float x, float y, float z )
{
	self->translation.x = x;
	self->translation.y = y;
	self->translation.z = z;
}
void DaoxSceneNode_MoveBy( DaoxSceneNode *self, DaoxVector3D delta )
{
	DaoxSceneNode_MoveByXYZ( self, delta.x, delta.y, delta.z );
}
void DaoxSceneNode_Move( DaoxSceneNode *self, DaoxVector3D pos )
{
	DaoxSceneNode_MoveXYZ( self, pos.x, pos.y, pos.z );
}
DaoxMatrix4D DaoxSceneNode_GetParentTransform( DaoxSceneNode *self )
{
	DaoxMatrix4D trans;

	if( self->controller ){
		DList *animations = self->controller->animations;
		/* Note: Rotation from animation should be applied before orientation: */
		trans = DaoxController_GetTransform( self->controller );
		if( animations == NULL ) return trans;
		if( animations->items.pAnimation[0]->channel == DAOX_ANIMATE_TF ) return trans;
	}else{
		DaoxQuaternion quaternion = DaoxQuaternion_FromRotation( & self->rotation );
		DaoxMatrix4D rotation = DaoxMatrix4D_FromQuaternion( & quaternion );
		DaoxMatrix4D scale = DaoxMatrix4D_ScaleVector( self->scale );
		trans = DaoxMatrix4D_Product( & rotation, & scale );
	}

	if( self->ctype == daox_type_joint ){
		DaoxJoint *joint = (DaoxJoint*) self;
		DaoxQuaternion quaternion = DaoxQuaternion_FromRotation( & joint->orientation );
		DaoxMatrix4D orientation = DaoxMatrix4D_FromQuaternion( & quaternion );
		trans = DaoxMatrix4D_Product( & orientation, & trans );
	}
	trans.B1 += self->translation.x;
	trans.B2 += self->translation.y;
	trans.B3 += self->translation.z;

	return trans;
}
DaoxMatrix4D DaoxSceneNode_GetWorldTransform( DaoxSceneNode *self )
{
	DaoxMatrix4D transform = DaoxSceneNode_GetParentTransform( self );
	DaoxSceneNode *node = self;
	while( node->parent ){
		DaoxMatrix4D trans = DaoxSceneNode_GetParentTransform( node->parent );
		transform = DaoxMatrix4D_Product( & trans, & transform );
		node = node->parent;
	}
	return transform;
}
DaoxVector3D DaoxSceneNode_GetWorldPosition( DaoxSceneNode *self )
{
	DaoxMatrix4D transform = DaoxMatrix4D_Identity();
	if( self->parent ) transform = DaoxSceneNode_GetWorldTransform( self->parent );
	return DaoxMatrix4D_MulVector( & transform, & self->translation, 1.0 );
}
void DaoxSceneNode_AddChild( DaoxSceneNode *self, DaoxSceneNode *child )
{
	GC_Assign( & child->parent, self );
	DList_Append( self->children, child );
}
static int DaoxAnimation_Compare( void *first, void *second )
{
	DaoxAnimation *a1 = (DaoxAnimation*) first;
	DaoxAnimation *a2 = (DaoxAnimation*) second;
	if( a1->channel == a2->channel ) return 0;
	return a1->channel < a2->channel ? -1 : 1;
}
void DaoxSceneNode_SortAnimations( DaoxSceneNode *self )
{
	if( self->controller && self->controller->animations ){
		DList_Sort( self->controller->animations, DaoxAnimation_Compare );
	}
}





void DaoxPointable_PointAt( DaoxPointable *self, DaoxVector3D pos )
{
	DaoxVector3D axis, newPointDirection;
	DaoxVector3D pointDirection = {0.0,0.0,-1.0};
	DaoxQuaternion rotation = DaoxQuaternion_FromRotation( & self->base.rotation );

	if( DaoxVector3D_Dist( & self->targetPosition, & pos ) < 1E-9 ) return;

	self->targetPosition = pos;
	pointDirection = DaoxQuaternion_Rotate( & rotation, & pointDirection );
	newPointDirection = DaoxVector3D_Sub( & pos, & self->base.translation );

	pointDirection = DaoxVector3D_Normalize( & pointDirection );
	newPointDirection = DaoxVector3D_Normalize( & newPointDirection );
	axis = DaoxVector3D_Cross( & newPointDirection, & pointDirection );
	if( DaoxVector3D_Norm2( & axis ) > 1E-9 ){
		float angle = DaoxVector3D_Angle( & newPointDirection, & pointDirection );
		DaoxQuaternion rot = DaoxQuaternion_FromAxisAngle( & axis, -angle );
		rot = DaoxQuaternion_Product( & rot, & rotation );
		self->base.rotation = DaoxQuaternion_ToRotation( & rot );
	}
}
void DaoxPointable_PointAtXYZ( DaoxPointable *self, float x, float y, float z )
{
	DaoxVector3D pos;
	pos.x = x;
	pos.y = y;
	pos.z = z;
	DaoxPointable_PointAt( self, pos );
}
void DaoxPointable_Move( DaoxPointable *self, DaoxVector3D pos )
{
	double angle;
	DaoxVector3D axis, newPointDirection;
	DaoxVector3D pointDirection = {0.0,0.0,-1.0};
	DaoxQuaternion rotation = DaoxQuaternion_FromRotation( & self->base.rotation );

	if( DaoxVector3D_Dist( & self->base.translation, & pos ) < 1E-9 ) return;

	pointDirection = DaoxQuaternion_Rotate( & rotation, & pointDirection );
	newPointDirection = DaoxVector3D_Sub( & self->targetPosition, & pos );

	pointDirection = DaoxVector3D_Normalize( & pointDirection );
	newPointDirection = DaoxVector3D_Normalize( & newPointDirection );
	axis = DaoxVector3D_Cross( & newPointDirection, & pointDirection );
	if( DaoxVector3D_Norm2( & axis ) > 1E-9 ){
		float angle = DaoxVector3D_Angle( & newPointDirection, & pointDirection );
		DaoxQuaternion rot = DaoxQuaternion_FromAxisAngle( & axis, -angle );
		rot = DaoxQuaternion_Product( & rot, & rotation );
		self->base.rotation = DaoxQuaternion_ToRotation( & rot );
	}
	self->base.translation = pos;

	DaoxPointable_PointAt( self, self->targetPosition );
}
void DaoxPointable_Move2( DaoxPointable *self, DaoxVector3D pos )
{
	DaoxSceneNode_Move( (DaoxSceneNode*) self, pos );
	DaoxPointable_PointAt( self, self->targetPosition );
}
void DaoxPointable_MoveXYZ( DaoxPointable *self, float x, float y, float z )
{
	DaoxSceneNode_MoveXYZ( (DaoxSceneNode*) self, x, y, z );
	DaoxPointable_PointAt( self, self->targetPosition );
}
void DaoxPointable_MoveBy( DaoxPointable *self, DaoxVector3D delta )
{
	DaoxSceneNode_MoveBy( (DaoxSceneNode*) self, delta );
	DaoxPointable_PointAt( self, self->targetPosition );
}
void DaoxPointable_MoveByXYZ( DaoxPointable *self, float dx, float dy, float dz )
{
	DaoxSceneNode_MoveByXYZ( (DaoxSceneNode*) self, dx, dy, dz );
	DaoxPointable_PointAt( self, self->targetPosition );
}





DaoxCamera* DaoxCamera_New()
{
	DaoxCamera *self = (DaoxCamera*) dao_calloc( 1, sizeof(DaoxCamera) );
	DaoxSceneNode_Init( (DaoxSceneNode*) self, daox_type_camera, 0 );
	self->viewTarget.x =  0;
	self->viewTarget.y =  0;
	self->viewTarget.z = -1;
	self->fovAngle = 90.0;
	self->aspectRatio = 1.2;
	self->nearPlane = 0.5;
	self->farPlane = 100.0;
	self->focusPlane = 20.0;
	return self;
}
void DaoxCamera_Delete( DaoxCamera *self )
{
	DaoxSceneNode_Free( (DaoxSceneNode*) self );
	dao_free( self );
}
void DaoxCamera_CopyFrom( DaoxCamera *self, DaoxCamera *other )
{
	self->viewTarget = other->viewTarget;
	self->fovAngle = other->fovAngle;
	self->aspectRatio = other->aspectRatio;
	self->nearPlane = other->nearPlane;
	self->farPlane = other->farPlane;
}

DaoxVector3D DaoxCamera_GetPosition( DaoxCamera *self )
{
	return self->base.translation;
}
DaoxVector3D DaoxCamera_GetDirection( DaoxCamera *self, DaoxVector3D *localDirection )
{
	DaoxQuaternion rotation = DaoxQuaternion_FromRotation( & self->base.rotation );
	return DaoxQuaternion_Rotate( & rotation, localDirection );
}
DaoxVector3D DaoxCamera_GetViewDirection( DaoxCamera *self )
{
	DaoxVector3D direction = {0.0,0.0,-1.0};
	return DaoxCamera_GetDirection( self, & direction );
}
DaoxVector3D DaoxCamera_GetUpDirection( DaoxCamera *self )
{
	DaoxVector3D direction = {0.0,1.0,0.0};
	return DaoxCamera_GetDirection( self, & direction );
}
DaoxVector3D DaoxCamera_GetRightDirection( DaoxCamera *self )
{
	DaoxVector3D direction = {1.0,0.0,0.0};
	return DaoxCamera_GetDirection( self, & direction );
}

void DaoxCamera_Move( DaoxCamera *self, DaoxVector3D pos )
{
	DaoxPointable_Move( (DaoxPointable*) self, pos );
}
void DaoxCamera_MoveXYZ( DaoxCamera *self, float x, float y, float z )
{
	DaoxPointable_MoveXYZ( (DaoxPointable*) self, x, y, z );
}
void DaoxCamera_MoveBy( DaoxCamera *self, DaoxVector3D delta )
{
	delta.x += self->base.translation.x;
	delta.y += self->base.translation.y;
	delta.z += self->base.translation.z;
	DaoxPointable_Move( (DaoxPointable*) self, delta );
}
void DaoxCamera_MoveByXYZ( DaoxCamera *self, float dx, float dy, float dz )
{
	DaoxVector3D delta;
	delta.x = dx;
	delta.y = dy;
	delta.z = dz;
	DaoxCamera_MoveBy( self, delta );
}
void DaoxCamera_RotateBy( DaoxCamera *self, float alpha )
{
	DaoxVector3D cameraPosition = DaoxCamera_GetPosition( self );
	DaoxVector3D cameraDirection = DaoxCamera_GetViewDirection( self );
	DaoxQuaternion rotation = DaoxQuaternion_FromRotation( & self->base.rotation );
	DaoxQuaternion rotation2 = DaoxQuaternion_FromAxisAngle( & cameraDirection, alpha );
	rotation = DaoxQuaternion_Product( & rotation2, & rotation );
	self->base.rotation = DaoxQuaternion_ToRotation( & rotation );
}
void DaoxCamera_Orient( DaoxCamera *self, int xyz )
{
	float unit = xyz > 0 ? 1.0 : -1.0;
	DaoxVector3D xaxis = DaoxVector3D_XYZ( unit, 0.0, 0.0 );
	DaoxVector3D yaxis = DaoxVector3D_XYZ( 0.0, unit, 0.0 );
	DaoxVector3D zaxis = DaoxVector3D_XYZ( 0.0, 0.0, unit );
	DaoxVector3D upaxis = abs(xyz) == 1 ? xaxis : (abs(xyz) == 2 ? yaxis : zaxis);
	DaoxVector3D cameraUp = DaoxCamera_GetUpDirection( self );
	DaoxVector3D cameraDirection = DaoxCamera_GetViewDirection( self );
	DaoxVector3D projection = DaoxVector3D_ProjectToPlane( & upaxis, & cameraDirection );
	DaoxVector3D cross = DaoxVector3D_Cross( & cameraUp, & projection );
	float angle = DaoxVector3D_Angle( & cameraUp, & projection );
	float dot = DaoxVector3D_Dot( & cross, & cameraDirection );
	DaoxCamera_RotateBy( self, 2*M_PI - angle );
}
void DaoxCamera_LookAt( DaoxCamera *self, DaoxVector3D pos )
{
	DaoxPointable_PointAt( (DaoxPointable*) self, pos );
}
void DaoxCamera_LookAtXYZ( DaoxCamera *self, float x, float y, float z )
{
	DaoxPointable_PointAtXYZ( (DaoxPointable*) self, x, y, z );
}





DaoxLight* DaoxLight_New()
{
	DaoxLight *self = (DaoxLight*) dao_calloc( 1, sizeof(DaoxLight) );
	DaoxSceneNode_Init( (DaoxSceneNode*) self, daox_type_light, 0 );
	self->targetPosition.x = 0;
	self->targetPosition.y = -1E16;
	self->targetPosition.z = 0;
	self->intensity.alpha = 1.0;

#if 0
	DaoxModel *model = DaoxModel_New();
	DaoxMesh *mesh = DaoxMesh_New();
	DaoxMesh_MakeBox( mesh, 0.5, 0.5, 0.5 );
	DaoxMesh_UpdateTree( mesh, 0 );
	DaoxModel_SetMesh( model, mesh );
	DaoxSceneNode_AddChild( (DaoxSceneNode*)self, (DaoxSceneNode*)model );
#endif
	return self;
}
void DaoxLight_Delete( DaoxLight *self )
{
	DaoxSceneNode_Free( (DaoxSceneNode*) self );
	dao_free( self );
}
void DaoxLight_CopyFrom( DaoxLight *self, DaoxLight *other )
{
	self->targetPosition = other->targetPosition;
	self->intensity = other->intensity;
}

void DaoxLight_Move( DaoxLight *self, DaoxVector3D pos )
{
	DaoxPointable_Move( (DaoxPointable*) self, pos );
}
void DaoxLight_PointAt( DaoxLight *self, DaoxVector3D pos )
{
	DaoxPointable_PointAt( (DaoxPointable*) self, pos );
}
void DaoxLight_MoveXYZ( DaoxLight *self, float x, float y, float z )
{
	DaoxPointable_MoveXYZ( (DaoxPointable*) self, x, y, z );
}
void DaoxLight_PointAtXYZ( DaoxLight *self, float x, float y, float z )
{
	DaoxPointable_PointAtXYZ( (DaoxPointable*) self, x, y, z );
}





DaoxJoint* DaoxJoint_New()
{
	DaoxJoint *self = (DaoxJoint*) dao_calloc( 1, sizeof(DaoxJoint) );
	DaoxSceneNode_Init( (DaoxSceneNode*) self, daox_type_joint, 0 );

#if 0
	DaoxModel *model = DaoxModel_New();
	DaoxMesh *mesh = DaoxMesh_New();
	DaoxMesh_MakeBox( mesh, 0.5, 0.5, 0.5 );
	DaoxMesh_UpdateTree( mesh, 0 );
	DaoxModel_SetMesh( model, mesh );
	DaoxSceneNode_AddChild( (DaoxSceneNode*)self, (DaoxSceneNode*)model );
#endif
	return self;
}
void DaoxJoint_Delete( DaoxJoint *self )
{
	DaoxSceneNode_Free( (DaoxSceneNode*) self );
	dao_free( self );
}



DaoxSkeleton* DaoxSkeleton_New()
{
	DaoxSkeleton *self = (DaoxSkeleton*) dao_calloc( 1, sizeof(DaoxSkeleton) );
	DaoCstruct_Init( (DaoCstruct*) self, daox_type_skeleton );
	self->joints = DList_New( DAO_DATA_VALUE );
	self->skinMats = DArray_New( sizeof(DaoxMatrix4D) );
	self->skinMats2 = DArray_New( sizeof(DaoxMatrix4D) );
	self->bindMat = DaoxMatrix4D_Identity();
	return self;
}
void DaoxSkeleton_Delete( DaoxSkeleton *self )
{
	DList_Delete( self->joints );
	DArray_Delete( self->skinMats );
	DArray_Delete( self->skinMats2 );
	DaoCstruct_Free( (DaoCstruct*) self );
	dao_free( self );
}
void DaoxSkeleton_UpdateSkinningMatrices( DaoxSkeleton *self )
{
	int i;
	DArray_Resize( self->skinMats2, self->skinMats->size );
	for(i=0; i<self->skinMats->size; ++i){
		DaoxSceneNode *node = self->joints->items.pSceneNode[i];
		DaoxMatrix4D world = DaoxSceneNode_GetWorldTransform( node );
		DaoxMatrix4D mat = self->bindMat;
		mat = DaoxMatrix4D_Product( self->skinMats->data.matrices4d + i, & mat );
		self->skinMats2->data.matrices4d[i] = DaoxMatrix4D_Product( & world, & mat );;
	}
}





DaoxModel* DaoxModel_New()
{
	DaoxModel *self = (DaoxModel*) dao_calloc( 1, sizeof(DaoxModel) );
	DaoxModel_Init( self, daox_type_model, NULL );
	return self;
}
void DaoxModel_Init( DaoxModel *self, DaoType *type, DaoxMesh *mesh )
{
	DaoxSceneNode_Init( (DaoxSceneNode*) self, type, 1 );
	DaoxModel_SetMesh( self, mesh );
	self->skeleton = NULL;
}
void DaoxModel_Free( DaoxModel *self )
{
	DaoxSceneNode_Free( (DaoxSceneNode*) self );
	GC_DecRC( self->mesh );
	GC_DecRC( self->skeleton );
}
void DaoxModel_Delete( DaoxModel *self )
{
	DaoxModel_Free( self );
	dao_free( self );
}
void DaoxModel_SetMesh( DaoxModel *self, DaoxMesh *mesh )
{
	GC_Assign( & self->mesh, mesh );
	if( mesh ) self->base.obbox = mesh->obbox;
}





DaoxScene* DaoxScene_New()
{
	DaoxScene *self = (DaoxScene*) dao_calloc( 1, sizeof(DaoxScene) );
	DaoCstruct_Init( (DaoCstruct*) self, daox_type_scene );
	self->randGenerator = _DaoRandGenerator_New( rand() );
	self->nodes = DList_New( DAO_DATA_VALUE );
	self->lights = DList_New(0);
	self->background.alpha = 1.0;
	return self;
}
void DaoxScene_Delete( DaoxScene *self )
{
	if( self->pathCache ) GC_DecRC( self->pathCache );
	_DaoRandGenerator_Delete( self->randGenerator );
	DaoCstruct_Free( (DaoCstruct*) self );
	DList_Delete( self->nodes );
	DList_Delete( self->lights );
	dao_free( self );
}

void DaoxScene_AddNode( DaoxScene *self, DaoxSceneNode *node )
{
	DList_Append( self->nodes, node );
	if( node->ctype == daox_type_light ) DList_Append( self->lights, node );
	if( node->ctype == daox_type_camera ) self->camera = (DaoxCamera*) node;
}

void DaoxScene_UpdateNode( DaoxScene *self, DaoxSceneNode *node, float dtime )
{
	int i;
	for(i=0; i<node->children->size; ++i){
		DaoxSceneNode *node2 = node->children->items.pSceneNode[i];
		DaoxScene_UpdateNode( self, node2, dtime );
	}
	if( node->controller ) DaoxController_Update( node->controller, dtime );
	if( DaoType_ChildOf( node->ctype, daox_type_emitter ) ){
		DaoxEmitter *emitter = (DaoxEmitter*) node;
		emitter->randGenerator = self->randGenerator;
		DaoxEmitter_Update( emitter, dtime );
		emitter->randGenerator = NULL;
	}
}
void DaoxScene_Update( DaoxScene *self, float dtime )
{
	int i;
	for(i=0; i<self->nodes->size; ++i){
		DaoxSceneNode *node = self->nodes->items.pSceneNode[i];
		DaoxScene_UpdateNode( self, node, dtime );
	}
}


