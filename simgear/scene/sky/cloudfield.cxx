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

#include <osg/Fog>
#include <osg/Texture2D>
#include <osg/PositionAttitudeTransform>
#include <osg/Vec4f>

#include <simgear/compiler.h>

#include <simgear/math/sg_random.h>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/scene/util/SGSceneUserData.hxx>

#include <algorithm>
#include <vector>
#include <iostream>

using namespace std;

using std::vector;

//#include <simgear/environment/visual_enviro.hxx>
#include <simgear/scene/util/RenderConstants.hxx>
#include <simgear/scene/util/SGUpdateVisitor.hxx>
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

float SGCloudField::fieldSize = 50000.0f;
double SGCloudField::timer_dt = 0.0;
float SGCloudField::view_distance = 20000.0f;
bool SGCloudField::wrap = true;
float SGCloudField::RADIUS_LEVEL_1 = 5000.0f;
float SGCloudField::RADIUS_LEVEL_2 = 1000.0f;

SGVec3f SGCloudField::view_vec, SGCloudField::view_X, SGCloudField::view_Y;


// Reposition the cloud layer at the specified origin and orientation
bool SGCloudField::reposition( const SGVec3f& p, const SGVec3f& up, double lon, double lat,
        		       double dt, int asl, float speed, float direction ) {
    // Determine any movement of the placed clouds
    if (placed_root->getNumChildren() == 0) return false;

    SGVec3<double> cart;
    SGGeod new_pos = SGGeod::fromRadFt(lon, lat, 0.0f);
                            
    SGGeodesy::SGGeodToCart(new_pos, cart);
    osg::Vec3f osg_pos = toOsg(cart);
    osg::Quat orient = toOsg(SGQuatd::fromLonLatRad(lon, lat) * SGQuatd::fromRealImag(0, SGVec3d(0, 1, 0)));
    
    // Always update the altitude transform, as this allows
    // the clouds to rise and fall smoothly depending on environment updates.
    altitude_transform->setPosition(osg::Vec3f(0.0f, 0.0f, (float) asl));
    
    // Similarly, always determine the effects of the wind
    osg::Vec3f wind = osg::Vec3f(-cos((direction + 180)* SGD_DEGREES_TO_RADIANS) * speed * dt,
                                 sin((direction + 180)* SGD_DEGREES_TO_RADIANS) * speed * dt,
                                 0.0f);
    osg::Vec3f windosg = field_transform->getAttitude() * wind;
    field_transform->setPosition(field_transform->getPosition() + windosg);
    
    if (!wrap) {
        // If we're not wrapping the cloudfield, then we make no effort to reposition
        return false;
    }
        
    if ((old_pos - osg_pos).length() > fieldSize*2) {
        // Big movement - reposition centered to current location.
        field_transform->setPosition(osg_pos);
        field_transform->setAttitude(orient);
        old_pos = osg_pos;
    } else {        
        osg::Quat fta =  field_transform->getAttitude();
        osg::Quat ftainv = field_transform->getAttitude().inverse();
        
        // delta is the vector from the old position to the new position in cloud-coords
        // osg::Vec3f delta = ftainv * (osg_pos - old_pos);
        old_pos = osg_pos;
                
        // Check if any of the clouds should be moved.
        for(CloudHash::const_iterator itr = cloud_hash.begin(), end = cloud_hash.end();
            itr != end;
            ++itr) {
              
             osg::ref_ptr<osg::PositionAttitudeTransform> pat = itr->second;
             osg::Vec3f currpos = field_transform->getPosition() + fta * pat->getPosition();
                                      
             // Determine the vector from the new position to the cloud in cloud-space.
             osg::Vec3f w =  ftainv * (currpos - toOsg(cart));               
              
             // Determine a course if required. Note that this involves some axis translation.
             float x = 0.0;
             float y = 0.0;
             if (w.x() >  0.6*fieldSize) { y =  fieldSize; }
             if (w.x() < -0.6*fieldSize) { y = -fieldSize; }
             if (w.y() >  0.6*fieldSize) { x = -fieldSize; }
             if (w.y() < -0.6*fieldSize) { x =  fieldSize; }
              
             if ((x != 0.0) || (y != 0.0)) {
                 removeCloudFromTree(pat);
                 SGGeod p = SGGeod::fromCart(toSG(field_transform->getPosition() + 
                                                  fta * pat->getPosition()));
                 addCloudToTree(pat, p, x, y);                
            }
        }        
    }
    
    // Render the clouds in order from farthest away layer to nearest one.
    field_root->getStateSet()->setRenderBinDetails(CLOUDS_BIN, "DepthSortedBin");
    return true;
}

SGCloudField::SGCloudField() :
        field_root(new osg::Group),
        field_transform(new osg::PositionAttitudeTransform),
        altitude_transform(new osg::PositionAttitudeTransform)
{
    old_pos = osg::Vec3f(0.0f, 0.0f, 0.0f);
    field_root->addChild(field_transform.get());
    field_root->setName("3D Cloud field root");
    osg::StateSet *rootSet = field_root->getOrCreateStateSet();
    rootSet->setRenderBinDetails(CLOUDS_BIN, "DepthSortedBin");
    rootSet->setAttributeAndModes(getFog());
    
    field_transform->addChild(altitude_transform.get());
    placed_root = new osg::Group();
    altitude_transform->addChild(placed_root);
}
    
SGCloudField::~SGCloudField() {
        }


void SGCloudField::clear(void) {

    for(CloudHash::const_iterator itr = cloud_hash.begin(), end = cloud_hash.end();
        itr != end;
        ++itr) {
        removeCloudFromTree(itr->second);
    }
    
    cloud_hash.clear();
}

void SGCloudField::applyVisRange(void)
{
    for (unsigned int i = 0; i < placed_root->getNumChildren(); i++) {
        osg::ref_ptr<osg::LOD> lodnode1 = (osg::LOD*) placed_root->getChild(i);
        for (unsigned int j = 0; j < lodnode1->getNumChildren(); j++) {
            osg::ref_ptr<osg::LOD> lodnode2 = (osg::LOD*) lodnode1->getChild(j);
            for (unsigned int k = 0; k < lodnode2->getNumChildren(); k++) {
                lodnode2->setRange(k, 0.0f, view_distance);
            }
        }
    }
}
            
bool SGCloudField::addCloud(float lon, float lat, float alt, int index, osg::ref_ptr<EffectGeode> geode) {
  return addCloud(lon, lat, alt, 0.0f, 0.0f, index, geode);
        }

bool SGCloudField::addCloud(float lon, float lat, float alt, float x, float y, int index, osg::ref_ptr<EffectGeode> geode) {
    // If this cloud index already exists, don't replace it.
    if (cloud_hash[index]) return false;

    osg::ref_ptr<osg::PositionAttitudeTransform> transform = new osg::PositionAttitudeTransform;

    transform->addChild(geode.get());
    addCloudToTree(transform, lon, lat, alt, x, y);
    cloud_hash[index] = transform;
    return true;
    }
    
// Remove a give cloud from inside the tree, without removing it from the cloud hash
void SGCloudField::removeCloudFromTree(osg::ref_ptr<osg::PositionAttitudeTransform> transform)
{
    osg::ref_ptr<osg::Group> lodnode = transform->getParent(0);
    lodnode->removeChild(transform);

    // Clean up the LOD nodes if required
    if (lodnode->getNumChildren() == 0) {
        osg::ref_ptr<osg::Group> lodnode1 = lodnode->getParent(0);
            
        lodnode1->removeChild(lodnode);
            
        if (lodnode1->getNumChildren() == 0) {
            placed_root->removeChild(lodnode1);
        }
    }
}

void SGCloudField::addCloudToTree(osg::ref_ptr<osg::PositionAttitudeTransform> transform,
                                  float lon, float lat, float alt, float x, float y) {
    
    // Get the base position
    SGGeod loc = SGGeod::fromDegFt(lon, lat, alt);
    addCloudToTree(transform, loc, x, y);      
}


void SGCloudField::addCloudToTree(osg::ref_ptr<osg::PositionAttitudeTransform> transform,
                                  SGGeod loc, float x, float y) {
        
    float alt = loc.getElevationFt();
    // Determine any shift by x/y
    if ((x != 0.0f) || (y != 0.0f)) {
        double crs = 90.0 - SG_RADIANS_TO_DEGREES * atan2(y, x); 
        double dst = sqrt(x*x + y*y);
        double endcrs;
        
        SGGeod base_pos = SGGeod::fromGeodFt(loc, 0.0f);            
        SGGeodesy::direct(base_pos, crs, dst, loc, endcrs);
    }
    
    // The direct call provides the position at 0 alt, so adjust as required.      
    loc.setElevationFt(alt);    
    addCloudToTree(transform, loc);    
}
        
        
void SGCloudField::addCloudToTree(osg::ref_ptr<osg::PositionAttitudeTransform> transform, SGGeod loc) {        
    // Work out where this cloud should go in OSG coordinates.
    SGVec3<double> cart;
    SGGeodesy::SGGeodToCart(loc, cart);
    osg::Vec3f pos = toOsg(cart);


    if (old_pos == osg::Vec3f(0.0f, 0.0f, 0.0f)) {
        // First setup.
        SGVec3<double> fieldcenter;
        SGGeodesy::SGGeodToCart(SGGeod::fromDegFt(loc.getLongitudeDeg(), loc.getLatitudeDeg(), 0.0f), fieldcenter);
        // Convert to the scenegraph orientation where we just rotate around
        // the y axis 180 degrees.
        osg::Quat orient = toOsg(SGQuatd::fromLonLatDeg(loc.getLongitudeDeg(), loc.getLatitudeDeg()) * SGQuatd::fromRealImag(0, SGVec3d(0, 1, 0)));
        
        osg::Vec3f osg_pos = toOsg(fieldcenter);            
        
        field_transform->setPosition(osg_pos);
        field_transform->setAttitude(orient);
        old_pos = osg_pos;
    }
    
    // Convert position to cloud-coordinates
    pos = pos - field_transform->getPosition();
    pos = field_transform->getAttitude().inverse() * pos;    

    // We have a two level dynamic quad tree which the cloud will be added
    // to. If there are no appropriate nodes in the quad tree, they are
    // created as required.
    bool found = false;
    osg::ref_ptr<osg::LOD> lodnode1;
    osg::ref_ptr<osg::LOD> lodnode;

    for (unsigned int i = 0; (!found) && (i < placed_root->getNumChildren()); i++) {
        lodnode1 = (osg::LOD*) placed_root->getChild(i);
        if ((lodnode1->getCenter() - pos).length2() < RADIUS_LEVEL_1*RADIUS_LEVEL_1) {
            // New cloud is within RADIUS_LEVEL_1 of the center of the LOD node.
            found = true;
        }
    }

    if (!found) {
        lodnode1 = new osg::LOD();
        placed_root->addChild(lodnode1.get());
    }

    // Now check if there is a second level LOD node at an appropriate distance
    found = false;

    for (unsigned int j = 0; (!found) && (j < lodnode1->getNumChildren()); j++) {
        lodnode = (osg::LOD*) lodnode1->getChild(j);
        if ((lodnode->getCenter() - pos).length2() < RADIUS_LEVEL_2*RADIUS_LEVEL_2) {
            // We've found the right leaf LOD node
            found = true;
        }
    }

    if (!found) {
        // No suitable leave node was found, so we need to add one.
        lodnode = new osg::LOD();
        lodnode1->addChild(lodnode, 0.0f, 4*RADIUS_LEVEL_1);
    }

    transform->setPosition(pos);
    lodnode->addChild(transform.get(), 0.0f, view_distance);

    lodnode->dirtyBound();
    lodnode1->dirtyBound();
    field_root->dirtyBound();
}
        
bool SGCloudField::deleteCloud(int identifier) {
    osg::ref_ptr<osg::PositionAttitudeTransform> transform = cloud_hash[identifier];
    if (transform == NULL) return false;
        
    removeCloudFromTree(transform);
    cloud_hash.erase(identifier);

    return true;
}
        
bool SGCloudField::repositionCloud(int identifier, float lon, float lat, float alt) {
    return repositionCloud(identifier, lon, lat, alt, 0.0f, 0.0f);
}

bool SGCloudField::repositionCloud(int identifier, float lon, float lat, float alt, float x, float y) {
    osg::ref_ptr<osg::PositionAttitudeTransform> transform = cloud_hash[identifier];
    
    if (transform == NULL) return false;

    removeCloudFromTree(transform);
    addCloudToTree(transform, lon, lat, alt, x, y);
    return true;
    }

bool SGCloudField::isDefined3D(void) {
    return (cloud_hash.size() > 0);
}

SGCloudField::CloudFog::CloudFog() {
    fog = new osg::Fog;
    fog->setMode(osg::Fog::EXP2);
    fog->setDataVariance(osg::Object::DYNAMIC);
}

void SGCloudField::updateFog(double visibility, const osg::Vec4f& color) {
    const double sqrt_m_log01 = sqrt(-log(0.01));
    osg::Fog* fog = CloudFog::instance()->fog.get();
    fog->setColor(color);
    fog->setDensity(sqrt_m_log01 / visibility);
}

