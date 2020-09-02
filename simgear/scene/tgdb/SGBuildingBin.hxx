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
#include <osg/VertexAttribDivisor>

#include <simgear/scene/util/OsgMath.hxx>
#include <simgear/scene/material/mat.hxx>

#include <simgear/scene/material/Effect.hxx>
#include <simgear/scene/material/EffectGeode.hxx>

#include <simgear/scene/util/QuadTreeBuilder.hxx>
#include <simgear/scene/util/RenderConstants.hxx>
#include <simgear/scene/util/StateAttributeFactory.hxx>
#include <simgear/structure/OSGUtils.hxx>

#define SG_BUILDING_QUAD_TREE_DEPTH 2
#define SG_BUILDING_FADE_OUT_LEVELS 4

// these correspond to building.eff
const int BUILDING_POSITION_ATTR = 10;  // (x,y,z)
const int BUILDING_SCALE_ATTR = 11; // (width, depth, height)
const int BUILDING_ATTR1 = 12;
const int BUILDING_ATTR2 = 13;
const int BUILDING_ATTR3 = 14;
const int BUILDING_ATTR4 = 15;

using namespace osg;

namespace simgear
{

struct BuildingBoundingBoxCallback : public Drawable::ComputeBoundingBoxCallback
{
    BuildingBoundingBoxCallback() {}
    BuildingBoundingBoxCallback(const BuildingBoundingBoxCallback&, const CopyOp&) {}
    META_Object(simgear, BuildingBoundingBoxCallback);
    virtual BoundingBox computeBound(const Drawable& drawable) const
    {
        BoundingBox bb;
        const Geometry* geom = static_cast<const Geometry*>(&drawable);
        const Vec3Array* pos = static_cast<const Vec3Array*>(geom->getVertexAttribArray(BUILDING_POSITION_ATTR));

        for (unsigned int v=0; v<pos->size(); ++v) {
            Vec3 pt = (*pos)[v];
            bb.expandBy(pt);
        }
        return bb;
    }
};

class SGBuildingBin {
public:

  enum BuildingType {
    SMALL = 0,
    MEDIUM,
    LARGE };

    struct BuildingInstance {
      BuildingInstance(Vec3f p, float w, float d, float h, float ph, float r, Vec2f wt0, Vec2f rt0, Vec3f t1, Vec2f rs) :
        position(p),
        width(w),
        depth(d),
        height(h),
        pitch_height(ph),
        rotation(r),
        walltex0(wt0),
        rooftex0(rt0),
        tex1(t1),
        rooftop_scale(rs)
      { }

      BuildingInstance(Vec3f p, BuildingInstance b) :
        position(p),
        width(b.width),
        depth(b.depth),
        height(b.height),
        pitch_height(b.pitch_height),
        rotation(b.rotation),
        walltex0(b.walltex0),
        rooftex0(b.rooftex0),
        tex1(b.tex1),
        rooftop_scale(b.rooftop_scale)
      { }


      Vec3f position;
      float width;
      float depth;
      float height;
      float pitch_height;
      float rotation;

      Vec2f walltex0;
      Vec2f rooftex0;
      Vec3f tex1; // Texture gains for the front, roof and sides

      Vec2f rooftop_scale;

      // References to allow the QuadTreeBuilder to work
      //const BuildingList* buildingList;
      //ref_ptr<Geometry> sharedGeometry;

      Vec3f getPosition() { return position; }
      float getRotation() { return rotation; }

      float getDistSqr(Vec3f p) {
        return (p - position) * (p - position);
      }
    };

private:
    const SGSharedPtr<SGMaterial> _material;

    std::string _materialName;
    std::string _textureName;
    std::string _lightMapName;

    // Visibility range for buildings
    float buildingRange;


    // Information for an instance of a building - position and orientation
    typedef std::vector<BuildingInstance> BuildingInstanceList;
    BuildingInstanceList buildingLocations;

public:

  SGBuildingBin(const SGMaterial *mat, bool useVBOs);
  SGBuildingBin(const SGPath& absoluteFileName, const SGMaterial *mat, bool useVBOs);

  ~SGBuildingBin();

  // Generate a building specifying the exact position, dimensions and texture index.
  void insert(SGVec3f p,
              float r,
              BuildingType buildingtype,
              float width,
              float depth,
              float height,
              float pitch_height,
              int floors,
              int roof_shape,
              int roof_orientation,
              int wall_tex_index,
              int roof_tex_index);

  // Generate a building of a given type at a specified position, using the random building material definition to determine the dimensions and texture index.
  void insert(SGVec3f p, float r, BuildingType type);
  int getNumBuildings();

  bool checkMinDist (SGVec3f p, float radius);

  const std::string& getMaterialName() const { return _materialName; }

  BuildingType getBuildingType(float roll);
  float getBuildingMaxRadius(BuildingType);
  float getBuildingMaxDepth(BuildingType);

  ref_ptr<Group> createBuildingsGroup(Matrix transInv, const SGReaderWriterOptions* options);

};

// List of buildings
typedef std::list<SGBuildingBin*> SGBuildingBinList;


osg::Group* createRandomBuildings(SGBuildingBinList& buildinglist, const osg::Matrix& transform,
                         const SGReaderWriterOptions* options);
}
#endif
