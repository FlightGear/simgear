// Billboard helper class
//
// Written by Harald JOHNSEN, started April 2005.
//
// Copyright (C) 2005  Harald JOHNSEN - hjohnsen@evc.net
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA
//
//

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include <simgear/compiler.h>

#include <plib/sg.h>
#include <plib/ssg.h>
#include <simgear/screen/extensions.hxx>
#include <simgear/screen/RenderTexture.h>
#include SG_GLU_H

#include "bbcache.hxx"


/*
memory usage :
size          1 tex       8 tex       32 tex      64 tex
64x64x4        16k        128k        512k         1Mo
128x128x4      64k        512k        2Mo          4Mo
256x256x4     256k        2Mo         8Mo         16Mo
*/

void SGBbCache::freeTextureMemory(void) {

	if( bbListCount ) {
		for(int i = 0 ; i < bbListCount ; i++) {
			if(bbList[i].texID)
				glDeleteTextures(1, & bbList[i].texID);
		}
		delete [] bbList;
	}
	bbListCount = 0;
	cacheSizeKb = 0;
	textureWH   = 0;
}

bool SGBbCache::allocTextureMemory(int cacheCount, int textureDimension) {
	textureWH = textureDimension;
	bbListCount = cacheCount;
	bbList = new bbInfo[bbListCount];
	for(int i = 0 ; i < bbListCount ; i++) {
		bbList[i].cldID = 0;
		bbList[i].texID = 0;
        glGenTextures(1, &bbList[i].texID);
        glBindTexture(GL_TEXTURE_2D, bbList[i].texID);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 
                         textureDimension, textureDimension, 0, GL_RGB, GL_FLOAT, NULL);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	}
    glBindTexture(GL_TEXTURE_2D, 0);
	cacheSizeKb = (textureDimension * textureDimension * 4);
	cacheSizeKb *= cacheCount;
	cacheSizeKb /= 1024;
	if(rt) {
		rt->BeginCapture();
		glViewport(0, 0, textureDimension, textureDimension);
		rt->EndCapture();
	}
	return true;
}

SGBbCache::SGBbCache(void) :
	bbListCount(0),
	cacheSizeKb(0),
	textureWH(0),
	builtBBCount(0),
	rt(0),
	rtAvailable(false),
	frameNumber(0),
	maxImpostorRegenFrame(20)
{
}

SGBbCache::~SGBbCache(void) {
	if(rt)
		delete rt;
	freeTextureMemory();
}


void SGBbCache::init(int cacheCount) {

	rt = new RenderTexture();
	// don't use default rtt on nvidia/win because of poor performance of glCopyTexSubImage2D
	// wihtout default pattrib params - see opengl forum
	rt->Reset("rgba tex2D ctt");
//	rt->Reset("rgba tex2D");
	if( rt->Initialize(256, 256, true) ) {
		rtAvailable = true;
		if (rt->BeginCapture())
		{
			glViewport(0, 0, 256, 256);
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			gluPerspective(60.0,  1, 1, 5.0);
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
			glDisable(GL_LIGHTING);
			glEnable(GL_COLOR_MATERIAL);
			glDisable(GL_CULL_FACE);
			glDisable(GL_FOG);
			glDisable(GL_DEPTH_TEST);
			glClearColor(0.0, 0.0, 0.0, 0.0);
			glEnable(GL_TEXTURE_2D);
			glEnable(GL_ALPHA_TEST);
			glAlphaFunc(GL_GREATER, 0.0f);
			glEnable(GL_SMOOTH);
			glEnable(GL_BLEND);
			glBlendFunc( GL_ONE, GL_ONE_MINUS_SRC_ALPHA );

			rt->EndCapture();
		}
	}
	if( cacheCount )
		allocTextureMemory( cacheCount, 64 );

}


bool SGBbCache::setCacheSize(int count, int textureDimension) {
	if( count < 0 || count > 500)
		return false;
	freeTextureMemory();
	if( count == 0)
		return true;

	// only allow some reasonable dimensions
	switch(textureDimension) {
		case 0:
			// default size
			textureDimension = 256;
			break;
		case 64:
		case 128:
		case 256:
			break;
		case 512:
			// rt is 256 so higher texture size has no meaning
			textureDimension = 256;
			break;
		default:
			textureDimension = 128;
			break;
	}
	return allocTextureMemory( count, textureDimension);
}


bool SGBbCache::setCacheSize(int sizeKb) {
	if( sizeKb < 0 || sizeKb > 256*1024)
		return false;
	freeTextureMemory();
	if( sizeKb == 0)
		return true;
	int count = 1;
	int textureDimension = 256;
	if( sizeKb >= 8*1024 ) {
		// more than 32 256x256 textures
		textureDimension = 256;
	} else 	if( sizeKb >= 2*1024 ) {
		// more than 32 128x128 textures
		textureDimension = 128;
	} else 	{
		// don't go under 64x64 textures
		textureDimension = 64;
	}
	count = (sizeKb * 1024) / (textureDimension * textureDimension * 4);
	if(count == 0)
		count = 1;
	return allocTextureMemory( count, textureDimension);
}

int SGBbCache::queryCacheSize(void) {
	return cacheSizeKb;
}

void SGBbCache::free(int bbId, int cldId) {
	if( bbId < 0 || bbId >= bbListCount )
		return;
	if( bbList[bbId].cldID != cldId )
		return;
	bbList[bbId].cldID = 0;
}

int SGBbCache::alloc(int cldId) {
	// pretend we have no more texture if render to texture is not available
	if( ! rtAvailable )
		return -1;
	for(int i = 0 ; i < bbListCount ; i++) {
		if( (bbList[i].cldID == 0) && (bbList[i].texID != 0) ) {
            bbList[i].cldID = cldId;
			bbList[i].angleX = -999;
			bbList[i].angleY = -999;
			bbList[i].frameUsed = 0;
			return i;
		}
	}
	return -1;
}

GLuint SGBbCache::QueryTexID(int cldId, int bbId) {
	if( bbId < 0 || bbId >= bbListCount )
		return 0;
	if( bbList[bbId].cldID != cldId )
		return 0;
	return bbList[bbId].texID;
}

int SGBbCache::queryImpostorAge(int bbId) {
	if( bbId < 0 || bbId >= bbListCount )
		return 0;
	return frameNumber - bbList[bbId].frame;
}

void SGBbCache::beginCapture(void) {

	rt->BeginCapture();

	glClear(GL_COLOR_BUFFER_BIT);

}



void SGBbCache::setRadius(float radius, float dist_center) {
	float border;
	//set viewport to texture resolution
	//glViewport(0, 0, 256, 256);
	glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    float near_ = dist_center - radius;
    float far_ = dist_center + radius;
	if( near_ <= 0 ) {
        // we are in trouble
        glFrustum(-1, 1, -1, 1, 1, 1 + radius * 2);
	} else {
        border = (near_ * radius) / sqrt(dist_center * dist_center - radius * radius);
        glFrustum(-border, border, -border, border, near_, far_);
	}
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}
void SGBbCache::setTextureData(int bbId) {
	if( bbId < 0 || bbId >= bbListCount )
		return;

    glBindTexture(GL_TEXTURE_2D, bbList[bbId].texID);
	glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, textureWH, textureWH);
 
//    bbList[bbId].angleX = angleX;
//    bbList[bbId].angleY = angleY;
    bbList[bbId].frame = frameNumber;
	bbList[bbId].frameUsed = frameNumber;
    builtBBCount ++;
	builtBBframe ++;
}

void SGBbCache::endCapture(void) {

	rt->EndCapture();
//    glBindTexture(GL_TEXTURE_2D, rt->GetTextureID() );

}


bool SGBbCache::isBbValid( int cldId, int bbId, float angleY, float angleX) {
	if( bbId < 0 || bbId >= bbListCount )
		return false;
	if( bbList[bbId].cldID != cldId )
		return false;

	// it was just allocated
	if( bbList[bbId].frameUsed == 0)
		return false;

	// we reuse old impostor to speed up things
	if( builtBBframe >= maxImpostorRegenFrame )
		return true;

    if( fabs(angleY - bbList[bbId].angleY) >= 4.0 )
        return false;

    if( fabs(angleX - bbList[bbId].angleX) >= 4.0 )
        return false;

	bbList[bbId].frameUsed = frameNumber;
	return true;
}

// TODO:this is not the right way to handle that
void SGBbCache::setReference( int cldId, int bbId, float angleY, float angleX) {
	if( bbId < 0 || bbId >= bbListCount )
		return;
	if( bbList[bbId].cldID != cldId )
		return;
	bbList[bbId].angleX = angleX;
	bbList[bbId].angleY = angleY;
}

void SGBbCache::startNewFrame(void) {
	builtBBframe = 0;
	// TOTO:find reasonable value
	int minFrameNumber = frameNumber - 500;
	frameNumber++;
	// cleanup of unused enties
	for( int bbId = 0 ; bbId < bbListCount ; bbId++)
		if( (bbList[bbId].cldID != 0) && (bbList[bbId].frameUsed < minFrameNumber) ) {
			// entry is now free
			bbList[bbId].cldID = 0;
		}
}
