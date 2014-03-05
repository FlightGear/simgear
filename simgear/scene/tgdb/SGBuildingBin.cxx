/* -*-c++-*-
 *
 * Copyright (C) 2012 Stuart Buchanan
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

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include <algorithm>
#include <vector>
#include <string>
#include <map>

#include <boost/foreach.hpp>
#include <boost/tuple/tuple_comparison.hpp>

#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Math>
#include <osg/MatrixTransform>
#include <osg/Matrix>
#include <osg/ShadeModel>
#include <osg/Material>
#include <osg/CullFace>

#include <osgDB/ReadFile>
#include <osgDB/FileUtils>

#include <simgear/debug/logstream.hxx>
#include <simgear/math/SGLimits.hxx>
#include <simgear/math/SGMisc.hxx>
#include <simgear/math/sg_random.h>
#include <simgear/misc/sg_path.hxx>
#include <simgear/scene/material/Effect.hxx>
#include <simgear/scene/material/EffectGeode.hxx>
#include <simgear/scene/model/model.hxx>
#include <simgear/props/props.hxx>

#include "ShaderGeometry.hxx"
#include "SGBuildingBin.hxx"


using namespace osg;

namespace simgear
{

typedef std::map<std::string, osg::observer_ptr<osg::StateSet> > BuildingStateSetMap;
static BuildingStateSetMap statesetmap;

typedef std::map<std::string, osg::observer_ptr<Effect> > EffectMap;
static EffectMap buildingEffectMap;

// Building instance scheme:
// vertex - local position of vertices, with 0,0,0 being the center front.
// fog coord - rotation
// color - xyz of tree quad origin, replicated 4 times.

struct BuildingBoundingBoxCallback : public Drawable::ComputeBoundingBoxCallback
{
    BuildingBoundingBoxCallback() {}
    BuildingBoundingBoxCallback(const BuildingBoundingBoxCallback&, const CopyOp&) {}
    META_Object(simgear, BuildingBoundingBoxCallback);
    virtual BoundingBox computeBound(const Drawable&) const;
};

BoundingBox
BuildingBoundingBoxCallback::computeBound(const Drawable& drawable) const
{
    BoundingBox bb;
    const Geometry* geom = static_cast<const Geometry*>(&drawable);
    const Vec3Array* v = static_cast<const Vec3Array*>(geom->getVertexArray());
    const Vec4Array* pos = static_cast<const Vec4Array*>(geom->getColorArray());

    Geometry::PrimitiveSetList primSets = geom->getPrimitiveSetList();
    for (Geometry::PrimitiveSetList::const_iterator psitr = primSets.begin(), psend = primSets.end();
         psitr != psend;
         ++psitr) {
        DrawArrays* da = static_cast<DrawArrays*>(psitr->get());
        GLint psFirst = da->getFirst();
        GLint psEndVert = psFirst + da->getCount();
        for (GLint i = psFirst;i < psEndVert; ++i) {
            Vec3 pt = (*v)[i];
            Matrixd trnsfrm = Matrixd::rotate(- M_PI * 2 * (*pos)[i].a(), Vec3(0.0f, 0.0f, 1.0f));
            pt = pt * trnsfrm;
            pt += Vec3((*pos)[i].x(), (*pos)[i].y(), (*pos)[i].z());
            bb.expandBy(pt);
        }
    }
    return bb;
}

  // Set up the building set based on the material definitions
  SGBuildingBin::SGBuildingBin(const SGMaterial *mat, bool useVBOs) {

    material_name = new std::string(mat->get_names()[0]);
    SG_LOG(SG_TERRAIN, SG_DEBUG, "Building material " << material_name);
    texture = new std::string(mat->get_building_texture());
    lightMap = new std::string(mat->get_building_lightmap());
    SG_LOG(SG_TERRAIN, SG_DEBUG, "Building texture " << texture);

    // Generate a random seed for the building generation.
    mt seed;
    mt_init(&seed, unsigned(123));

    smallSharedGeometry = new osg::Geometry();
    mediumSharedGeometry = new osg::Geometry();
    largeSharedGeometry = new osg::Geometry();

    smallBuildingMaxRadius = std::max(mat->get_building_small_max_depth() * 0.5, mat->get_building_small_max_width() * 0.5);
    mediumBuildingMaxRadius = std::max(mat->get_building_medium_max_depth() * 0.5, mat->get_building_medium_max_width() * 0.5);
    largeBuildingMaxRadius = std::max(mat->get_building_large_max_depth() * 0.5, mat->get_building_large_max_width() * 0.5);

    smallBuildingMaxDepth = mat->get_building_small_max_depth();
    mediumBuildingMaxDepth = mat->get_building_medium_max_depth();
    largeBuildingMaxDepth = mat->get_building_large_max_depth();

    smallBuildingFraction = mat->get_building_small_fraction();
    mediumBuildingFraction = mat->get_building_medium_fraction();

    buildingRange = mat->get_building_range();

    SG_LOG(SG_TERRAIN, SG_DEBUG, "Building fractions " << smallBuildingFraction << " " << mediumBuildingFraction);


    // TODO: Reverse this - otherwise we never get any large buildings!
    BuildingType types[] = { SGBuildingBin::SMALL, SGBuildingBin::MEDIUM, SGBuildingBin::LARGE };
    BuildingList lists[] = { SGBuildingBin::smallBuildings, SGBuildingBin::mediumBuildings, SGBuildingBin::largeBuildings };
    ref_ptr<Geometry> geometries[] = { smallSharedGeometry, mediumSharedGeometry, largeSharedGeometry };

    for (int bt=0; bt < 3; bt++) {
      SGBuildingBin::BuildingType buildingtype = types[bt];
      ref_ptr<Geometry> sharedGeometry = geometries[bt];
      BuildingList buildings = lists[bt];

      osg::ref_ptr<osg::Vec3Array> v = new osg::Vec3Array;
      osg::ref_ptr<osg::Vec2Array> t = new osg::Vec2Array;
      osg::ref_ptr<osg::Vec3Array> n = new osg::Vec3Array;

      v->reserve(BUILDING_SET_SIZE * VERTICES_PER_BUILDING);
      t->reserve(BUILDING_SET_SIZE * VERTICES_PER_BUILDING);
      n->reserve(BUILDING_SET_SIZE * VERTICES_PER_BUILDING);

      sharedGeometry->setFogCoordBinding(osg::Geometry::BIND_PER_VERTEX);
      sharedGeometry->setComputeBoundingBoxCallback(new BuildingBoundingBoxCallback);
      sharedGeometry->setUseDisplayList(false);
      sharedGeometry->setDataVariance(osg::Object::STATIC);
      if (useVBOs) {
          sharedGeometry->setUseVertexBufferObjects(true);
      }
      
      for (unsigned int j = 0; j < BUILDING_SET_SIZE; j++) {
        float width;
        float depth;
        int floors;
        float height;
        bool pitched;

        if (buildingtype == SGBuildingBin::SMALL) {
          // Small building
          width = mat->get_building_small_min_width() + mt_rand(&seed) * mt_rand(&seed) * (mat->get_building_small_max_width() - mat->get_building_small_min_width());
          depth = mat->get_building_small_min_depth() + mt_rand(&seed) * mt_rand(&seed) * (mat->get_building_small_max_depth() - mat->get_building_small_min_depth());
          floors = SGMisc<double>::round(mat->get_building_small_min_floors() + mt_rand(&seed) * (mat->get_building_small_max_floors() - mat->get_building_small_min_floors()));
          height = floors * (2.8 + mt_rand(&seed));

          // Small buildings are never deeper than they are wide.
          if (depth > width) { depth = width; }

          pitched = (mt_rand(&seed) < mat->get_building_small_pitch());
        } else if (buildingtype == SGBuildingBin::MEDIUM) {
          width = mat->get_building_medium_min_width() + mt_rand(&seed) * mt_rand(&seed) * (mat->get_building_medium_max_width() - mat->get_building_medium_min_width());
          depth = mat->get_building_medium_min_depth() + mt_rand(&seed) * mt_rand(&seed) * (mat->get_building_medium_max_depth() - mat->get_building_medium_min_depth());
          floors = SGMisc<double>::round(mat->get_building_medium_min_floors() + mt_rand(&seed) * (mat->get_building_medium_max_floors() - mat->get_building_medium_min_floors()));
          height = floors * (2.8 + mt_rand(&seed));

          while ((height > width) && (floors > mat->get_building_medium_min_floors())) {
            // Ensure that medium buildings aren't taller than they are wide
            floors--;
            height = floors * (2.8 + mt_rand(&seed));
          }

          pitched = (mt_rand(&seed) < mat->get_building_medium_pitch());
        } else {
          width = mat->get_building_large_min_width() + mt_rand(&seed) * (mat->get_building_large_max_width() - mat->get_building_large_min_width());
          depth = mat->get_building_large_min_depth() + mt_rand(&seed) * (mat->get_building_large_max_depth() - mat->get_building_large_min_depth());
          floors = SGMisc<double>::round(mat->get_building_large_min_floors() + mt_rand(&seed) * (mat->get_building_large_max_floors() - mat->get_building_large_min_floors()));
          height = floors * (2.8 + mt_rand(&seed));
          pitched = (mt_rand(&seed) < mat->get_building_large_pitch());
        }

        Building building = Building(buildingtype,
                                    width,
                                    depth,
                                    height,
                                    floors,
                                    pitched);

        buildings.push_back(building);

        // Now create an OSG Geometry based on the Building
        float cw = 0.5f * building.width;
        float cd = building.depth;
        float ch = building.height;

        // 0,0,0 is the bottom center of the front
        // face, e.g. where the front door would be

        // BASEMENT
        // This exteds 10m below the main section
        // Front face
        v->push_back( osg::Vec3( 0, -cw, -10) ); // bottom right
        v->push_back( osg::Vec3( 0,  cw, -10) ); // bottom left
        v->push_back( osg::Vec3( 0,  cw,   0) ); // top left
        v->push_back( osg::Vec3( 0, -cw,   0) ); // top right

        for (int i=0; i<4; ++i)
          n->push_back( osg::Vec3(1, 0, 0) ); // normal

        // Left face
        v->push_back( osg::Vec3( -cd, -cw, -10) ); // bottom right
        v->push_back( osg::Vec3(   0, -cw, -10) ); // bottom left
        v->push_back( osg::Vec3(   0, -cw,   0) ); // top left
        v->push_back( osg::Vec3( -cd, -cw,   0) ); // top right

        for (int i=0; i<4; ++i)
          n->push_back( osg::Vec3(0, -1, 0) ); // normal

        // Back face
        v->push_back( osg::Vec3( -cd,  cw, -10) ); // bottom right
        v->push_back( osg::Vec3( -cd, -cw, -10) ); // bottom left
        v->push_back( osg::Vec3( -cd, -cw,   0) ); // top left
        v->push_back( osg::Vec3( -cd,  cw,   0) ); // top right

        for (int i=0; i<4; ++i)
          n->push_back( osg::Vec3(-1, 0, 0) ); // normal

        // Right face
        v->push_back( osg::Vec3(   0, cw, -10) ); // bottom right
        v->push_back( osg::Vec3( -cd, cw, -10) ); // bottom left
        v->push_back( osg::Vec3( -cd, cw,   0) ); // top left
        v->push_back( osg::Vec3(   0, cw,   0) ); // top right

        for (int i=0; i<4; ++i)
          n->push_back( osg::Vec3(0, 1, 0) ); // normal

        // MAIN BODY
        // Front face
        v->push_back( osg::Vec3( 0, -cw,  0) ); // bottom right
        v->push_back( osg::Vec3( 0,  cw,  0) ); // bottom left
        v->push_back( osg::Vec3( 0,  cw, ch) ); // top left
        v->push_back( osg::Vec3( 0, -cw, ch) ); // top right

        for (int i=0; i<4; ++i)
          n->push_back( osg::Vec3(1, 0, 0) ); // normal

        // Left face
        v->push_back( osg::Vec3( -cd, -cw,  0) ); // bottom right
        v->push_back( osg::Vec3(   0, -cw,  0) ); // bottom left
        v->push_back( osg::Vec3(   0, -cw, ch) ); // top left
        v->push_back( osg::Vec3( -cd, -cw, ch) ); // top right

        for (int i=0; i<4; ++i)
          n->push_back( osg::Vec3(0, -1, 0) ); // normal

        // Back face
        v->push_back( osg::Vec3( -cd,  cw,  0) ); // bottom right
        v->push_back( osg::Vec3( -cd, -cw,  0) ); // bottom left
        v->push_back( osg::Vec3( -cd, -cw, ch) ); // top left
        v->push_back( osg::Vec3( -cd,  cw, ch) ); // top right

        for (int i=0; i<4; ++i)
          n->push_back( osg::Vec3(-1, 0, 0) ); // normal

        // Right face
        v->push_back( osg::Vec3(   0, cw,  0) ); // bottom right
        v->push_back( osg::Vec3( -cd, cw,  0) ); // bottom left
        v->push_back( osg::Vec3( -cd, cw, ch) ); // top left
        v->push_back( osg::Vec3(   0, cw, ch) ); // top right

        for (int i=0; i<4; ++i)
          n->push_back( osg::Vec3(0, 1, 0) ); // normal

        // ROOF
        if (building.pitched) {

          // Front pitched roof
          v->push_back( osg::Vec3(    0, -cw,   ch) ); // bottom right
          v->push_back( osg::Vec3(    0,  cw,   ch) ); // bottom left
          v->push_back( osg::Vec3(-0.5*cd,  cw, ch+3) ); // top left
          v->push_back( osg::Vec3(-0.5*cd, -cw, ch+3) ); // top right

          for (int i=0; i<4; ++i)
            n->push_back( osg::Vec3(0.707, 0, 0.707) ); // normal

          // Left pitched roof
          v->push_back( osg::Vec3(    -cd, -cw,   ch) ); // bottom right
          v->push_back( osg::Vec3(      0, -cw,   ch) ); // bottom left
          v->push_back( osg::Vec3(-0.5*cd, -cw, ch+3) ); // top left
          v->push_back( osg::Vec3(-0.5*cd, -cw, ch+3) ); // top right

          for (int i=0; i<4; ++i)
            n->push_back( osg::Vec3(0, -1, 0) ); // normal

          // Back pitched roof
          v->push_back( osg::Vec3(    -cd,  cw,   ch) ); // bottom right
          v->push_back( osg::Vec3(    -cd, -cw,   ch) ); // bottom left
          v->push_back( osg::Vec3(-0.5*cd, -cw, ch+3) ); // top left
          v->push_back( osg::Vec3(-0.5*cd,  cw, ch+3) ); // top right

          for (int i=0; i<4; ++i)
            n->push_back( osg::Vec3(-0.707, 0, 0.707) ); // normal

          // Right pitched roof
          v->push_back( osg::Vec3(      0, cw,   ch) ); // bottom right
          v->push_back( osg::Vec3(    -cd, cw,   ch) ); // bottom left
          v->push_back( osg::Vec3(-0.5*cd, cw, ch+3) ); // top left
          v->push_back( osg::Vec3(-0.5*cd, cw, ch+3) ); // top right

          for (int i=0; i<4; ++i)
            n->push_back( osg::Vec3(0, 1, 0) ); // normal
        } else {
          // If the roof isn't pitched, we still generate the
          // vertices for simplicity later.

          // Top of the roof
          v->push_back( osg::Vec3(  0, -cw, ch) ); // bottom right
          v->push_back( osg::Vec3(  0,  cw, ch) ); // bottom left
          v->push_back( osg::Vec3(-cd,  cw, ch) ); // top left
          v->push_back( osg::Vec3(-cd, -cw, ch) ); // top right

          for (int i=0; i<4; ++i)
            n->push_back( osg::Vec3(0, 0, 1) ); // normal

          // Left non-pitched roof
          v->push_back( osg::Vec3( -cd, -cw, ch) ); // bottom right
          v->push_back( osg::Vec3(   0, -cw, ch) ); // bottom left
          v->push_back( osg::Vec3(   0, -cw, ch) ); // top left
          v->push_back( osg::Vec3( -cd, -cw, ch) ); // top right

          for (int i=0; i<4; ++i)
            n->push_back( osg::Vec3(0, -1, 0) ); // normal

          // Back pitched roof
          v->push_back( osg::Vec3(-cd,  cw, ch) ); // bottom right
          v->push_back( osg::Vec3(-cd, -cw, ch) ); // bottom left
          v->push_back( osg::Vec3(-cd, -cw, ch) ); // top left
          v->push_back( osg::Vec3(-cd,  cw, ch) ); // top right

          for (int i=0; i<4; ++i)
            n->push_back( osg::Vec3(1, 0, 0) ); // normal

          // Right pitched roof
          v->push_back( osg::Vec3(  0, cw, ch) ); // bottom right
          v->push_back( osg::Vec3(-cd, cw, ch) ); // bottom left
          v->push_back( osg::Vec3(-cd, cw, ch) ); // top left
          v->push_back( osg::Vec3(  0, cw, ch) ); // top right

          for (int i=0; i<4; ++i)
            n->push_back( osg::Vec3(0, 1, 0) ); // normal
        }

        // The 1024x1024 texture is split into 32x16 blocks.
        // For a small building, each block is 6m wide and 3m high.
        // For a medium building, each block is 10m wide and 3m high.
        // For a large building, each block is 20m wide and 3m high

        if (building.type == SGBuildingBin::SMALL) {
          // Small buildings are represented on the bottom 5 rows of 3 floors
          int row = ((int) (mt_rand(&seed) * 1000)) % 5;
          float base_y = (float) row * 16.0 * 3.0 / 1024.0;
          float top_y = base_y + 16.0 * (float) building.floors / 1024.0;
          float left_x = 32.0 / 1024.0 * round((float) building.width / 6.0f);
          float right_x = 0.0f;
          float front_x = 384.0/1024.0;
          float back_x = 384.0/1024.0 + 32.0 / 1024.0 * round((float) building.depth/ 6.0f);

          // BASEMENT - uses the baseline texture
          for (unsigned int i = 0; i < 16; i++) {
            t->push_back( osg::Vec2( left_x, base_y) );
          }
          // MAIN BODY
          // Front
          t->push_back( osg::Vec2( right_x, base_y) ); // bottom right
          t->push_back( osg::Vec2( left_x,  base_y) ); // bottom left
          t->push_back( osg::Vec2( left_x,  top_y ) ); // top left
          t->push_back( osg::Vec2( right_x, top_y ) ); // top right

          // Left
          t->push_back( osg::Vec2( front_x, base_y) ); // bottom right
          t->push_back( osg::Vec2( back_x,  base_y) ); // bottom left
          t->push_back( osg::Vec2( back_x,  top_y ) ); // top left
          t->push_back( osg::Vec2( front_x, top_y ) ); // top right

          // Back (same as front for the moment)
          t->push_back( osg::Vec2( right_x, base_y) ); // bottom right
          t->push_back( osg::Vec2( left_x,  base_y) ); // bottom left
          t->push_back( osg::Vec2( left_x,  top_y ) ); // top left
          t->push_back( osg::Vec2( right_x, top_y ) ); // top right

          // Right (same as left for the moment)
          t->push_back( osg::Vec2( front_x, base_y) ); // bottom right
          t->push_back( osg::Vec2( back_x,  base_y) ); // bottom left
          t->push_back( osg::Vec2( back_x,  top_y ) ); // top left
          t->push_back( osg::Vec2( front_x, top_y ) ); // top right

          // ROOF
          if (building.pitched) {
            // Use the entire height of the roof texture
            top_y = base_y + 16.0 * 3.0 / 1024.0;
            left_x = 512/1024.0 + 32.0 / 1024.0 * round(building.width / 6.0f);
            right_x = 512/1024.0;
            front_x = 480.0/1024.0;
            back_x = 512.0/1024.0;

            // Front
            t->push_back( osg::Vec2( right_x, base_y) ); // bottom right
            t->push_back( osg::Vec2( left_x,  base_y) ); // bottom left
            t->push_back( osg::Vec2( left_x,  top_y ) ); // top left
            t->push_back( osg::Vec2( right_x, top_y ) ); // top right

            // Left
            t->push_back( osg::Vec2( front_x, base_y) ); // bottom right
            t->push_back( osg::Vec2( back_x,  base_y) ); // bottom left
            t->push_back( osg::Vec2( back_x,  top_y ) ); // top left
            t->push_back( osg::Vec2( front_x, top_y ) ); // top right

            // Back (same as front for the moment)
            t->push_back( osg::Vec2( right_x, base_y) ); // bottom right
            t->push_back( osg::Vec2( left_x,  base_y) ); // bottom left
            t->push_back( osg::Vec2( left_x,  top_y ) ); // top left
            t->push_back( osg::Vec2( right_x, top_y ) ); // top right

            // Right (same as left for the moment)
            t->push_back( osg::Vec2( front_x, base_y) ); // bottom right
            t->push_back( osg::Vec2( back_x,  base_y) ); // bottom left
            t->push_back( osg::Vec2( back_x,  top_y ) ); // top left
            t->push_back( osg::Vec2( front_x, top_y ) ); // top right
          } else {
            // Flat roof
            left_x = 640.0/1024.0;
            right_x = 512.0/1024.0;
            // Use the entire height of the roof texture
            top_y = base_y + 16.0 * 3.0 / 1024.0;

            // Flat roofs still have 4 surfaces, so we need to set the textures
            for (int i=0; i<4; ++i) {
              t->push_back( osg::Vec2( right_x, base_y) ); // bottom right
              t->push_back( osg::Vec2( left_x,  base_y) ); // bottom left
              t->push_back( osg::Vec2( left_x,  top_y ) ); // top left
              t->push_back( osg::Vec2( right_x, top_y ) ); // top right
            }
          }

        }

        if (building.type == SGBuildingBin::MEDIUM)
        {
          int column = ((int) (mt_rand(&seed) * 1000)) % 5;
          float base_y = 288 / 1024.0;
          float top_y = base_y + 16.0 * (float) building.floors / 1024.0;
          float left_x = column * 192.0 /1024.0 + 32.0 / 1024.0 * round((float) building.width / 10.0f);
          float right_x = column * 192.0 /1024.0;

          // BASEMENT - uses the baseline texture
          for (unsigned int i = 0; i < 16; i++) {
            t->push_back( osg::Vec2( left_x, base_y) );
          }

          // MAIN BODY
          // Front
          t->push_back( osg::Vec2( right_x, base_y) ); // bottom right
          t->push_back( osg::Vec2( left_x,  base_y) ); // bottom left
          t->push_back( osg::Vec2( left_x,  top_y ) ); // top left
          t->push_back( osg::Vec2( right_x, top_y ) ); // top right

          // Left
          t->push_back( osg::Vec2( right_x, base_y) ); // bottom right
          t->push_back( osg::Vec2( left_x,  base_y) ); // bottom left
          t->push_back( osg::Vec2( left_x,  top_y ) ); // top left
          t->push_back( osg::Vec2( right_x, top_y ) ); // top right

          // Back (same as front for the moment)
          t->push_back( osg::Vec2( right_x, base_y) ); // bottom right
          t->push_back( osg::Vec2( left_x,  base_y) ); // bottom left
          t->push_back( osg::Vec2( left_x,  top_y ) ); // top left
          t->push_back( osg::Vec2( right_x, top_y ) ); // top right

          // Right (same as left for the moment)
          t->push_back( osg::Vec2( right_x, base_y) ); // bottom right
          t->push_back( osg::Vec2( left_x,  base_y) ); // bottom left
          t->push_back( osg::Vec2( left_x,  top_y ) ); // top left
          t->push_back( osg::Vec2( right_x, top_y ) ); // top right

          // ROOF
          if (building.pitched) {
            base_y = 288.0/1024.0;
            top_y = 576.0/1024.0;
            left_x = 960.0/1024.0;
            right_x = 1.0;

            // Front
            t->push_back( osg::Vec2( right_x, base_y) ); // bottom right
            t->push_back( osg::Vec2( left_x,  base_y) ); // bottom left
            t->push_back( osg::Vec2( left_x,  top_y ) ); // top left
            t->push_back( osg::Vec2( right_x, top_y ) ); // top right

            // Left
            t->push_back( osg::Vec2( right_x, base_y) ); // bottom right
            t->push_back( osg::Vec2( left_x,  base_y) ); // bottom left
            t->push_back( osg::Vec2( left_x,  top_y ) ); // top left
            t->push_back( osg::Vec2( right_x, top_y ) ); // top right

            // Back (same as front for the moment)
            t->push_back( osg::Vec2( right_x, base_y) ); // bottom right
            t->push_back( osg::Vec2( left_x,  base_y) ); // bottom left
            t->push_back( osg::Vec2( left_x,  top_y ) ); // top left
            t->push_back( osg::Vec2( right_x, top_y ) ); // top right

            // Right (same as left for the moment)
            t->push_back( osg::Vec2( right_x, base_y) ); // bottom right
            t->push_back( osg::Vec2( left_x,  base_y) ); // bottom left
            t->push_back( osg::Vec2( left_x,  top_y ) ); // top left
            t->push_back( osg::Vec2( right_x, top_y ) ); // top right
          } else {
            // Flat roof
            base_y = 416/1024.0;
            top_y = 576.0/1024.0;
            left_x = column * 192.0 /1024.0;
            right_x = (column + 1)* 192.0 /1024.0;

            // Flat roofs still have 4 surfaces
            for (int i=0; i<4; ++i) {
              t->push_back( osg::Vec2( right_x, base_y) ); // bottom right
              t->push_back( osg::Vec2( left_x,  base_y) ); // bottom left
              t->push_back( osg::Vec2( left_x,  top_y ) ); // top left
              t->push_back( osg::Vec2( right_x, top_y ) ); // top right
            }
          }
        }

        if (building.type == SGBuildingBin::LARGE)
        {
          int column = ((int) (mt_rand(&seed) * 1000)) % 8;
          float base_y = 576 / 1024.0;
          float top_y = base_y + 16.0 * (float) building.floors / 1024.0;
          float left_x = column * 128.0 /1024.0 + 32.0 / 1024.0 * round((float) building.width / 20.0f);
          float right_x = column * 128.0 /1024.0;

          // BASEMENT - uses the baseline texture
          for (unsigned int i = 0; i < 16; i++) {
            t->push_back( osg::Vec2( left_x, base_y) );
          }

          // MAIN BODY
          // Front
          t->push_back( osg::Vec2( right_x, base_y) ); // bottom right
          t->push_back( osg::Vec2( left_x,  base_y) ); // bottom left
          t->push_back( osg::Vec2( left_x,  top_y ) ); // top left
          t->push_back( osg::Vec2( right_x, top_y ) ); // top right

          // Left
          t->push_back( osg::Vec2( right_x, base_y) ); // bottom right
          t->push_back( osg::Vec2( left_x,  base_y) ); // bottom left
          t->push_back( osg::Vec2( left_x,  top_y ) ); // top left
          t->push_back( osg::Vec2( right_x, top_y ) ); // top right

          // Back (same as front for the moment)
          t->push_back( osg::Vec2( right_x, base_y) ); // bottom right
          t->push_back( osg::Vec2( left_x,  base_y) ); // bottom left
          t->push_back( osg::Vec2( left_x,  top_y ) ); // top left
          t->push_back( osg::Vec2( right_x, top_y ) ); // top right

          // Right (same as left for the moment)
          t->push_back( osg::Vec2( right_x, base_y) ); // bottom right
          t->push_back( osg::Vec2( left_x,  base_y) ); // bottom left
          t->push_back( osg::Vec2( left_x,  top_y ) ); // top left
          t->push_back( osg::Vec2( right_x, top_y ) ); // top right

          // ROOF
          if (building.pitched) {
            base_y = 896/1024.0;
            top_y = 1.0;
            // Front
            t->push_back( osg::Vec2( right_x, base_y) ); // bottom right
            t->push_back( osg::Vec2( left_x,  base_y) ); // bottom left
            t->push_back( osg::Vec2( left_x,  top_y ) ); // top left
            t->push_back( osg::Vec2( right_x, top_y ) ); // top right

            // Left
            t->push_back( osg::Vec2( right_x, base_y) ); // bottom right
            t->push_back( osg::Vec2( left_x,  base_y) ); // bottom left
            t->push_back( osg::Vec2( left_x,  top_y ) ); // top left
            t->push_back( osg::Vec2( right_x, top_y ) ); // top right

            // Back (same as front for the moment)
            t->push_back( osg::Vec2( right_x, base_y) ); // bottom right
            t->push_back( osg::Vec2( left_x,  base_y) ); // bottom left
            t->push_back( osg::Vec2( left_x,  top_y ) ); // top left
            t->push_back( osg::Vec2( right_x, top_y ) ); // top right

            // Right (same as left for the moment)
            t->push_back( osg::Vec2( right_x, base_y) ); // bottom right
            t->push_back( osg::Vec2( left_x,  base_y) ); // bottom left
            t->push_back( osg::Vec2( left_x,  top_y ) ); // top left
            t->push_back( osg::Vec2( right_x, top_y ) ); // top right
          } else {
            // Flat roof
            base_y = 896/1024.0;
            top_y = 1.0;

            // Flat roofs still have 4 surfaces
            for (int i=0; i<4; ++i) {
              t->push_back( osg::Vec2( right_x, base_y) ); // bottom right
              t->push_back( osg::Vec2( left_x,  base_y) ); // bottom left
              t->push_back( osg::Vec2( left_x,  top_y ) ); // top left
              t->push_back( osg::Vec2( right_x, top_y ) ); // top right
            }
          }
        }
      }

      // Set the vertex, texture and normals.  Colors will be set per-instance
      // later.
      sharedGeometry->setVertexArray(v);
      sharedGeometry->setTexCoordArray(0, t, Array::BIND_PER_VERTEX);
      sharedGeometry->setNormalArray(n, Array::BIND_PER_VERTEX);
    }
  }

  void SGBuildingBin::insert(SGVec3f p, float r, BuildingType type) {

    if (type == SGBuildingBin::SMALL) {
      smallBuildingLocations.push_back(BuildingInstance(p, r, &smallBuildings, smallSharedGeometry));
    }

    if (type == SGBuildingBin::MEDIUM) {
      mediumBuildingLocations.push_back(BuildingInstance(p, r, &mediumBuildings, mediumSharedGeometry));
    }

    if (type == SGBuildingBin::LARGE) {
      largeBuildingLocations.push_back(BuildingInstance(p, r, &largeBuildings, largeSharedGeometry));
    }
  }

  int SGBuildingBin::getNumBuildings() {
    return smallBuildingLocations.size() + mediumBuildingLocations.size() + largeBuildingLocations.size();
  }

  bool SGBuildingBin::checkMinDist (SGVec3f p, float radius) {
    BuildingInstanceList::iterator iter;

    float r = (radius + smallBuildingMaxRadius) * (radius + smallBuildingMaxRadius);
    for (iter = smallBuildingLocations.begin(); iter != smallBuildingLocations.end(); ++iter) {
      if (iter->getDistSqr(p) < r) {
        return false;
      }
    }

    r = (radius + mediumBuildingMaxRadius) * (radius + mediumBuildingMaxRadius);
    for (iter = mediumBuildingLocations.begin(); iter != mediumBuildingLocations.end(); ++iter) {
      if (iter->getDistSqr(p) < r) {
        return false;
      }
    }

    r = (radius + largeBuildingMaxRadius) * (radius + largeBuildingMaxRadius);
    for (iter = largeBuildingLocations.begin(); iter != largeBuildingLocations.end(); ++iter) {
      if (iter->getDistSqr(p) < r) {
        return false;
      }
    }

    return true;
  }

  SGBuildingBin::BuildingType SGBuildingBin::getBuildingType(float roll) {

    if (roll < smallBuildingFraction) {
      return SGBuildingBin::SMALL;
    }

    if (roll < (smallBuildingFraction + mediumBuildingFraction)) {
      return SGBuildingBin::MEDIUM;
    }

    return SGBuildingBin::LARGE;
  }

  float SGBuildingBin::getBuildingMaxRadius(BuildingType type) {

    if (type == SGBuildingBin::SMALL) return smallBuildingMaxRadius;
    if (type == SGBuildingBin::MEDIUM) return mediumBuildingMaxRadius;
    if (type == SGBuildingBin::LARGE) return largeBuildingMaxRadius;

    return 0;
  }

  float SGBuildingBin::getBuildingMaxDepth(BuildingType type) {

    if (type == SGBuildingBin::SMALL) return smallBuildingMaxDepth;
    if (type == SGBuildingBin::MEDIUM) return mediumBuildingMaxDepth;
    if (type == SGBuildingBin::LARGE) return largeBuildingMaxDepth;

    return 0;
  }

  ref_ptr<Group> SGBuildingBin::createBuildingsGroup(Matrix transInv, const SGReaderWriterOptions* options)
  {
    ref_ptr<Effect> effect;
    EffectMap::iterator iter = buildingEffectMap.find(*texture);

    if ((iter == buildingEffectMap.end())||
        (!iter->second.lock(effect)))
    {
      SGPropertyNode_ptr effectProp = new SGPropertyNode;
      makeChild(effectProp, "inherits-from")->setStringValue("Effects/building");
      SGPropertyNode* params = makeChild(effectProp, "parameters");
      // Main texture - n=0
      params->getChild("texture", 0, true)->getChild("image", 0, true)
          ->setStringValue(*texture);

      // Light map - n=3
      params->getChild("texture", 3, true)->getChild("image", 0, true)
          ->setStringValue(*lightMap);

      effect = makeEffect(effectProp, true, options);
      if (iter == buildingEffectMap.end())
          buildingEffectMap.insert(EffectMap::value_type(*texture, effect));
      else
          iter->second = effect; // update existing, but empty observer
    }

    ref_ptr<Group> group = new osg::Group();

    // Now, create a quadbuilding for the buildings.

    BuildingInstanceList locs[] = { smallBuildingLocations,
                                    SGBuildingBin::mediumBuildingLocations,
                                    SGBuildingBin::largeBuildingLocations };

    for (int i = 0; i < 3; i++)
    {
      // Create a quad tree.  Only small and medium buildings are faded out.
      BuildingGeometryQuadtree
          quadbuilding(GetBuildingCoord(), AddBuildingLeafObject(),
                   SG_BUILDING_QUAD_TREE_DEPTH,
                   MakeBuildingLeaf(buildingRange, effect, (i != 2)));

      // Transform building positions from the "geocentric" positions we
      // get from the scenery polys into the local Z-up coordinate
      // system.
      std::vector<BuildingInstance> rotatedBuildings;
      rotatedBuildings.reserve(locs[i].size());
      std::transform(locs[i].begin(), locs[i].end(),
                     std::back_inserter(rotatedBuildings),
                     BuildingInstanceTransformer(transInv));
      quadbuilding.buildQuadTree(rotatedBuildings.begin(), rotatedBuildings.end());

      for (size_t j = 0; j < quadbuilding.getRoot()->getNumChildren(); ++j)
              group->addChild(quadbuilding.getRoot()->getChild(j));
    }

    return group;
  }

  // We may end up with a quadtree with many empty leaves. One might say
  // that we should avoid constructing the leaves in the first place,
  // but this node visitor tries to clean up after the fact.
  struct QuadTreeCleaner : public osg::NodeVisitor
  {
      QuadTreeCleaner() : NodeVisitor(NodeVisitor::TRAVERSE_ALL_CHILDREN)
      {
      }
      void apply(LOD& lod)
      {
          for (int i  = lod.getNumChildren() - 1; i >= 0; --i) {
              EffectGeode* geode = dynamic_cast<EffectGeode*>(lod.getChild(i));
              if (!geode)
                  continue;
              bool geodeEmpty = true;
              if (geode->getNumDrawables() > 1) {
                SG_LOG(SG_TERRAIN, SG_DEBUG, "Building LOD Drawables: " << geode->getNumDrawables());
              }

              for (unsigned j = 0; j < geode->getNumDrawables(); ++j) {
                  const Geometry* geom = dynamic_cast<Geometry*>(geode->getDrawable(j));
                  if (!geom) {
                      geodeEmpty = false;
                      break;
                  }
                  for (unsigned k = 0; k < geom->getNumPrimitiveSets(); k++) {
                      const PrimitiveSet* ps = geom->getPrimitiveSet(k);
                      if (ps->getNumIndices() > 0) {
                          geodeEmpty = false;
                          break;
                      }
                  }
              }
              if (geodeEmpty)
                  lod.removeChildren(i, 1);
          }
      }
  };

  // This actually returns a MatrixTransform node. If we rotate the whole
  // forest into the local Z-up coordinate system we can reuse the
  // primitive building geometry for all the forests of the same type.
  osg::Group* createRandomBuildings(SGBuildingBinList& buildings, const osg::Matrix& transform,
                           const SGReaderWriterOptions* options)
  {
      Matrix transInv = Matrix::inverse(transform);
      static Matrix ident;
      // Set up some shared structures.
      MatrixTransform* mt = new MatrixTransform(transform);
      SGBuildingBinList::iterator i;

      for (i = buildings.begin(); i != buildings.end(); ++i) {
          SGBuildingBin* bin = *i;
          ref_ptr<Group> group = bin->createBuildingsGroup(transInv, options);

          for (size_t j = 0; j < group->getNumChildren(); ++j) {
            mt->addChild(group->getChild(j));
          }

          delete bin;
      }

      buildings.clear();

      QuadTreeCleaner cleaner;
      mt->accept(cleaner);
      return mt;
  }
}
