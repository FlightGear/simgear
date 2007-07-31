// a layer of 3d clouds
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

#ifndef _CLOUDFIELD_HXX
#define _CLOUDFIELD_HXX

#include <plib/sg.h>
#include <simgear/compiler.h>
#include <vector>


SG_USING_STD(vector);

class SGNewCloud;

class culledCloud {
public:
	SGNewCloud	*aCloud;
	sgVec3		eyePos;
	float		dist;
	float		heading;
	float		alt;
	bool operator<(const culledCloud &b) const {
		return (this->dist < b.dist);
	}
};
typedef vector<culledCloud> list_of_culledCloud;

/**
 * A layer of 3D clouds.
 */
class SGCloudField {

private:
	class Cloud  {
	public:
		SGNewCloud	*aCloud;
		sgVec3		pos;
		bool		visible;
	};


	typedef vector<Cloud> list_of_Cloud;

	// cull all clouds of a tiled field
	void cullClouds(sgVec3 eyePos, sgMat4 mat);

	void applyDensity(void);

	list_of_Cloud theField;
	// this is a relative position only, with that we can move all clouds at once
	sgVec3 relative_position;
//	double lon, lat;

	sgFrustum frustum;

	sgMat4 transform;
	double deltax, deltay, alt;
    double last_lon, last_lat, last_course;
    sgSphere field_sphere;
	float	last_density;
	bool	draw_in_3d;

public:

	SGCloudField();
	~SGCloudField();

	void clear(void);

	// add one cloud, data is not copied, ownership given
	void addCloud( sgVec3 pos, SGNewCloud *cloud);

	// for debug only
	void buildTestLayer(void);

	// Render a cloud field
	void Render( float *sun_color );

	// reposition the cloud layer at the specified origin and orientation
	void reposition( sgVec3 p, sgVec3 up, double lon, double lat, double alt, double dt, float direction, float speed);

	bool is3D(void) { return draw_in_3d; }

	// visibility distance for clouds in meters
	static float CloudVis;

	static sgVec3 view_vec, view_X, view_Y;

	static float density;
	static double timer_dt;
	static double fieldSize;
	static bool enable3D;

	// return the size of the memory pool used by texture impostors
	static int get_CacheSize(void);
	static int get_CacheResolution(void);
	static float get_CloudVis(void) { return CloudVis; }
	static float get_density(void) { return density; }
	static bool get_enable3dClouds(void) { return enable3D; }

	static void set_CacheSize(int sizeKb);
	static void set_CacheResolution(int resolutionPixels);
	static void set_CloudVis(float distance);
	static void set_density(float density);
	static void set_enable3dClouds(bool enable);
};

#endif // _CLOUDFIELD_HXX
