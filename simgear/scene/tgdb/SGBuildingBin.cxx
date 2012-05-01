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
#include <simgear/math/sg_random.h>
#include <simgear/misc/sg_path.hxx>
#include <simgear/scene/material/Effect.hxx>
#include <simgear/scene/material/EffectGeode.hxx>
#include <simgear/scene/model/model.hxx>
#include <simgear/props/props.hxx>
#include <simgear/scene/util/QuadTreeBuilder.hxx>
#include <simgear/scene/util/RenderConstants.hxx>
#include <simgear/scene/util/StateAttributeFactory.hxx>
#include <simgear/structure/OSGUtils.hxx>


#include "ShaderGeometry.hxx"
#include "SGBuildingBin.hxx"

#define SG_BUILDING_QUAD_TREE_DEPTH 2
#define SG_BUILDING_FADE_OUT_LEVELS 4

using namespace osg;

namespace simgear
{
  
typedef std::map<std::string, osg::observer_ptr<osg::StateSet> > BuildingStateSetMap;
static BuildingStateSetMap statesetmap;

void addBuildingToLeafGeode(Geode* geode, const SGBuildingBin::Building& building)
{
      // Generate a repeatable random seed
      mt seed;
      mt_init(&seed, unsigned(building.position.x()));      
      
      // Get or create geometry.
      osg::ref_ptr<osg::Geometry> geom;
      osg::Vec3Array* v = new osg::Vec3Array;
      osg::Vec2Array* t = new osg::Vec2Array;
      osg::Vec4Array* c = new osg::Vec4Array; // single value
      osg::Vec3Array* n = new osg::Vec3Array;            
      
      if (geode->getNumDrawables() == 0) {
        geom = new osg::Geometry;        
        v = new osg::Vec3Array;
        t = new osg::Vec2Array;
        c = new osg::Vec4Array;
        n = new osg::Vec3Array;
        
        // Set the color, which is bound overall, and simply white
        c->push_back( osg::Vec4( 1, 1, 1, 1) );
        geom->setColorArray(c);
        geom->setColorBinding(osg::Geometry::BIND_OVERALL);

        geom->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
        // Temporary primitive set. Will be over-written later.
        geom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::QUADS,0,1));
        geode->addDrawable(geom);
      } else {
        geom = (osg::Geometry*) geode->getDrawable(0);        
        v = (osg::Vec3Array*) geom->getVertexArray();
        t = (osg::Vec2Array*) geom->getTexCoordArray(0);
        c = (osg::Vec4Array*) geom->getColorArray();
        n = (osg::Vec3Array*) geom->getNormalArray();
      }
      
      // For the moment we'll create a simple box with 5 sides (no need 
      // for a base).
      int num_quads = 5;    
      
      if (building.pitched) {
        // If it's a pitched roof, we add another 3 quads (we'll be
        // removing the flat top).
        num_quads+=3;        
      }          

      // Set up the rotation and translation matrix, which we apply to
      // vertices as they are created as we'll be adding buildings later.
      osg::Matrix transformMat;
      transformMat = osg::Matrix::translate(toOsg(building.position));
      double hdg =  - building.rotation * M_PI * 2;
      osg::Matrix rotationMat = osg::Matrix::rotate(hdg,
                                               osg::Vec3d(0.0, 0.0, 1.0));
      transformMat.preMult(rotationMat);                  

      // Create the vertices
      float cw = 0.5f * building.width;
      float cd = building.depth;
      float ch = building.height;
      
      // 0,0,0 is the bottom center of the front
      // face, e.g. where the front door would be
      
      
      // BASEMENT
      // This exteds 10m below the main section
      // Front face        
      v->push_back( osg::Vec3( 0,  cw, -10) * transformMat ); // bottom right
      v->push_back( osg::Vec3( 0, -cw, -10) * transformMat ); // bottom left
      v->push_back( osg::Vec3( 0, -cw,   0) * transformMat ); // top left
      v->push_back( osg::Vec3( 0,  cw,   0) * transformMat ); // top right
      
      for (int i=0; i<4; ++i)
        n->push_back( osg::Vec3(-1, 0, 0) * rotationMat ); // normal
      
      // Left face
      v->push_back( osg::Vec3(  0, -cw, -10) * transformMat ); // bottom right
      v->push_back( osg::Vec3( cd, -cw, -10) * transformMat ); // bottom left
      v->push_back( osg::Vec3( cd, -cw,   0) * transformMat ); // top left
      v->push_back( osg::Vec3(  0, -cw,   0) * transformMat ); // top right

      for (int i=0; i<4; ++i)
        n->push_back( osg::Vec3(0, -1, 0) * rotationMat ); // normal

      // Back face
      v->push_back( osg::Vec3( cd, -cw, -10) * transformMat ); // bottom right
      v->push_back( osg::Vec3( cd,  cw, -10) * transformMat ); // bottom left
      v->push_back( osg::Vec3( cd,  cw,   0) * transformMat ); // top left
      v->push_back( osg::Vec3( cd, -cw,   0) * transformMat ); // top right
      
      for (int i=0; i<4; ++i)
        n->push_back( osg::Vec3(1, 0, 0) * rotationMat ); // normal
      
      // Right face
      v->push_back( osg::Vec3( cd, cw, -10) * transformMat ); // bottom right
      v->push_back( osg::Vec3(  0, cw, -10) * transformMat ); // bottom left
      v->push_back( osg::Vec3(  0, cw,   0) * transformMat ); // top left
      v->push_back( osg::Vec3( cd, cw,   0) * transformMat ); // top right

      for (int i=0; i<4; ++i)
        n->push_back( osg::Vec3(0, 1, 0) * rotationMat ); // normal      
      
      // MAIN BODY
      // Front face        
      v->push_back( osg::Vec3( 0,  cw,  0) * transformMat ); // bottom right
      v->push_back( osg::Vec3( 0, -cw,  0) * transformMat ); // bottom left
      v->push_back( osg::Vec3( 0, -cw, ch) * transformMat ); // top left
      v->push_back( osg::Vec3( 0,  cw, ch) * transformMat ); // top right
      
      for (int i=0; i<4; ++i)
        n->push_back( osg::Vec3(-1, 0, 0) * rotationMat ); // normal
      
      // Left face
      v->push_back( osg::Vec3(  0, -cw,  0) * transformMat ); // bottom right
      v->push_back( osg::Vec3( cd, -cw,  0) * transformMat ); // bottom left
      v->push_back( osg::Vec3( cd, -cw, ch) * transformMat ); // top left
      v->push_back( osg::Vec3(  0, -cw, ch) * transformMat ); // top right

      for (int i=0; i<4; ++i)
        n->push_back( osg::Vec3(0, -1, 0) * rotationMat ); // normal

      // Back face
      v->push_back( osg::Vec3( cd, -cw,  0) * transformMat ); // bottom right
      v->push_back( osg::Vec3( cd,  cw,  0) * transformMat ); // bottom left
      v->push_back( osg::Vec3( cd,  cw, ch) * transformMat ); // top left
      v->push_back( osg::Vec3( cd, -cw, ch) * transformMat ); // top right
      
      for (int i=0; i<4; ++i)
        n->push_back( osg::Vec3(1, 0, 0) * rotationMat ); // normal
      
      // Right face
      v->push_back( osg::Vec3( cd, cw,  0) * transformMat ); // bottom right
      v->push_back( osg::Vec3(  0, cw,  0) * transformMat ); // bottom left
      v->push_back( osg::Vec3(  0, cw, ch) * transformMat ); // top left
      v->push_back( osg::Vec3( cd, cw, ch) * transformMat ); // top right

      for (int i=0; i<4; ++i)
        n->push_back( osg::Vec3(0, 1, 0) * rotationMat ); // normal
      
      // ROOF
      if (building.pitched) {      
        
        // Front pitched roof
        v->push_back( osg::Vec3(     0,  cw,   ch) * transformMat ); // bottom right
        v->push_back( osg::Vec3(     0, -cw,   ch) * transformMat ); // bottom left
        v->push_back( osg::Vec3(0.5*cd, -cw, ch+3) * transformMat ); // top left
        v->push_back( osg::Vec3(0.5*cd,  cw, ch+3) * transformMat ); // top right
        
        for (int i=0; i<4; ++i)
          n->push_back( osg::Vec3(-0.707, 0, 0.707) * rotationMat ); // normal
        
        // Left pitched roof
        v->push_back( osg::Vec3(     0, -cw,   ch) * transformMat ); // bottom right
        v->push_back( osg::Vec3(    cd, -cw,   ch) * transformMat ); // bottom left
        v->push_back( osg::Vec3(0.5*cd, -cw, ch+3) * transformMat ); // top left
        v->push_back( osg::Vec3(0.5*cd, -cw, ch+3) * transformMat ); // top right
        
        for (int i=0; i<4; ++i)
          n->push_back( osg::Vec3(0, -1, 0) * rotationMat ); // normal

        // Back pitched roof
        v->push_back( osg::Vec3(    cd, -cw,   ch) * transformMat ); // bottom right
        v->push_back( osg::Vec3(    cd,  cw,   ch) * transformMat ); // bottom left
        v->push_back( osg::Vec3(0.5*cd,  cw, ch+3) * transformMat ); // top left
        v->push_back( osg::Vec3(0.5*cd, -cw, ch+3) * transformMat ); // top right
        
        for (int i=0; i<4; ++i)
          n->push_back( osg::Vec3(0.707, 0, 0.707) * rotationMat ); // normal      

        // Right pitched roof
        v->push_back( osg::Vec3(    cd, cw,   ch) * transformMat ); // bottom right
        v->push_back( osg::Vec3(     0, cw,   ch) * transformMat ); // bottom left
        v->push_back( osg::Vec3(0.5*cd, cw, ch+3) * transformMat ); // top left
        v->push_back( osg::Vec3(0.5*cd, cw, ch+3) * transformMat ); // top right
        
        for (int i=0; i<4; ++i)
          n->push_back( osg::Vec3(0, 1, 0) * rotationMat ); // normal
      } else {      
        // Top face
        v->push_back( osg::Vec3(  0,  cw, ch) * transformMat ); // bottom right
        v->push_back( osg::Vec3(  0, -cw, ch) * transformMat ); // bottom left
        v->push_back( osg::Vec3( cd, -cw, ch) * transformMat ); // top left
        v->push_back( osg::Vec3( cd,  cw, ch) * transformMat ); // top right
        
        for (int i=0; i<4; ++i)
          n->push_back( osg::Vec3( 0, 0, 1) * rotationMat ); // normal
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
        float left_x = 0.0f;
        float right_x = 32.0 / 1024.0 * round((float) building.width / 6.0f);
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
          left_x = 512/1024.0;
          right_x = 512/1024.0 + 32.0 / 1024.0 * round(building.width / 6.0f);
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
          left_x = 512.0/1024.0;
          right_x = 640.0/1024.0;
          // Use the entire height of the roof texture
          top_y = base_y + 16.0 * 3.0 / 1024.0;    
          
          t->push_back( osg::Vec2( right_x, base_y) ); // bottom right
          t->push_back( osg::Vec2( left_x,  base_y) ); // bottom left
          t->push_back( osg::Vec2( left_x,  top_y ) ); // top left
          t->push_back( osg::Vec2( right_x, top_y ) ); // top right
        }
        
      }
      
      if (building.type == SGBuildingBin::MEDIUM) 
      {
        int column = ((int) (mt_rand(&seed) * 1000)) % 5;        
        float base_y = 288 / 1024.0;
        float top_y = base_y + 16.0 * (float) building.floors / 1024.0;
        float left_x = column * 192.0 /1024.0;
        float right_x = left_x + 32.0 / 1024.0 * round((float) building.width / 10.0f);

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
          right_x = left_x + 32.0 / 1024.0 * 6.0;
          
          t->push_back( osg::Vec2( right_x, base_y) ); // bottom right
          t->push_back( osg::Vec2( left_x,  base_y) ); // bottom left
          t->push_back( osg::Vec2( left_x,  top_y ) ); // top left
          t->push_back( osg::Vec2( right_x, top_y ) ); // top right
        }
      }

      if (building.type == SGBuildingBin::LARGE)
      {
        int column = ((int) (mt_rand(&seed) * 1000)) % 8;        
        float base_y = 576 / 1024.0;
        float top_y = base_y + 16.0 * (float) building.floors / 1024.0;
        float left_x = column * 128.0 /1024.0;
        float right_x = left_x + 32.0 / 1024.0 * round((float) building.width / 20.0f);

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
          
          t->push_back( osg::Vec2( right_x, base_y) ); // bottom right
          t->push_back( osg::Vec2( left_x,  base_y) ); // bottom left
          t->push_back( osg::Vec2( left_x,  top_y ) ); // top left
          t->push_back( osg::Vec2( right_x, top_y ) ); // top right
        }

      }

      // Set the vertex, texture and normals back.
      geom->setVertexArray(v);
      geom->setTexCoordArray(0, t);
      geom->setNormalArray(n);
      
      geom->setPrimitiveSet(0, new osg::DrawArrays(osg::PrimitiveSet::QUADS,0,v->size()));
      geode->setDrawable(0, geom);      
}

typedef std::map<std::string, osg::observer_ptr<Effect> > EffectMap;

static EffectMap buildingEffectMap;

// Helper classes for creating the quad tree
namespace
{
struct MakeBuildingLeaf
{
    MakeBuildingLeaf(float range, Effect* effect) :
        _range(range), _effect(effect) {}
    
    MakeBuildingLeaf(const MakeBuildingLeaf& rhs) :
        _range(rhs._range), _effect(rhs._effect)
    {}

    LOD* operator() () const
    {
        LOD* result = new LOD;
        
        // Create a series of LOD nodes so trees cover decreases slightly
        // gradually with distance from _range to 2*_range
        for (float i = 0.0; i < SG_BUILDING_FADE_OUT_LEVELS; i++)
        {   
            EffectGeode* geode = new EffectGeode;
            geode->setEffect(_effect.get());
            result->addChild(geode, 0, _range * (1.0 + i / (SG_BUILDING_FADE_OUT_LEVELS - 1.0)));               
        }
        return result;
    }
    
    float _range;
    ref_ptr<Effect> _effect;
};

struct AddBuildingLeafObject
{
    void operator() (LOD* lod, const SGBuildingBin::Building& building) const
    {
        Geode* geode = static_cast<Geode*>(lod->getChild(int(building.position.x() * 10.0f) % lod->getNumChildren()));
        addBuildingToLeafGeode(geode, building);
    }
};

struct GetBuildingCoord
{
    Vec3 operator() (const SGBuildingBin::Building& building) const
    {
        return toOsg(building.position);
    }
};

typedef QuadTreeBuilder<LOD*, SGBuildingBin::Building, MakeBuildingLeaf, AddBuildingLeafObject,
                        GetBuildingCoord> BuildingGeometryQuadtree;
}

struct BuildingTransformer
{
    BuildingTransformer(Matrix& mat_) : mat(mat_) {}
    SGBuildingBin::Building operator()(const SGBuildingBin::Building& building) const
    {
        Vec3 pos = toOsg(building.position);
        return SGBuildingBin::Building(toSG(pos * mat), building);
    }
    Matrix mat;
};



// This actually returns a MatrixTransform node. If we rotate the whole
// forest into the local Z-up coordinate system we can reuse the
// primitive building geometry for all the forests of the same type.
osg::Group* createRandomBuildings(SGBuildingBinList buildings, const osg::Matrix& transform,
                         const SGReaderWriterOptions* options)
{
    Matrix transInv = Matrix::inverse(transform);
    static Matrix ident;
    // Set up some shared structures.
    MatrixTransform* mt = new MatrixTransform(transform);

    SGBuildingBin* bin = NULL;
      
    BOOST_FOREACH(bin, buildings)
    {      
      
        ref_ptr<Effect> effect;
        EffectMap::iterator iter = buildingEffectMap.find(bin->texture);

        if ((iter == buildingEffectMap.end())||
            (!iter->second.lock(effect)))
        {
            SGPropertyNode_ptr effectProp = new SGPropertyNode;
            makeChild(effectProp, "inherits-from")->setStringValue("Effects/building");
            SGPropertyNode* params = makeChild(effectProp, "parameters");
            // Main texture - n=0
            params->getChild("texture", 0, true)->getChild("image", 0, true)
                ->setStringValue(bin->texture);

            // Light map - n=1
            params->getChild("texture", 1, true)->getChild("image", 0, true)
                ->setStringValue(bin->lightMap);
                
            effect = makeEffect(effectProp, true, options);
            if (iter == buildingEffectMap.end())
                buildingEffectMap.insert(EffectMap::value_type(bin->texture, effect));
            else
                iter->second = effect; // update existing, but empty observer
        }
      
        // Now, create a quadbuilding for the buildings.            
        BuildingGeometryQuadtree
            quadbuilding(GetBuildingCoord(), AddBuildingLeafObject(),
                     SG_BUILDING_QUAD_TREE_DEPTH,
                     MakeBuildingLeaf(20000.0f, effect)); // FIXME - tie to property
                     
        // Transform building positions from the "geocentric" positions we
        // get from the scenery polys into the local Z-up coordinate
        // system.
        std::vector<SGBuildingBin::Building> rotatedBuildings;
        rotatedBuildings.reserve(bin->buildings.size());
        std::transform(bin->buildings.begin(), bin->buildings.end(),
                       std::back_inserter(rotatedBuildings),
                       BuildingTransformer(transInv));
        quadbuilding.buildQuadTree(rotatedBuildings.begin(), rotatedBuildings.end());
        
        ref_ptr<Group> group = quadbuilding.getRoot();
        
        mt->addChild(group);        
    }
    
    return mt;
}

}
