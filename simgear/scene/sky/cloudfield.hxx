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

#include <simgear/compiler.h>
#include <vector>
#include <map>

#include <osgDB/ReaderWriter>

#include <osg/ref_ptr>
#include <osg/Array>
#include <osg/Geometry>
#include <osg/Group>
#include <osg/Switch>

namespace osg
{
        class Fog;
        class StateSet;
        class Vec4f;
}

#include <simgear/misc/sg_path.hxx>
#include <simgear/structure/Singleton.hxx>
#include <simgear/math/SGMath.hxx>

using std::vector;

class SGNewCloud;

namespace simgear
{
    class EffectGeode;
}

typedef std::map<int, osg::ref_ptr<osg::PositionAttitudeTransform> > CloudHash;

/**
 * A layer of 3D clouds, defined by lat/long/alt.
 */
class SGCloudField {

private:
	class Cloud  {
	public:
		SGNewCloud	*aCloud;
		SGVec3f		pos;
		bool		visible;
	};

  float Rnd(float);
        
  // Theoretical maximum cloud depth, used for fudging the LoD
  // ranges to ensure that clouds become visible at maximum range
  static float MAX_CLOUD_DEPTH;

  // this is a relative position only, with that we can move all clouds at once
  SGVec3f relative_position;

  osg::ref_ptr<osg::Group> field_root;
  osg::ref_ptr<osg::Group> placed_root;
  osg::ref_ptr<osg::PositionAttitudeTransform> field_transform;
  osg::ref_ptr<osg::PositionAttitudeTransform> altitude_transform;
  osg::ref_ptr<osg::LOD> field_lod;

  osg::Vec3f old_pos;
  CloudHash cloud_hash;

  struct CloudFog : public simgear::Singleton<CloudFog>
  {
    CloudFog();
    osg::ref_ptr<osg::Fog> fog;
  };

  void removeCloudFromTree(osg::ref_ptr<osg::PositionAttitudeTransform> transform);
  void addCloudToTree(osg::ref_ptr<osg::PositionAttitudeTransform> transform, float lon, float lat, float alt, float x, float y);
  void addCloudToTree(osg::ref_ptr<osg::PositionAttitudeTransform> transform, SGGeod loc, float x, float y);
  void addCloudToTree(osg::ref_ptr<osg::PositionAttitudeTransform> transform, SGGeod loc);
  void applyVisRangeAndCoverage(void);

public:

	SGCloudField();
	~SGCloudField();

	void clear(void);
	bool isDefined3D(void);

	// add one cloud, data is not copied, ownership given
	void addCloud( SGVec3f& pos, osg::ref_ptr<simgear::EffectGeode> cloud);

        /**
	 * Add a new cloud with a given index at a specific point defined by lon/lat and an x/y offset
	 */
	bool addCloud(float lon, float lat, float alt, int index, osg::ref_ptr<simgear::EffectGeode> geode);
	bool addCloud(float lon, float lat, float alt, float x, float y, int index, osg::ref_ptr<simgear::EffectGeode> geode);

	// Cloud handling functions.
  bool deleteCloud(int identifier);
  bool repositionCloud(int identifier, float lon, float lat, float alt);
  bool repositionCloud(int identifier, float lon, float lat, float alt, float x, float y);


  /**
    * reposition the cloud layer at the specified origin and
    * orientation.
    * @param p position vector
    * @param up the local up vector
    * @param lon specifies a rotation about the Z axis
    * @param lat specifies a rotation about the new Y axis
    * @param dt the time elapsed since the last call
    * @param asl altitude of the layer
    * @param speed of cloud layer movement (due to wind)
    * @param direction of cloud layer movement (due to wind)
    */
  bool reposition( const SGVec3f& p, const SGVec3f& up,
            double lon, double lat, double dt, int asl, float speed, float direction);

  osg::Group* getNode() { return field_root.get(); }

  // visibility distance for clouds in meters
  static float CloudVis;

  static SGVec3f view_vec, view_X, view_Y;

  static float view_distance;
  static float impostor_distance;
  static float lod1_range;
  static float lod2_range;
  static bool use_impostors;
  static double timer_dt;
  static float fieldSize;
  static bool wrap;

  static bool getWrap(void) { return wrap; }
  static void setWrap(bool w) { wrap = w; }

  static float getImpostorDistance(void) { return impostor_distance; }
  static void setImpostorDistance(float d) { impostor_distance = d; }
  static float getLoD1Range(void) { return lod1_range; }
  static void setLoD1Range(int d) { lod1_range = d; }
  static float getLoD2Range(void) { return lod2_range; }
  static void setLoD2Range(int d) { lod2_range = d; }
  static void setUseImpostors(bool b) { use_impostors = b; }
  static bool getUseImpostors(void) { return use_impostors; }
  
  static float getVisRange(void) { return view_distance; }
  static void setVisRange(float d) { view_distance = d; }
  void applyVisAndLoDRange(void);

  static osg::Fog* getFog()
  {
          return CloudFog::instance()->fog.get();
  }
  static void updateFog(double visibility, const osg::Vec4f& color);
};

#endif // _CLOUDFIELD_HXX
