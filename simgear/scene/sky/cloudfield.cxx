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
#include <osgSim/Impostor>

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

using namespace simgear;

float SGCloudField::fieldSize = 50000.0f;
double SGCloudField::timer_dt = 0.0;
float SGCloudField::view_distance = 20000.0f;
bool SGCloudField::wrap = true;
float SGCloudField::MAX_CLOUD_DEPTH = 2000.0f;
bool SGCloudField::use_impostors = false;
float SGCloudField::lod1_range = 8000.0f;
float SGCloudField::lod2_range = 4000.0f;
float SGCloudField::impostor_distance = 15000.0f;

int impostorcount = 0;
int lodcount = 0;
int cloudcount = 0;

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
        old_pos_accumulated = osg_pos;
    } else if ((old_pos - osg_pos).length() > fieldSize*0.1) {
        // Smaller, but non-trivial movement - check if any clouds need to be moved
        osg::Vec3d ftp = field_transform->getPosition();
        osg::Quat fta =  field_transform->getAttitude();
        osg::Quat ftainv = field_transform->getAttitude().inverse();

        // delta is the vector from the old position to the new position in cloud-coords
        // osg::Vec3f delta = ftainv * (osg_pos - old_pos);
        old_pos = osg_pos;

        bool movement_accumulated = ((old_pos_accumulated - osg_pos).length() > fieldSize*2.0); // FIXME Use a distance of the planet's ~1 degree of great circle arc in this check.
        if (movement_accumulated) {
            // Big movement accumulated - reposition centered to current location, restore wind effect from above, transform existing clouds.
            // Prevents clouds from tilting when too far from the old position.
            field_transform->setAttitude(orient);
            field_transform->setPosition(osg_pos + orient * wind);
            old_pos_accumulated = osg_pos;
        }

        // Check if any of the clouds should be moved.
        for(CloudHash::const_iterator itr = cloud_hash.begin(), end = cloud_hash.end();
            itr != end;
            ++itr) {

             osg::ref_ptr<osg::PositionAttitudeTransform> pat = itr->second;

             if (pat == 0) {
                continue;
             }

             osg::Vec3f currpos = ftp + fta * pat->getPosition();

             // Determine the vector from the new position to the cloud in cloud-space.
             osg::Vec3f w =  ftainv * (currpos - toOsg(cart));

             // Determine a course if required. Note that this involves some axis translation.
             float x = 0.0;
             float y = 0.0;
             if (w.x() >  0.6*fieldSize) { y =  fieldSize; }
             if (w.x() < -0.6*fieldSize) { y = -fieldSize; }
             if (w.y() >  0.6*fieldSize) { x = -fieldSize; }
             if (w.y() < -0.6*fieldSize) { x =  fieldSize; }

             if ((x != 0.0) || (y != 0.0) || movement_accumulated) {
                 removeCloudFromTree(pat);
                 // XXX Why is the thing inside of toSG() recalculating currpos instead of making it Vec3d and using here?
                 SGGeod p = SGGeod::fromCart(toSG(ftp +
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
    old_pos_accumulated = osg::Vec3f(0.0f, 0.0f, 0.0f);
    field_root->addChild(field_transform.get());
    field_root->setName("3D Cloud field root");
    osg::StateSet *rootSet = field_root->getOrCreateStateSet();
    rootSet->setRenderBinDetails(CLOUDS_BIN, "DepthSortedBin");
    rootSet->setAttributeAndModes(getFog());

    field_transform->addChild(altitude_transform.get());
    placed_root = new osg::Group();
    altitude_transform->addChild(placed_root);
    impostorcount = 0;
    lodcount = 0;
    cloudcount = 0;
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

void SGCloudField::applyVisAndLoDRange(void)
{
    for (unsigned int i = 0; i < placed_root->getNumChildren(); i++) {
        osg::ref_ptr<osg::LOD> lodnode1 = (osg::LOD*) placed_root->getChild(i);
        for (unsigned int j = 0; j < lodnode1->getNumChildren(); j++) {
            lodnode1->setRange(j, 0.0f, lod1_range + lod2_range + view_distance + MAX_CLOUD_DEPTH);
            osg::ref_ptr<osg::LOD> lodnode2 = (osg::LOD*) lodnode1->getChild(j);
            for (unsigned int k = 0; k < lodnode2->getNumChildren(); k++) {
                lodnode2->setRange(k, 0.0f, view_distance + MAX_CLOUD_DEPTH);
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
    addCloudToTree(transform, lon, lat, alt, x, y, true);
    cloud_hash[index] = transform;
    return true;
}

// Remove a give cloud from inside the tree, without removing it from the cloud hash
void SGCloudField::removeCloudFromTree(osg::ref_ptr<osg::PositionAttitudeTransform> transform)
{
    if (transform == 0)
    {
        // Ooops!
        return;
    }
    osg::ref_ptr<osg::Group> lodnode = transform->getParent(0);
    lodnode->removeChild(transform);
    cloudcount--;

    if (lodnode->getNumChildren() == 0) {
        osg::ref_ptr<osg::Group> lodnode1 = lodnode->getParent(0);
        osg::ref_ptr<osgSim::Impostor> impostornode = (osgSim::Impostor*) lodnode1->getParent(0);

        lodnode1->removeChild(lodnode);
        lodcount--;

        if (lodnode1->getNumChildren() == 0) {
          impostornode->removeChild(lodnode1);
          placed_root->removeChild(impostornode);
          impostorcount--;
        }
    }
}

void SGCloudField::addCloudToTree(osg::ref_ptr<osg::PositionAttitudeTransform> transform,
                                  float lon, float lat, float alt, float x, float y, bool auto_reposition) {

    // Get the base position
    SGGeod loc = SGGeod::fromDegFt(lon, lat, alt);
    addCloudToTree(transform, loc, x, y, auto_reposition);
}


void SGCloudField::addCloudToTree(osg::ref_ptr<osg::PositionAttitudeTransform> transform,
                                  SGGeod loc, float x, float y, bool auto_reposition) {

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
    addCloudToTree(transform, loc, auto_reposition);
}


void SGCloudField::addCloudToTree(osg::ref_ptr<osg::PositionAttitudeTransform> transform, SGGeod loc, bool auto_reposition) {
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
        old_pos_accumulated = osg_pos;
    } else if (auto_reposition) {
        SGVec3<double> fieldcenter;
        SGGeodesy::SGGeodToCart(SGGeod::fromDegFt(loc.getLongitudeDeg(), loc.getLatitudeDeg(), 0.0f), fieldcenter);
        osg::Quat orient = toOsg(SGQuatd::fromLonLatDeg(loc.getLongitudeDeg(), loc.getLatitudeDeg()) * SGQuatd::fromRealImag(0, SGVec3d(0, 1, 0)));
        osg::Vec3f osg_pos = toOsg(fieldcenter);
        if ((old_pos_accumulated - osg_pos).length() > fieldSize*2.0) { // FIXME Use a distance of the planet's ~1 degree of great circle arc in this check.
            // Big movement accumulated - reposition existing clouds centered to current location.
            // Prevents clouds from tilting when too far from the old position.
            osg::Vec3d ftp = field_transform->getPosition();
            osg::Quat fta =  field_transform->getAttitude();
            field_transform->setPosition(osg_pos);
            field_transform->setAttitude(orient);
            old_pos = osg_pos;
            old_pos_accumulated = osg_pos;
            for(CloudHash::const_iterator itr = cloud_hash.begin(), end = cloud_hash.end();
                itr != end;
                ++itr) {
                 osg::ref_ptr<osg::PositionAttitudeTransform> pat = itr->second;
                 if (pat == 0) {
                    continue;
                 }
                 removeCloudFromTree(pat);
                 SGGeod p = SGGeod::fromCart(toSG(ftp + fta * pat->getPosition()));
                 addCloudToTree(pat, p, 0.0, 0.0);
            }
        }
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
    osg::ref_ptr<osgSim::Impostor> impostornode;

    for (unsigned int i = 0; (!found) && (i < placed_root->getNumChildren()); i++) {
        lodnode1 = (osg::LOD*) placed_root->getChild(i);
        if ((lodnode1->getCenter() - pos).length2() < lod1_range*lod1_range) {
          // New cloud is within RADIUS_LEVEL_1 of the center of the LOD node.
          found = true;
        }
    }

    if (!found) {
        if (use_impostors) {
          impostornode = new osgSim::Impostor();
          impostornode->setImpostorThreshold(impostor_distance);
          //impostornode->setImpostorThresholdToBound();
          //impostornode->setCenter(pos);
          placed_root->addChild(impostornode.get());
          lodnode1 = (osg::ref_ptr<osg::LOD>) impostornode;
        } else {
          lodnode1 = new osg::LOD();
          placed_root->addChild(lodnode1.get());
        }
        impostorcount++;
    }

    // Now check if there is a second level LOD node at an appropriate distance
    found = false;

    for (unsigned int j = 0; (!found) && (j < lodnode1->getNumChildren()); j++) {
        lodnode = (osg::LOD*) lodnode1->getChild(j);
        if ((lodnode->getCenter() - pos).length2() < lod2_range*lod2_range) {
            // We've found the right leaf LOD node
            found = true;
        }
    }

    if (!found) {
        // No suitable leaf node was found, so we need to add one.
        lodnode = new osg::LOD();
        lodnode1->addChild(lodnode, 0.0f, lod1_range + lod2_range + view_distance + MAX_CLOUD_DEPTH);
        lodcount++;
    }

    transform->setPosition(pos);
    lodnode->addChild(transform.get(), 0.0f, view_distance + MAX_CLOUD_DEPTH);
    cloudcount++;
    SG_LOG(SG_ENVIRONMENT, SG_DEBUG, "Impostors: " << impostorcount <<
                                     " LoD: " << lodcount <<
                                     " Clouds: " << cloudcount);

    lodnode->dirtyBound();
    lodnode1->dirtyBound();
    field_root->dirtyBound();
}

bool SGCloudField::deleteCloud(int identifier) {
    osg::ref_ptr<osg::PositionAttitudeTransform> transform = cloud_hash[identifier];
    if (transform == 0) return false;

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
    return (! cloud_hash.empty());
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
