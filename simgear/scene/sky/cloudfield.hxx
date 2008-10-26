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
#include <osgSim/Impostor>
#include <osgDB/ReaderWriter>

#include <osg/ref_ptr>
#include <osg/Array>
#include <osg/Geometry>
#include <osg/Group>
#include <osg/Switch>
#include <osg/Billboard>

#include <simgear/misc/sg_path.hxx>

using std::vector;

class SGNewCloud;

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

        float Rnd(float);

	// this is a relative position only, with that we can move all clouds at once
	sgVec3 relative_position;
        //	double lon, lat;

        osg::ref_ptr<osg::Group> field_root;
        osg::ref_ptr<osg::MatrixTransform> field_transform;
        osg::ref_ptr<osg::Switch> field_group;

	double deltax, deltay, alt;
        double last_lon, last_lat, last_course;
        sgSphere field_sphere;
	float	last_density;
        osg::Vec3 last_pos;

public:

	SGCloudField();
	~SGCloudField();

	void clear(void);

	// add one cloud, data is not copied, ownership given
	void addCloud( SGVec3f& pos, SGNewCloud *cloud);

        /**
        * reposition the cloud layer at the specified origin and
        * orientation.
        * @param p position vector
        * @param up the local up vector
        * @param lon specifies a rotation about the Z axis
        * @param lat specifies a rotation about the new Y axis
        * @param spin specifies a rotation about the new Z axis
        *        (and orients the sunrise/set effects)
        * @param dt the time elapsed since the last call
        */
        bool reposition( const SGVec3f& p, const SGVec3f& up,
                        double lon, double lat, double dt = 0.0 );

        osg::Group* getNode() { return field_root.get(); }

	// visibility distance for clouds in meters
	static float CloudVis;

	static sgVec3 view_vec, view_X, view_Y;

	static float density;
	static double timer_dt;
	static double fieldSize;
	
        bool defined3D;

	static float get_density(void) { return density; }

	static void set_density(float density);

        void applyDensity(void);
};

#endif // _CLOUDFIELD_HXX
