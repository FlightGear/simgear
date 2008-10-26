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

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include <osg/Texture2D>
#include <osg/PositionAttitudeTransform>

#include <simgear/compiler.h>

#include <plib/sg.h>
#include <simgear/math/sg_random.h>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/math/polar3d.hxx>

#include <algorithm>
#include <vector>

using std::vector;

#include <simgear/environment/visual_enviro.hxx>
#include "sky.hxx"
#include "newcloud.hxx"
#include "cloudfield.hxx"

#if defined(__MINGW32__)
#define isnan(x) _isnan(x)
#endif

#if defined (__FreeBSD__)
#  if __FreeBSD_version < 500000
     extern "C" {
       inline int isnan(double r) { return !(r <= 0 || r >= 0); }
     }
#  endif
#endif


#if defined (__CYGWIN__)
#include <ieeefp.h>
#endif

double SGCloudField::fieldSize = 50000.0;
float SGCloudField::density = 100.0;
double SGCloudField::timer_dt = 0.0;
sgVec3 SGCloudField::view_vec, SGCloudField::view_X, SGCloudField::view_Y;

void SGCloudField::set_density(float density) {
	SGCloudField::density = density;
}

// reposition the cloud layer at the specified origin and orientation
bool SGCloudField::reposition( const SGVec3f& p, const SGVec3f& up, double lon, double lat,
        		       double dt )
{
        osg::Matrix T, LON, LAT;

        LON.makeRotate(lon, osg::Vec3(0, 0, 1));
        LAT.makeRotate(90.0 * SGD_DEGREES_TO_RADIANS - lat, osg::Vec3(0, 1, 0));

        osg::Vec3 u = up.osg();
        u.normalize();

        if ((last_lon == 0.0f) || (fabs(last_lon - lon) > 1.0) || (fabs(last_lat - lat) > 1.0))
        {
                // First time, or large delta requires repositioning from scratch.
                // TODO: Make this calculation better - a 0.5 degree shift will take a number
                // of reposition calls to correct at the moment.
                
                // combine p and asl (meters) to get translation offset.
                osg::Vec3 pos = p.osg();
                pos += u;
                
                T.makeTranslate(pos);

                field_transform->setMatrix( LAT*LON*T );
                last_lon = lon;
                last_lat = lat;
                last_pos = p.osg();
        }
        else
        {
                // Rotation positon back to simple X-Z space.
                osg::Vec3 pos = last_pos;
                pos += u;
                
                T.makeTranslate(pos);
                
                osg::Matrix U = LAT*LON;
                osg::Vec3 x = osg::Vec3f(fieldSize, 0.0, 0.0)*U;
                osg::Vec3 y = osg::Vec3f(0.0, fieldSize, 0.0)*U;

                osg::Matrix V;
                V.makeIdentity();
                V.invert(U*T);
                
                osg::Vec3 q = pos*V;
                
                // Shift the field if we've moved away from the centre box.
                if (q.x() >  fieldSize) last_pos = last_pos + x;
                if (q.x() < -fieldSize) last_pos = last_pos - x;
                if (q.y() >  fieldSize) last_pos = last_pos + y;
                if (q.y() < -fieldSize) last_pos = last_pos - y;
                
                pos = last_pos;
                pos += u;
                
                T.makeTranslate(pos);
                field_transform->setMatrix( LAT*LON*T );
        }
    return true;
}

SGCloudField::SGCloudField() :
        field_root(new osg::Group),
        field_transform(new osg::MatrixTransform),
        field_group(new osg::Switch),
	deltax(0.0),
	deltay(0.0),
        last_lon(0.0),
        last_lat(0.0),
	last_course(0.0),
	last_density(0.0),
        defined3D(false)               
{
        field_root->addChild(field_transform.get()); 
        field_group->setName("3Dcloud");

        // We duplicate the defined field group in a 3x3 array. This way,
        // we can simply shift entire groups around.
        for(int x = -1 ; x <= 1 ; x++) {
                for(int y = -1 ; y <= 1 ; y++ ) {
                        osg::ref_ptr<osg::PositionAttitudeTransform> transform =
                                new osg::PositionAttitudeTransform;
                        transform->addChild(field_group.get());
                        transform->setPosition(osg::Vec3(x*fieldSize, y * fieldSize, 0.0));

                        field_transform->addChild(transform.get());
                }
        }

}

SGCloudField::~SGCloudField() {
}


void SGCloudField::clear(void) {
        int num_children = field_group->getNumChildren();

        for (int i = 0; i < num_children; i++) {
                field_group->removeChild(i);
        }
        SGCloudField::defined3D = false;
}

// use a table or else we see poping when moving the slider...
static int densTable[][10] = {
	{0,0,0,0,0,0,0,0,0,0},
	{1,0,0,0,0,0,0,0,0,0},
	{1,0,0,0,1,0,0,0,0,0},
	{1,0,0,0,1,0,0,1,0,0}, // 30%
	{1,0,1,0,1,0,0,1,0,0},
	{1,0,1,0,1,0,1,1,0,0}, // 50%
	{1,0,1,0,1,0,1,1,0,1},
	{1,0,1,1,1,0,1,1,0,1}, // 70%
	{1,1,1,1,1,0,1,1,0,1},
	{1,1,1,1,1,0,1,1,1,1}, // 90%
	{1,1,1,1,1,1,1,1,1,1}
};

void SGCloudField::applyDensity(void) {

        int row = (int) (density / 10.0);
        int col = 0;
        int num_children = field_group->getNumChildren();

        if (density != last_density) {
                // Switch on/off the children depending on the required density.
                for (int i = 0; i < num_children; i++) {
                        if (++col > 9) col = 0;
                        if ( densTable[row][col] ) {
                                field_group->setValue(i, true);
                        } else {
                                field_group->setValue(i, false);
                        }
                }
        }

        last_density = density;
}

void SGCloudField::addCloud( SGVec3f& pos, SGNewCloud *cloud) {
        defined3D = true;
        osg::ref_ptr<osg::LOD> lod = cloud->genCloud();
        osg::ref_ptr<osg::PositionAttitudeTransform> transform = new osg::PositionAttitudeTransform;

        transform->setPosition(pos.osg());
        transform->addChild(lod.get());

        field_group->addChild(transform.get());
}
