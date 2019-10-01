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
#include <simgear/io/iostreams/sgstream.hxx>
#include <simgear/math/SGLimits.hxx>
#include <simgear/math/SGMisc.hxx>
#include <simgear/math/sg_random.h>
#include <simgear/misc/sg_path.hxx>
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
        osg::Vec3Array* v = new osg::Vec3Array;
        osg::Vec2Array* t = new osg::Vec2Array;
        osg::Vec3Array* n = new osg::Vec3Array;
        osg::Vec4Array* c = new osg::Vec4Array;
        // Color array is used to identify the different building faces by the
        // vertex shader for texture mapping.

        v->reserve(48);
        t->reserve(48);
        n->reserve(48);
        c->reserve(48);

        // Now create an OSG Geometry based on the Building
        // 0,0,0 is the bottom center of the front
        // face, e.g. where the front door would be

        // BASEMENT
        // This extends 10m below the main section
        // Front face
        v->push_back( osg::Vec3( 0.0, -0.5, -1.0) ); // bottom right
        v->push_back( osg::Vec3( 0.0,  0.5, -1.0) ); // bottom left
        v->push_back( osg::Vec3( 0.0,  0.5,  0.0) ); // top left
        v->push_back( osg::Vec3( 0.0, -0.5,  0.0) ); // top right

        for (int i=0; i<4; ++i) {
          n->push_back( osg::Vec3(1, 0, 0) );    // normal
          c->push_back( osg::Vec4(1, 0, 0, 0) ); // color - used to differentiate wall from roof
        }

        // Left face
        v->push_back( osg::Vec3( -1.0, -0.5, -1.0) ); // bottom right
        v->push_back( osg::Vec3(  0.0, -0.5, -1.0) ); // bottom left
        v->push_back( osg::Vec3(  0.0, -0.5,  0.0) ); // top left
        v->push_back( osg::Vec3( -1.0, -0.5,  0.0) ); // top right

        for (int i=0; i<4; ++i) {
          n->push_back( osg::Vec3(0, -1, 0) );   // normal
          c->push_back( osg::Vec4(1, 0, 0, 0) ); // color - used to differentiate wall from roof
        }

        // Back face
        v->push_back( osg::Vec3( -1.0,  0.5, -1.0) ); // bottom right
        v->push_back( osg::Vec3( -1.0, -0.5, -1.0) ); // bottom left
        v->push_back( osg::Vec3( -1.0, -0.5,  0.0) ); // top left
        v->push_back( osg::Vec3( -1.0,  0.5,  0.0) ); // top right

        for (int i=0; i<4; ++i) {
          n->push_back( osg::Vec3(-1, 0, 0) );   // normal
          c->push_back( osg::Vec4(1, 0, 0, 0) ); // color - used to differentiate wall from roof
        }

        // Right face
        v->push_back( osg::Vec3(  0.0, 0.5, -1.0) ); // bottom right
        v->push_back( osg::Vec3( -1.0, 0.5, -1.0) ); // bottom left
        v->push_back( osg::Vec3( -1.0, 0.5,  0.0) ); // top left
        v->push_back( osg::Vec3(  0.0, 0.5,  0.0) ); // top right

        for (int i=0; i<4; ++i) {
          n->push_back( osg::Vec3(0, 1, 0) );    // normal
          c->push_back( osg::Vec4(1, 0, 0, 0) ); // color - used to differentiate wall from roof
        }

        // MAIN BODY
        // Front face
        v->push_back( osg::Vec3( 0.0, -0.5, 0.0) ); // bottom right
        v->push_back( osg::Vec3( 0.0,  0.5, 0.0) ); // bottom left
        v->push_back( osg::Vec3( 0.0,  0.5, 1.0) ); // top left
        v->push_back( osg::Vec3( 0.0, -0.5, 1.0) ); // top right

        for (int i=0; i<4; ++i) {
          n->push_back( osg::Vec3(1, 0, 0) );    // normal
          c->push_back( osg::Vec4(1, 0, 0, 0) ); // color - used to differentiate wall from roof
        }

        // Left face
        v->push_back( osg::Vec3( -1.0, -0.5, 0.0) ); // bottom right
        v->push_back( osg::Vec3(  0.0, -0.5, 0.0) ); // bottom left
        v->push_back( osg::Vec3(  0.0, -0.5, 1.0) ); // top left
        v->push_back( osg::Vec3( -1.0, -0.5, 1.0) ); // top right

        for (int i=0; i<4; ++i) {
          n->push_back( osg::Vec3(0, -1, 0) );    // normal
          c->push_back( osg::Vec4(1, 0, 0, 0) ); // color - used to differentiate wall from roof
        }

        // Back face
        v->push_back( osg::Vec3( -1.0,  0.5, 0.0) ); // bottom right
        v->push_back( osg::Vec3( -1.0, -0.5, 0.0) ); // bottom left
        v->push_back( osg::Vec3( -1.0, -0.5, 1.0) ); // top left
        v->push_back( osg::Vec3( -1.0,  0.5, 1.0) ); // top right

        for (int i=0; i<4; ++i) {
          n->push_back( osg::Vec3(-1, 0, 0) );    // normal
          c->push_back( osg::Vec4(1, 0, 0, 0) ); // color - used to differentiate wall from roof
        }

        // Right face
        v->push_back( osg::Vec3(  0.0, 0.5, 0.0) ); // bottom right
        v->push_back( osg::Vec3( -1.0, 0.5, 0.0) ); // bottom left
        v->push_back( osg::Vec3( -1.0, 0.5, 1.0) ); // top left
        v->push_back( osg::Vec3(  0.0, 0.5, 1.0) ); // top right

        for (int i=0; i<4; ++i) {
          n->push_back( osg::Vec3(0, 1, 0) );    // normal
          c->push_back( osg::Vec4(1, 0, 0, 0) ); // color - used to differentiate wall from roof
        }

        // ROOF - assume pitched - will be flattened in shader if required.
        // Front pitched roof
        v->push_back( osg::Vec3(  0.0, -0.5, 1.0) ); // bottom right
        v->push_back( osg::Vec3(  0.0,  0.5, 1.0) ); // bottom left
        v->push_back( osg::Vec3( -0.5,  0.5, 1.5) ); // top left
        v->push_back( osg::Vec3( -0.5, -0.5, 1.5) ); // top right

        for (int i=0; i<4; ++i) {
          n->push_back( osg::Vec3(0.707, 0, 0.707) ); // normal
          c->push_back( osg::Vec4(0, 1, 0, 0) ); // color - used to differentiate wall from roof
        }

        // Left pitched roof
        v->push_back( osg::Vec3(-1.0, -0.5, 1.0) ); // bottom right
        v->push_back( osg::Vec3( 0.0, -0.5, 1.0) ); // bottom left
        v->push_back( osg::Vec3(-0.5, -0.5, 1.5) ); // top left
        v->push_back( osg::Vec3(-0.5, -0.5, 1.5) ); // top right

        for (int i=0; i<4; ++i) {
          n->push_back( osg::Vec3(0, -1, 0) );   // normal
          c->push_back( osg::Vec4(0, 1, 0, 0) ); // color - used to differentiate wall from roof
        }

        // Back pitched roof
        v->push_back( osg::Vec3(-1.0,  0.5, 1.0) ); // bottom right
        v->push_back( osg::Vec3(-1.0, -0.5, 1.0) ); // bottom left
        v->push_back( osg::Vec3(-0.5, -0.5, 1.5) ); // top left
        v->push_back( osg::Vec3(-0.5,  0.5, 1.5) ); // top right

        for (int i=0; i<4; ++i) {
          n->push_back( osg::Vec3(-0.707, 0, 0.707) ); // normal
          c->push_back( osg::Vec4(0, 1, 0, 0) ); // color - used to differentiate wall from roof
        }

        // Right pitched roof
        v->push_back( osg::Vec3( 0.0, 0.5, 1.0) ); // bottom right
        v->push_back( osg::Vec3(-1.0, 0.5, 1.0) ); // bottom left
        v->push_back( osg::Vec3(-0.5, 0.5, 1.5) ); // top left
        v->push_back( osg::Vec3(-0.5, 0.5, 1.5) ); // top right

        for (int i=0; i<4; ++i) {
          n->push_back( osg::Vec3(0, 1, 0) );   // normal
          c->push_back( osg::Vec4(0, 1, 0, 0) ); // color - used to differentiate wall from roof
        }

        // The 1024x1024 texture is split into 32x16 blocks.
        // For a small building, each block is 6m wide and 3m high.
        // For a medium building, each block is 10m wide and 3m high.
        // For a large building, each block is 20m wide and 3m high

        // Texture coordinates are adjusted in the shader.  Here we just use
        // the full [0,1] range.

        // BASEMENT - uses the baseline texture
        for (unsigned int i = 0; i < 16; i++) {
          t->push_back( osg::Vec2( 0.0, 0.0) );
        }

        // MAIN BODY
        // Front
        t->push_back( osg::Vec2( 1.0, 0.0) ); // bottom right
        t->push_back( osg::Vec2( 0.0, 0.0) ); // bottom left
        t->push_back( osg::Vec2( 0.0, 1.0) ); // top left
        t->push_back( osg::Vec2( 1.0, 1.0) ); // top right

        // Left
        t->push_back( osg::Vec2( 0.0, 0.0) ); // bottom right
        t->push_back( osg::Vec2( 1.0, 0.0) ); // bottom left
        t->push_back( osg::Vec2( 1.0, 1.0) ); // top left
        t->push_back( osg::Vec2( 0.0, 1.0) ); // top right

        // Back (same as front for the moment)
        t->push_back( osg::Vec2( 1.0, 0.0) ); // bottom right
        t->push_back( osg::Vec2( 0.0, 0.0) ); // bottom left
        t->push_back( osg::Vec2( 0.0, 1.0 ) ); // top left
        t->push_back( osg::Vec2( 1.0, 1.0 ) ); // top right

        // Right (same as left for the moment)
        t->push_back( osg::Vec2( 0.0, 0.0) ); // bottom right
        t->push_back( osg::Vec2( 1.0, 0.0) ); // bottom left
        t->push_back( osg::Vec2( 1.0, 1.0 ) ); // top left
        t->push_back( osg::Vec2( 0.0, 1.0 ) ); // top right

        // ROOF
        // ROOF - Front top surface.  Negative x-coord will wrap texture
        t->push_back( osg::Vec2( -1.0, 0.0) ); // bottom right
        t->push_back( osg::Vec2(  0.0, 0.0) ); // bottom left
        t->push_back( osg::Vec2(  0.0, 1.0 ) ); // top left
        t->push_back( osg::Vec2( -1.0, 1.0 ) ); // top right

        // ROOF - Left
        t->push_back( osg::Vec2( 1.0, 1.0) ); // bottom right
        t->push_back( osg::Vec2( 1.0, 0.0) ); // bottom left
        t->push_back( osg::Vec2( 1.0, 0.0 ) ); // top left
        t->push_back( osg::Vec2( 1.0, 1.0 ) ); // top right

        // ROOF - Back (same as front for the moment)
        t->push_back( osg::Vec2( -1.0, 0.0) ); // bottom right
        t->push_back( osg::Vec2(  0.0, 0.0) ); // bottom left
        t->push_back( osg::Vec2(  0.0, 1.0 ) ); // top left
        t->push_back( osg::Vec2( -1.0, 1.0 ) ); // top right

        // ROOF - Right (same as left for the moment)
        t->push_back( osg::Vec2( 1.0, 1.0) ); // bottom right
        t->push_back( osg::Vec2( 1.0, 0.0) ); // bottom left
        t->push_back( osg::Vec2( 1.0, 0.0 ) ); // top left
        t->push_back( osg::Vec2( 1.0, 1.0 ) ); // top right

        Geometry* geom = new Geometry;
        static int buildingCounter = 0;
        geom->setName("BuildingGeometry_" + std::to_string(buildingCounter++));
        geom->setVertexArray(v);
        geom->setTexCoordArray(0, t, Array::BIND_PER_VERTEX);
        geom->setNormalArray(n, Array::BIND_PER_VERTEX);
        geom->setColorArray(c);
        geom->setUseDisplayList( false );
        geom->setUseVertexBufferObjects( true );
        geom->setComputeBoundingBoxCallback(new BuildingBoundingBoxCallback);

        geom->setVertexAttribArray(BUILDING_POSITION_ATTR, new osg::Vec3Array, Array::BIND_PER_VERTEX);
        geom->setVertexAttribArray(BUILDING_SCALE_ATTR, new osg::Vec3Array, Array::BIND_PER_VERTEX);
        geom->setVertexAttribArray(BUILDING_ROT_PITCH_TEX0X_ATTR, new osg::Vec3Array, Array::BIND_PER_VERTEX);
        geom->setVertexAttribArray(BUILDING_TEX0Y_TEX1X_TEX1Y_ATTR, new osg::Vec3Array, Array::BIND_PER_VERTEX);
        geom->setVertexAttribArray(BUILDING_RTEX0X_RTEX0Y_ATTR, new osg::Vec3Array, Array::BIND_PER_VERTEX);

        geom->addPrimitiveSet( new osg::DrawArrays( GL_QUADS, 0, 48, 0) );

        EffectGeode* geode = new EffectGeode;
        geode->addDrawable(geom);
        geode->setEffect(_effect.get());

        StateSet* ss = geode->getOrCreateStateSet();
        ss->setAttributeAndModes(new osg::VertexAttribDivisor(BUILDING_POSITION_ATTR, 1));
        ss->setAttributeAndModes(new osg::VertexAttribDivisor(BUILDING_SCALE_ATTR, 1));
        ss->setAttributeAndModes(new osg::VertexAttribDivisor(BUILDING_ROT_PITCH_TEX0X_ATTR, 1));
        ss->setAttributeAndModes(new osg::VertexAttribDivisor(BUILDING_TEX0Y_TEX1X_TEX1Y_ATTR, 1));
        ss->setAttributeAndModes(new osg::VertexAttribDivisor(BUILDING_RTEX0X_RTEX0Y_ATTR, 1));

        LOD* result = new LOD;
        result->addChild(geode, 0, _range);
        return result;
    }

    float _range;
    ref_ptr<Effect> _effect;
    bool _fade_out;
};

struct AddBuildingLeafObject
{
    void operator() (LOD* lod, const SGBuildingBin::BuildingInstance building) const
    {
        //Geode* geode = static_cast<Geode*>(lod->getChild(int(building.position.x() * 10.0f) % lod->getNumChildren()));
        Geode* geode = static_cast<Geode*>(lod->getChild(0));

        Geometry* geom = static_cast<Geometry*>(geode->getDrawable(0));

        osg::Vec3Array* positions =  static_cast<osg::Vec3Array*> (geom->getVertexAttribArray(BUILDING_POSITION_ATTR));    // (x,y,z)
        osg::Vec3Array* scale =  static_cast<osg::Vec3Array*> (geom->getVertexAttribArray(BUILDING_SCALE_ATTR)); // (width, depth, height)
        osg::Vec3Array* rot = static_cast<osg::Vec3Array*> (geom->getVertexAttribArray(BUILDING_ROT_PITCH_TEX0X_ATTR)); // (rotation, pitch height, wall texture x offset)
        osg::Vec3Array* tex = static_cast<osg::Vec3Array*> (geom->getVertexAttribArray(BUILDING_TEX0Y_TEX1X_TEX1Y_ATTR)); // (wall texture y offset, wall/roof texture x gain, wall/roof texture y gain)
        osg::Vec3Array* rtex = static_cast<osg::Vec3Array*> (geom->getVertexAttribArray(BUILDING_RTEX0X_RTEX0Y_ATTR)); // (roof texture x offset, roof texture y offset, unused)

        positions->push_back(building.position);
        // Depth is the x-axis, width is the y-axis
        scale->push_back(osg::Vec3(building.depth, building.width, building.height));
        rot->push_back(osg::Vec3(building.rotation, building.pitch_height, building.walltex0.x()));
        tex->push_back(osg::Vec3(building.walltex0.y(), building.tex1.x(), building.tex1.y()));
        rtex->push_back(osg::Vec3(building.rooftex0.x(), building.rooftex0.y(), 0.0f));

        DrawArrays* primSet = static_cast<DrawArrays*>(geom->getPrimitiveSet(0));
        primSet->setNumInstances(positions->size());
    }
};

struct GetBuildingCoord
{
    Vec3 operator() (const SGBuildingBin::BuildingInstance& building) const
    {
        return building.position;
    }
};

typedef QuadTreeBuilder<LOD*, SGBuildingBin::BuildingInstance, MakeBuildingLeaf, AddBuildingLeafObject,
                        GetBuildingCoord> BuildingGeometryQuadtree;


  // Set up a BuildingBin from a file containing a list of individual building
  // positions.
  SGBuildingBin::SGBuildingBin(const SGPath& absoluteFileName, const SGMaterial *mat, bool useVBOs) :
    SGBuildingBin::SGBuildingBin(mat, useVBOs)
  {
    if (!absoluteFileName.exists()) {
      SG_LOG(SG_TERRAIN, SG_ALERT, "Building list file " << absoluteFileName << " does not exist.");
      return;
    }

    sg_gzifstream stream(absoluteFileName);
    if (!stream.is_open()) {
      SG_LOG(SG_TERRAIN, SG_ALERT, "Unable to open " << absoluteFileName);
      return;
    }

    while (!stream.eof()) {
      // read a line.  Each line defines a single building position, and may have
      // a comment, starting with #
      std::string line;
      std::getline(stream, line);

      // strip comments
      std::string::size_type hash_pos = line.find('#');
      if (hash_pos != std::string::npos)
          line.resize(hash_pos);

      // and process further
      std::stringstream in(line);

      // Line format is X Y Z R B W D H P S O F WT RT
      // where:
      // X,Y,Z are the cartesian coordinates of the center of the front face. +X is East, +Y is North
      // R is the building rotation in degrees centered on the middle of the front face.
      // B is the building type [0, 1, 2] for SMALL, MEDIUM, LARGE
      // W is the building width in meters
      // D is the building depth in meters
      // H is the building height in meters, excluding any pitched roof
      // P is the pitch height in meters. 0 for a flat roof
      // S is the roof shape (currently unused - all roofs are flat or gabled depending on pitch height) :
      //   0=flat 1=skillion 2=gabled 3=half-hipped 4=hipped 5=pyramidal 6=gambled
      //   7=mansard 8=dome 9=onion 10=round 11=saltbox
      // O is the roof ridge orientation (currently unused, all roofs are assumed orthogonal) :
      //   0 = parallel to the front face of the building
      //   1 = orthogonal to the front face of the building
      // F is the number of floors (integer)
      // WT is the texture index to use (integer) for walls. Buildings with the same WT value will have the same wall texture assigned.  There are 6 small, 6 medium and 4 large textures.
      // RT is the texture index to use (integer) for roofs. Buildings with the same RT value will have the same roof texture assigned.  There are 6 small, 6 medium and 4 large textures.
      float x, y, z, r, w, d, h, p;
      int b, s, o, f, wt, rt;
      in >> x >> y >> z >> r >> b >> w >> d >> h >> p >> s >> o >> f >> wt >> rt;

      //SG_LOG(SG_TERRAIN, SG_ALERT, "Building entry " << x << " " << y << " " << z << " " << b );
      SGVec3f loc = SGVec3f(x,y,z);
      BuildingType type = BuildingType::SMALL;
      if (b == 1)  type = BuildingType::MEDIUM;
      if (b == 2)  type = BuildingType::LARGE;

      // Rotation is in the file as degrees, but in the datastructure normalized
      // to 0.0 - 1.0
      float rot = (float) (r / 360.0f);

      if (w == 0.0f) {
        // If width is not defined, then we assume we don't have a full set of
        // data for the buildings so just use the random building definitions
        insert(loc, rot, type);
      } else {
        insert(loc, rot, type, w, d, h, p, f, wt, rt);
      }
    }

    stream.close();
  };

  // Set up the building set based on the material definitions
  SGBuildingBin::SGBuildingBin(const SGMaterial *mat, bool useVBOs) {
    material_name = new std::string(mat->get_names()[0]);
    SG_LOG(SG_TERRAIN, SG_DEBUG, "Building material " << material_name);
    material = mat;
    texture = new std::string(mat->get_building_texture());
    lightMap = new std::string(mat->get_building_lightmap());
    buildingRange = mat->get_building_range();
    SG_LOG(SG_TERRAIN, SG_DEBUG, "Building texture " << texture);
  }

  SGBuildingBin::~SGBuildingBin() {
    buildingLocations.clear();
  }

  // Generate a building specifying the exact position, dimensions and texture index.
  void SGBuildingBin::insert(SGVec3f p, float r, BuildingType buildingtype, float width, float depth, float height, float pitch_height, int floors, int wall_tex_index, int roof_tex_index) {

    // The 2048x2048 texture is split into 64x32 blocks.  So there are 64 on
    // the x-axis and 128 on the y-axis.
    // The leftmost 32 are used for the sides of the building, and the rightmost
    // 32 for the roof.
    const float BUILDING_TEXTURE_BLOCK_HEIGHT = 1.0f / 64.0f; // The height of a single block within the random building texture
    const float BUILDING_TEXTURE_BLOCK_WIDTH  = 1.0f / 32.0f;  // The width of a single block within the random building texture
    Vec2f wall_tex0, roof_tex0, tex1;

    if (buildingtype == SGBuildingBin::SMALL) {
      // SMALL BUILDINGS
      // Maximum texture height is 3 stories.
      // Small buildings are represented on the bottom 18 rows
      // Each block is 6m wide and 3m high.
      int wall_row = wall_tex_index % 6;
      int roof_row = roof_tex_index % 6;
      float wall_offset_x = 0.0f;
      float wall_offset_y = (float) wall_row  * 3.0f * BUILDING_TEXTURE_BLOCK_HEIGHT;
      float roof_offset_x = 0.0f;
      float roof_offset_y = (float) roof_row  * 3.0f * BUILDING_TEXTURE_BLOCK_HEIGHT;
      float gain_x = min(0.5f, std::round(width / 6.0f) * BUILDING_TEXTURE_BLOCK_WIDTH);
      float gain_y =  (float) (min(3, floors)) * BUILDING_TEXTURE_BLOCK_HEIGHT;

      wall_tex0 = Vec2f(wall_offset_x, wall_offset_y);
      roof_tex0 = Vec2f(roof_offset_x, roof_offset_y);
      tex1 = Vec2f(gain_x, gain_y);
    } else if (buildingtype == SGBuildingBin::MEDIUM) {
      // MEDIUM BUILDING
      // Maximum texture height is 8 stories.
      // Medium buildings are arranged on the texture in a 2x3 pattern.
      // For a medium building, each block is 10m wide and 3m high.
      int wall_column = wall_tex_index % 2;
      int wall_row = wall_tex_index % 3;
      int roof_column = roof_tex_index % 2;
      int roof_row = roof_tex_index % 3;

      float wall_offset_x = wall_column * 0.25f;
      // Counting from the bottom, we have 6 rows of small buildings, each 3 blocks high
      float wall_offset_y = (6.0f * 3.0f + wall_row * 8.0f) * BUILDING_TEXTURE_BLOCK_HEIGHT;

      float roof_offset_x = roof_column * 0.25f;
      // Counting from the bottom, we have 6 rows of small buildings, each 3 blocks high
      float roof_offset_y = (6.0f * 3.0f + roof_row * 8.0f) * BUILDING_TEXTURE_BLOCK_HEIGHT;

      float gain_x = min(0.25f, std::round(width / 10.0f) * BUILDING_TEXTURE_BLOCK_WIDTH);
      float gain_y = (float) (min(8, floors)) * BUILDING_TEXTURE_BLOCK_HEIGHT;

      wall_tex0 = Vec2f(wall_offset_x, wall_offset_y);
      roof_tex0 = Vec2f(roof_offset_x, roof_offset_y);
      tex1 = Vec2f(gain_x, gain_y);
    } else {
      // LARGE BUILDING
      // Maximum texture height is 22 stories.
      // Large buildings are arranged in a 4x1 pattern
      // Each block is 20m wide and 3m high.
      int wall_column = wall_tex_index % 4;
      int roof_column = roof_tex_index % 4;
      // Counting from the bottom we have 6 rows of small buildings (3 blocks high),
      // then 3 rows of medium buildings (8 blocks high).  Then the large building texture
      float wall_offset_x = wall_column * 0.125f;
      float wall_offset_y = (6.0f * 3.0f + 3.0f * 8.0f) * BUILDING_TEXTURE_BLOCK_HEIGHT;
      float roof_offset_x = roof_column * 0.125f;
      float roof_offset_y = (6.0f * 3.0f + 3.0f * 8.0f) * BUILDING_TEXTURE_BLOCK_HEIGHT;
      float gain_x = min(0.125f, std::round(width / 20.0f) * BUILDING_TEXTURE_BLOCK_WIDTH);
      float gain_y = (float) min(22, floors) * BUILDING_TEXTURE_BLOCK_HEIGHT;

      wall_tex0 = Vec2f(wall_offset_x, wall_offset_y);
      roof_tex0 = Vec2f(roof_offset_x, roof_offset_y);
      tex1 = Vec2f(gain_x, gain_y);
    }

    buildingLocations.push_back(BuildingInstance(toOsg(p), width, depth, height, pitch_height, r, wall_tex0, roof_tex0, tex1));
  }


  // Generate a building of a given type at a specified position, using the random building material definition to determine the dimensions and texture index.
  void SGBuildingBin::insert(SGVec3f p, float r, BuildingType buildingtype) {

    float width, depth, height, pitch_height;
    int floors;

    // Generate a random seed for the building generation.
    mt seed;
    mt_init(&seed, unsigned(p.x() + p.y() + p.z()));

    if (buildingtype == SGBuildingBin::SMALL) {
      // Small building
      // Maximum number of floors is 3, and maximum width/depth is 192m.
      width = material->get_building_small_min_width() + mt_rand(&seed) * mt_rand(&seed) * (material->get_building_small_max_width() - material->get_building_small_min_width());
      depth = material->get_building_small_min_depth() + mt_rand(&seed) * mt_rand(&seed) * (material->get_building_small_max_depth() - material->get_building_small_min_depth());
      floors = SGMisc<double>::round(material->get_building_small_min_floors() + mt_rand(&seed) * (material->get_building_small_max_floors() - material->get_building_small_min_floors()));
      height = floors * (2.8 + mt_rand(&seed));

      // Small buildings are never deeper than they are wide.
      if (depth > width) { depth = width; }

      pitch_height = (mt_rand(&seed) < material->get_building_small_pitch()) ? 3.0 : 0.0;
    } else if (buildingtype == SGBuildingBin::MEDIUM) {
      // MEDIUM BUILDING
      width = material->get_building_medium_min_width() + mt_rand(&seed) * mt_rand(&seed) * (material->get_building_medium_max_width() - material->get_building_medium_min_width());
      depth = material->get_building_medium_min_depth() + mt_rand(&seed) * mt_rand(&seed) * (material->get_building_medium_max_depth() - material->get_building_medium_min_depth());
      floors = SGMisc<double>::round(material->get_building_medium_min_floors() + mt_rand(&seed) * (material->get_building_medium_max_floors() - material->get_building_medium_min_floors()));
      height = floors * (2.8 + mt_rand(&seed));

      while ((height > width) && (floors > material->get_building_medium_min_floors())) {
        // Ensure that medium buildings aren't taller than they are wide
        floors--;
        height = floors * (2.8 + mt_rand(&seed));
      }

      pitch_height = (mt_rand(&seed) < material->get_building_medium_pitch()) ? 3.0 : 0.0;
    } else {
      // LARGE BUILDING
      width = material->get_building_large_min_width() + mt_rand(&seed) * (material->get_building_large_max_width() - material->get_building_large_min_width());
      depth = material->get_building_large_min_depth() + mt_rand(&seed) * (material->get_building_large_max_depth() - material->get_building_large_min_depth());
      floors = SGMisc<double>::round(material->get_building_large_min_floors() + mt_rand(&seed) * (material->get_building_large_max_floors() - material->get_building_large_min_floors()));
      height = floors * (2.8 + mt_rand(&seed));
      pitch_height = (mt_rand(&seed) < material->get_building_large_pitch()) ? 3.0 : 0.0;
    }

    insert(p, r, buildingtype, width, depth, height, pitch_height, floors, (int) (mt_rand(&seed) * 1000.0), (int) (mt_rand(&seed) * 1000.0));
  }

  int SGBuildingBin::getNumBuildings() {
    return buildingLocations.size();
  }

  bool SGBuildingBin::checkMinDist (SGVec3f p, float radius) {
    BuildingInstanceList::iterator iter;
    for (iter = buildingLocations.begin(); iter != buildingLocations.end(); ++iter) {
      if (iter->getDistSqr(toOsg(p)) < radius) {
        return false;
      }
    }
    return true;
  }

  SGBuildingBin::BuildingType SGBuildingBin::getBuildingType(float roll) {

    if (roll < material->get_building_small_fraction()) {
      return SGBuildingBin::SMALL;
    }

    if (roll < (material->get_building_small_fraction() + material->get_building_medium_fraction())) {
      return SGBuildingBin::MEDIUM;
    }

    return SGBuildingBin::LARGE;
  }

  float SGBuildingBin::getBuildingMaxRadius(BuildingType type) {
    if (type == SGBuildingBin::SMALL) return material->get_building_small_max_width();
    if (type == SGBuildingBin::MEDIUM) return material->get_building_medium_max_width();
    if (type == SGBuildingBin::LARGE) return material->get_building_large_max_width();
    return 0;
  }

  float SGBuildingBin::getBuildingMaxDepth(BuildingType type) {
    if (type == SGBuildingBin::SMALL) return material->get_building_small_max_depth();
    if (type == SGBuildingBin::MEDIUM) return material->get_building_medium_max_depth();
    if (type == SGBuildingBin::LARGE) return material->get_building_large_max_depth();
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

    // Transform building positions from the "geocentric" positions we
    // get from the scenery polys into the local Z-up coordinate
    // system.
    std::vector<BuildingInstance> rotatedBuildings;
    rotatedBuildings.reserve(buildingLocations.size());
    for (const auto &b : buildingLocations) {
        rotatedBuildings.emplace_back(BuildingInstance(
            b.position * transInv,
            b
        ));
    }

    // Now, create a quadbuilding for the buildings.
    BuildingGeometryQuadtree
        quadbuilding(GetBuildingCoord(), AddBuildingLeafObject(),
                 SG_BUILDING_QUAD_TREE_DEPTH,
                 MakeBuildingLeaf(buildingRange, effect, false));

    quadbuilding.buildQuadTree(rotatedBuildings.begin(), rotatedBuildings.end());

    ref_ptr<Group> group = new osg::Group();

    static int buildingGroupCounter = 0;
    group->setName("BuildingsGroup_" + std::to_string(buildingGroupCounter++));

    for (size_t j = 0; j < quadbuilding.getRoot()->getNumChildren(); ++j)
            group->addChild(quadbuilding.getRoot()->getChild(j));

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
  // set of buildings into the local Z-up coordinate system we can reuse the
  // primitive building geometry for all the buildings of the same type.
  osg::Group* createRandomBuildings(SGBuildingBinList& buildings, const osg::Matrix& transform,
                           const SGReaderWriterOptions* options)
  {
      Matrix transInv = Matrix::inverse(transform);
      // Set up some shared structures.
      MatrixTransform* mt = new MatrixTransform(transform);
      SGBuildingBinList::iterator i;

      for (i = buildings.begin(); i != buildings.end(); ++i) {
          SGBuildingBin* bin = *i;
          ref_ptr<Group> group = bin->createBuildingsGroup(transInv, options);

          for (size_t j = 0; j < group->getNumChildren(); ++j) {
            mt->addChild(group->getChild(j));
          }
      }

      QuadTreeCleaner cleaner;
      mt->accept(cleaner);
      return mt;
  }
}
