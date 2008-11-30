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
#include <simgear/scene/util/RenderConstants.hxx>
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

using namespace simgear;


#if defined (__CYGWIN__)
#include <ieeefp.h>
#endif

float SGCloudField::fieldSize = 50000.0f;
float SGCloudField::density = 100.0f;
double SGCloudField::timer_dt = 0.0;
sgVec3 SGCloudField::view_vec, SGCloudField::view_X, SGCloudField::view_Y;

void SGCloudField::set_density(float density) {
	SGCloudField::density = density;
}

// reposition the cloud layer at the specified origin and orientation
bool SGCloudField::reposition( const SGVec3f& p, const SGVec3f& up, double lon, double lat,
        		       double dt, int asl )
{
    osg::Matrix T, LON, LAT;
    
    // Calculating the reposition information is expensive. 
    // Only perform the reposition every 60 frames.
    reposition_count = (reposition_count + 1) % 60;
    if ((reposition_count != 0) || !defined3D) return false;
    
    SGGeoc pos = SGGeoc::fromGeod(SGGeod::fromRad(lon, lat));
    
    double dist = SGGeodesy::distanceM(cld_pos, pos);
    
    if (dist > (fieldSize * 2)) {
        // First time or very large distance
        SGVec3<double> cart;
        SGGeodesy::SGGeodToCart(SGGeod::fromRad(lon, lat), cart);
        T.makeTranslate(cart.osg());
        
        LON.makeRotate(lon, osg::Vec3(0, 0, 1));
        LAT.makeRotate(90.0 * SGD_DEGREES_TO_RADIANS - lat, osg::Vec3(0, 1, 0));

        field_transform->setMatrix( LAT*LON*T );
        cld_pos = SGGeoc::fromGeod(SGGeod::fromRad(lon, lat));
    } else if (dist > fieldSize) {
        // Distance requires repositioning of cloud field.
        // We can easily work out the direction to reposition
        // from the course between the cloud position and the
        // camera position.
        SGGeoc pos = SGGeoc::fromGeod(SGGeod::fromRad(lon, lat));
        
        float crs = SGGeoc::courseDeg(cld_pos, pos);
        if ((crs < 45.0) || (crs > 315.0)) {
            SGGeodesy::advanceRadM(cld_pos, 0.0, fieldSize, cld_pos);
        }
        
        if ((crs > 45.0) && (crs < 135.0)) {
            SGGeodesy::advanceRadM(cld_pos, SGD_PI_2, fieldSize, cld_pos);
        }

        if ((crs > 135.0) && (crs < 225.0)) {
            SGGeodesy::advanceRadM(cld_pos, SGD_PI, fieldSize, cld_pos);
        }
        
        if ((crs > 225.0) && (crs < 315.0)) {
            SGGeodesy::advanceRadM(cld_pos, SGD_PI + SGD_PI_2, fieldSize, cld_pos);
        }
        
        SGVec3<double> cart;
        SGGeodesy::SGGeodToCart(SGGeod::fromRad(cld_pos.getLongitudeRad(), cld_pos.getLatitudeRad()), cart);
        T.makeTranslate(cart.osg());
        
        LON.makeRotate(cld_pos.getLongitudeRad(), osg::Vec3(0, 0, 1));
        LAT.makeRotate(90.0 * SGD_DEGREES_TO_RADIANS - cld_pos.getLatitudeRad(), osg::Vec3(0, 1, 0));

        field_transform->setMatrix( LAT*LON*T );
    }
    
    field_root->getStateSet()->setRenderBinDetails(asl, "RenderBin");

    return true;
}

SGCloudField::SGCloudField() :
        field_root(new osg::Group),
        field_transform(new osg::MatrixTransform),
	deltax(0.0),
	deltay(0.0),
	last_course(0.0),
	last_density(0.0),
        defined3D(false),
        reposition_count(0)
{
    cld_pos = SGGeoc();
    field_root->addChild(field_transform.get());
    field_root->setName("3D Cloud field root");
    osg::StateSet *rootSet = field_root->getOrCreateStateSet();
    rootSet->setRenderBinDetails(CLOUDS_BIN, "DepthSortedBin");
    
    osg::ref_ptr<osg::Group> quad_root = new osg::Group();
    osg::ref_ptr<osg::LOD> quad[BRANCH_SIZE][BRANCH_SIZE];
    
    for (int i = 0; i < BRANCH_SIZE; i++) {
        for (int j = 0; j < BRANCH_SIZE; j++) {
            quad[i][j] = new osg::LOD();
            quad[i][j]->setName("Quad");
            quad_root->addChild(quad[i][j].get());
        }
    }
    
    int leafs = QUADTREE_SIZE / BRANCH_SIZE;

    for (int x = 0; x < QUADTREE_SIZE; x++) {
        for (int y = 0; y < QUADTREE_SIZE; y++) {
            field_group[x][y]= new osg::Switch;
            field_group[x][y]->setName("3D cloud group");
            
            // Work out where to put this node in the quad tree
            int i = x / leafs;
            int j = y / leafs;
            quad[i][j]->addChild(field_group[x][y].get(), 0.0f, 20000.0f);
        }
    }

    // We duplicate the defined field group in a 3x3 array. This way,
    // we can simply shift entire groups around.
    // TODO: "Bend" the edge groups so when shifted they line up.
    // Currently the clouds "jump down" when we reposition them.
    for(int x = -1 ; x <= 1 ; x++) {
        for(int y = -1 ; y <= 1 ; y++ ) {
            osg::ref_ptr<osg::PositionAttitudeTransform> transform =
                    new osg::PositionAttitudeTransform;
            transform->addChild(quad_root.get());
            transform->setPosition(osg::Vec3(x*fieldSize, y * fieldSize, 0.0));

            field_transform->addChild(transform.get());
        }
    }
}

SGCloudField::~SGCloudField() {
}


void SGCloudField::clear(void) {
    for (int x = 0; x < QUADTREE_SIZE; x++) {
        for (int y = 0; y < QUADTREE_SIZE; y++) {
            int num_children = field_group[x][y]->getNumChildren();

            for (int i = 0; i < num_children; i++) {
                field_group[x][y]->removeChild(i);
            }
        }
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

        if (density != last_density) {
            for (int x = 0; x < QUADTREE_SIZE; x++) {
                for (int y = 0; y < QUADTREE_SIZE; y++) {
                // Switch on/off the children depending on the required density.
                    int num_children = field_group[x][y]->getNumChildren();
                    for (int i = 0; i < num_children; i++) {
                        if (++col > 9) col = 0;
                        if ( densTable[row][col] ) {
                            field_group[x][y]->setValue(i, true);
                        } else {
                            field_group[x][y]->setValue(i, false);
                        }
                    }
                }
            }
        }

        last_density = density;
}

void SGCloudField::addCloud( SGVec3f& pos, SGNewCloud *cloud) {
        defined3D = true;
        osg::ref_ptr<osg::Geode> geode = cloud->genCloud();
        
        // Determine which quadtree to put it in.
        int x = (int) floor((pos.x() + fieldSize/2.0) * QUADTREE_SIZE / fieldSize);
        if (x >= QUADTREE_SIZE) x = (QUADTREE_SIZE - 1);
        if (x < 0) x = 0;
        
        int y = (int) floor((pos.y() + fieldSize/2.0) * QUADTREE_SIZE / fieldSize);
        if (y >= QUADTREE_SIZE) y = (QUADTREE_SIZE - 1);
        if (y < 0) y = 0;
        
        osg::ref_ptr<osg::PositionAttitudeTransform> transform = new osg::PositionAttitudeTransform;

        transform->setPosition(pos.osg());
        transform->addChild(geode.get());
        
        field_group[x][y]->addChild(transform.get(), true);
}
