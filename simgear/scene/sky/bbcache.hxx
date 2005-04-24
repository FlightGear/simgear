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

#ifndef _BBCACHE_HXX
#define _BBCACHE_HXX


#include <plib/sg.h>
#include <plib/ssg.h>
#include <simgear/screen/extensions.hxx>
#include <simgear/screen/RenderTexture.h>

class SGBbCache {
private:

	typedef struct {
		GLuint 	texID;
		int		cldID;
		float	angleX, angleY;
		// creation frame number for debug only
		int		frame;
		// last time this entry was used
		int		frameUsed;
	} bbInfo;

	void freeTextureMemory(void);
	bool allocTextureMemory(int count, int textureDimension);

	// a list of impostors
	bbInfo	*bbList;
	int		bbListCount;
	int		textureWH;
	int		cacheSizeKb;

	// for debug only, stats
	int		builtBBCount;
	// count of generated BB during the current frame
	int		builtBBframe;

	long	frameNumber;
	RenderTexture *rt;
	bool	rtAvailable;

public:
	SGBbCache(void);
	~SGBbCache(void);

	// call this first to initialize everything, cacheCount is the number of texture to allocate
	void init(int cacheCount);

	// free one cache slot, usualy when the cached object is destroyed
	void free(int bbId, int cldId);

	// allocate a new texture, return an index in the cache
	int alloc(int cldId);

	// give the texture name to use
	GLuint QueryTexID(int cldId, int bbId);

	// save the rendered texture from the current context to a new texture
	void setTextureData(int bbId);

	// start the rendering of a billboard in the RTT context
	void beginCapture(void);

	// adjust the projection matrix of the RTT context to the size of the object
	void setRadius(float radius, float dist_center);

	// forget the RTT and go back to the previous rendering context
	void endCapture(void);

	// for debugging only, give the number of frames since the inpostor was built
	int queryImpostorAge(int bbId);

	// can we still use this impostor ?
	bool isBbValid( int cloudId, int bbId, float angleY, float angleX);

	// save view angles of this billboard
	void setReference( int cloudId, int bbId, float angleY, float angleX);

	// prepare the cache for the rendering of a new frame
	void startNewFrame(void);

	// alloc the impostors texture memory given the size of the memory pool
	// if sizeKb == 0 then the memory pool is freed and impostors are disabled
	bool setCacheSize(int sizeKb);

	// alloc the impostors texture memory given the count and size of texture
	// if count == 0 then the memory pool is freed and impostors are disabled
	bool setCacheSize(int count, int textureDimension);

	// return the size of the memory pool used by texture impostors
	int queryCacheSize(void);

	int maxImpostorRegenFrame;
};

#endif // _BBCACHE_HXX
