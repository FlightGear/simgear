/* -*-c++-*-
 *
 * Copyright (C) 2011 Stuart Buchanan
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */

#ifndef SG_BUILDING_BIN_HXX
#define SG_BUILDING_BIN_HXX

#include <math.h>

#include <vector>
#include <string>

#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Math>
#include <osg/MatrixTransform>
#include <osg/Matrix>
#include <osg/ShadeModel>
#include <osg/Material>
#include <osg/CullFace>


#include <simgear/scene/util/OsgMath.hxx>
#include <simgear/scene/material/mat.hxx>

#include <simgear/scene/util/QuadTreeBuilder.hxx>
#include <simgear/scene/util/RenderConstants.hxx>
#include <simgear/scene/util/StateAttributeFactory.hxx>
#include <simgear/structure/OSGUtils.hxx>

#define SG_BUILDING_QUAD_TREE_DEPTH 4
#define SG_BUILDING_FADE_OUT_LEVELS 4

using namespace osg;

namespace simgear
{
class SGBuildingBin {
public:

  // Number of buildings to auto-generate. Individual
  // building instances are taken from this set.
  static const unsigned int BUILDING_SET_SIZE = 200;

  static const unsigned int QUADS_PER_BUILDING = 12;
  static const unsigned int VERTICES_PER_BUILDING = 4 * QUADS_PER_BUILDING;
  static const unsigned int VERTICES_PER_BUILDING_SET = BUILDING_SET_SIZE * VERTICES_PER_BUILDING;

  enum BuildingType {
    SMALL = 0,
    MEDIUM,
    LARGE };

private:

  struct Building {
    Building(BuildingType t, float w, float d, float h, int f, bool pitch) :
      type(t),
      width(w),
      depth(d),
      height(h),
      floors(f),
      pitched(pitch),
      radius(std::max(d, 0.5f*w))
    { }

    BuildingType type;
    float width;
    float depth;
    float height;
    int floors;
    bool pitched;
    float radius;

    float getFootprint() {
      return radius;
    }
  };

  // The set of buildings that are instantiated
  typedef std::vector<Building> BuildingList;
  BuildingList smallBuildings;
  BuildingList mediumBuildings;
  BuildingList largeBuildings;

  std::string* material_name;
  std::string* texture;
  std::string* lightMap;

  // Fraction of buildings of this type
  float smallBuildingFraction;
  float mediumBuildingFraction;

  // The maximum radius of each building type
  float smallBuildingMaxRadius;
  float mediumBuildingMaxRadius;
  float largeBuildingMaxRadius;

  // The maximum depth of each building type
  float smallBuildingMaxDepth;
  float mediumBuildingMaxDepth;
  float largeBuildingMaxDepth;

  // Visibility range for buildings
  float buildingRange;

  // Shared geometries of the building set
  ref_ptr<Geometry> smallSharedGeometry;
  ref_ptr<Geometry> mediumSharedGeometry;
  ref_ptr<Geometry> largeSharedGeometry;

  struct BuildingInstance {
    BuildingInstance(SGVec3f p, float r, const BuildingList* bl, ref_ptr<Geometry> sg) :
      position(p),
      rotation(r),
      buildingList(bl),
      sharedGeometry(sg)
    { }

    BuildingInstance(SGVec3f p, BuildingInstance b) :
      position(p),
      rotation(b.rotation),
      buildingList(b.buildingList),
      sharedGeometry(b.sharedGeometry)
    { }

    SGVec3f position;
    float rotation;

    // References to allow the QuadTreeBuilder to work
    const BuildingList* buildingList;
    ref_ptr<Geometry> sharedGeometry;

    SGVec3f getPosition() { return position; }
    float getRotation() { return rotation; }

    float getDistSqr(SGVec3f p) {
      return distSqr(p, position);
    }

    const osg::Vec4f getColorValue() {
      return osg::Vec4f(toOsg(position), rotation);
    }
  };

  // Information for an instance of a building - position and orientation
  typedef std::vector<BuildingInstance> BuildingInstanceList;
  BuildingInstanceList smallBuildingLocations;
  BuildingInstanceList mediumBuildingLocations;
  BuildingInstanceList largeBuildingLocations;

public:

  SGBuildingBin(const SGMaterial *mat, bool useVBOs);

  ~SGBuildingBin() {
    smallBuildings.clear();
    mediumBuildings.clear();
    largeBuildings.clear();
    smallBuildingLocations.clear();
    mediumBuildingLocations.clear();
    largeBuildingLocations.clear();
  }

  void insert(SGVec3f p, float r, BuildingType type);
  int getNumBuildings();

  bool checkMinDist (SGVec3f p, float radius);

  std::string* getMaterialName() { return material_name; }

  BuildingType getBuildingType(float roll);

  float getBuildingMaxRadius(BuildingType);
  float getBuildingMaxDepth(BuildingType);

  // Helper classes for creating the quad tree
  struct MakeBuildingLeaf
  {
      MakeBuildingLeaf(float range, Effect* effect, bool fade) :
          _range(range), _effect(effect), _fade_out(fade) {}

      MakeBuildingLeaf(const MakeBuildingLeaf& rhs) :
          _range(rhs._range), _effect(rhs._effect), _fade_out(rhs._fade_out)
      {}

      LOD* operator() () const
      {
          LOD* result = new LOD;

          if (_fade_out) {
              // Create a series of LOD nodes so buidling cover decreases
              // gradually with distance from _range to 2*_range
              for (float i = 0.0; i < SG_BUILDING_FADE_OUT_LEVELS; i++)
              {
                  EffectGeode* geode = new EffectGeode;
                  geode->setEffect(_effect.get());
                  result->addChild(geode, 0, _range * (1.0 + i / (SG_BUILDING_FADE_OUT_LEVELS - 1.0)));
              }
          } else {
              // No fade-out, so all are visible for 2X range
              EffectGeode* geode = new EffectGeode;
              geode->setEffect(_effect.get());
              result->addChild(geode, 0, 2.0 * _range);
          }
          return result;
      }

      float _range;
      ref_ptr<Effect> _effect;
      bool _fade_out;
  };

  struct AddBuildingLeafObject
  {
      Geometry* createNewBuildingGeometryInstance(const BuildingInstance& building) const
      {
        Geometry* geom = simgear::clone(building.sharedGeometry.get(), CopyOp::SHALLOW_COPY);
        geom->setColorArray(new Vec4Array, Array::BIND_PER_VERTEX);
        geom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::QUADS));
        return geom;
      }

      void operator() (LOD* lod, const BuildingInstance& building) const
      {
          Geode* geode = static_cast<Geode*>(lod->getChild(int(building.position.x() * 10.0f) % lod->getNumChildren()));
          unsigned int numDrawables = geode->getNumDrawables();

          // Get the last geometry of to be added and check if there is space for
          // another building instance within it.  This is done by checking
          // if the number of Color values matches the number of vertices.
          // The color array is used to store the position of a particular
          // instance.
          Geometry* geom;

          if (numDrawables == 0) {
            // Create a new copy of the shared geometry to instantiate
            geom = createNewBuildingGeometryInstance(building);
            geode->addDrawable(geom);
          } else {
            geom = static_cast<Geometry*>(geode->getDrawable(numDrawables - 1));
          }

          // Check if this building is too close to any other others.
          DrawArrays* primSet  = static_cast<DrawArrays*>(geom->getPrimitiveSet(0));
          Vec4Array* posArray = static_cast<Vec4Array*>(geom->getColorArray());

          // Now check if this geometry is full.
          if (posArray->size() >= static_cast<Vec3Array*>(geom->getVertexArray())->size()) {
            // This particular geometry is full, so we generate another
            // by taking a shallow copy of the shared Geomety.
            geom = createNewBuildingGeometryInstance(building);
            geode->addDrawable(geom);
            posArray = static_cast<Vec4Array*>(geom->getColorArray());
            SG_LOG(SG_TERRAIN, SG_DEBUG, "Added new geometry to building geod: " << geode->getNumDrawables());
          }

          // We now have a geometry with space for this new building.
          // Set the position and rotation
          osg::Vec4f c = osg::Vec4f(toOsg(building.position), building.rotation);
          posArray->insert(posArray->end(), VERTICES_PER_BUILDING, c);
          size_t numVerts = posArray->size();
          primSet = static_cast<DrawArrays*>(geom->getPrimitiveSet(0));
          primSet->setCount(numVerts);
      }
  };

  struct GetBuildingCoord
  {
      Vec3 operator() (const BuildingInstance& building) const
      {
          return toOsg(building.position);
      }
  };

  typedef QuadTreeBuilder<LOD*, BuildingInstance, MakeBuildingLeaf, AddBuildingLeafObject,
                          GetBuildingCoord> BuildingGeometryQuadtree;

  struct BuildingInstanceTransformer
  {
      BuildingInstanceTransformer(Matrix& mat_) : mat(mat_) {}
      BuildingInstance operator()(const BuildingInstance& buildingInstance) const
      {
          Vec3 pos = toOsg(buildingInstance.position) * mat;
          return BuildingInstance(toSG(pos), buildingInstance);
      }
      Matrix mat;
  };

  ref_ptr<Group> createBuildingsGroup(Matrix transInv, const SGReaderWriterOptions* options);

};

// List of buildings
typedef std::list<SGBuildingBin*> SGBuildingBinList;


osg::Group* createRandomBuildings(SGBuildingBinList& buildinglist, const osg::Matrix& transform,
                         const SGReaderWriterOptions* options);
}
#endif
