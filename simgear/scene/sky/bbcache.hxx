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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
//

#ifndef _BBCACHE_HXX
#define _BBCACHE_HXX


#include <plib/sg.h>
#include <plib/ssg.h>
#include <simgear/screen/extensions.hxx>
#include <simgear/screen/RenderTexture.h>

/**
 * Billboard helper class.
 */
class SGBbCache {
private:

	/**
	* storage class for impostors state.
	*/
	class bbInfo {
	public:
		/// the texture used by this impostor
		GLuint 	texID;
		/// the cloud owning this impostor
		int		cldID;
		float	angleX, angleY;
		// creation frame number for debug only
		int		frame;
		/// last time this entry was used
		int		frameUsed;
		/// dirty flag for lazy rebuild of impostor
		bool	needRedraw;
	};

	void freeTextureMemory(void);
    /**
     * Allocate and initialize the texture pool.
     * @param count the number of texture to build
     * @param textureDimension size in pixel of each texture
     */
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

    /**
     * Call this first to initialize the cache.
     * @param cacheCount the number of texture to allocate
     */
	void init(int cacheCount);

    /**
     * Free one cache slot, usualy when the cached object is destroyed.
     * @param bbId the impostor slot
     * @param cldId the cloud identifier
     */
	void free(int bbId, int cldId);

    /**
     * Allocate a new impostor.
     * @param cldId the cloud identifier
     * @return an impostor slot
     */
	int alloc(int cldId);

    /**
     * Query the texture name associated with this cloud.
     * @param bbId the impostor slot
     * @param cldId the cloud identifier
     * @return a texture name
     */
	GLuint QueryTexID(int cldId, int bbId);

    /**
     * Save the rendered texture from the current context to a new texture.
     * @param bbId the impostor slot
     */
	void setTextureData(int bbId);

    /**
     * Start the rendering of a billboard in the RTT context.
     */
	void beginCapture(void);

    /**
     * Adjust the projection matrix of the RTT context to the size of the object.
     * @param radius radius in meters of the object to draw
     * @param dist_center distance between viewer and object
     */
	void setRadius(float radius, float dist_center);

    /**
     * Forget the RTT and go back to the previous rendering context.
     */
	void endCapture(void);

    /**
     * For debugging only, give the number of frames since the impostor was built.
     * @param bbId the impostor slot
     */
	int queryImpostorAge(int bbId);

    /**
     * Can we still use this impostor ? Check versus view angles and load.
     * @param bbId the impostor slot
     * @param cloudId the cloud identifier
     * @param angleY rotation needed to face the impostor
     * @param angleX rotation needed to face the impostor
     */
	bool isBbValid( int cloudId, int bbId, float angleY, float angleX);

    /**
     * Save view angles of this billboard.
     * @param bbId the impostor slot
     * @param cloudId the cloud identifier
     * @param angleY rotation needed to face the impostor
     * @param angleX rotation needed to face the impostor
     */
	void setReference( int cloudId, int bbId, float angleY, float angleX);

    /**
     * Prepare the cache for the rendering of a new frame.
     * Do some garbage collect of unused impostors
     */
	void startNewFrame(void);

    /**
     * Alloc the impostors texture memory given the size of the memory pool.
	 * If sizeKb == 0 then the memory pool is freed and impostors are disabled
     * @param sizeKb size of the texture pool in K
     */
	bool setCacheSize(int sizeKb);

    /**
     * Alloc the impostors texture memory given the count and size of texture.
	 * If count == 0 then the memory pool is freed and impostors are disabled
     * @param count number of texture to allocate
     * @param textureDimension size of each texture in pixels
     */
	bool setCacheSize(int count, int textureDimension);

	bool isRttAvailable(void) { return rtAvailable; }

    /**
     * Force all impostors to be rebuilt.
     */
	void invalidateCache(void);

    /**
     * Flag the impostor for a lazy update.
     * @param bbId the impostor slot
     * @param cldId the cloud identifier
     */
	void invalidate(int cldId, int bbId);

    /**
     * Return the size of the memory pool used by texture impostors.
     * @return size of the memory pool in Kb
     */
	int queryCacheSize(void);

    /**
     * Maximum number of impostor to regen each frame.
     * If we can't update all of them we will do that in the next frame
     */
	int maxImpostorRegenFrame;
};

#endif // _BBCACHE_HXX
