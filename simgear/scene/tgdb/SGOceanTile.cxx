/* -*-c++-*-
 *
 * Copyright (C) 2006-2007 Mathias Froehlich 
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

#include "SGOceanTile.hxx"

#include <simgear/compiler.h>

#include <osg/Geode>
#include <osg/Geometry>
#include <osg/MatrixTransform>
#include <osg/StateSet>

#include <simgear/bucket/newbucket.hxx>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/math/sg_types.hxx>
#include <simgear/misc/texcoord.hxx>
#include <simgear/scene/material/mat.hxx>
#include <simgear/scene/material/matlib.hxx>

void fillDrawElements(int width, int height,
                      osg::DrawElementsUShort::vector_type::iterator elements)
{
    for (int j = 0; j < height - 1; j++) {
        for (int i = 0; i < width - 1; i++) {
            *elements++ = j * width + i;
            *elements++ = j * width + i + 1;
            *elements++ = (j + 1) * width + i;
            *elements++ = (j + 1) * width + i;
            *elements++ = j * width + i + 1;
            *elements++ = (j + 1) * width + i + 1;
        }
    }
}

// Generate an ocean tile
osg::Node* SGOceanTile(const SGBucket& b, SGMaterialLib *matlib)
{
  osg::StateSet *stateSet = 0;

  double tex_width = 1000.0;
  
  // find Ocean material in the properties list
  SGMaterial *mat = matlib->find( "Ocean" );
  if ( mat != NULL ) {
    // set the texture width and height values for this
    // material
    tex_width = mat->get_xsize();
    
    // set ssgState
    stateSet = mat->get_state();
  } else {
    SG_LOG( SG_TERRAIN, SG_ALERT, "Ack! unknown use material name = Ocean");
  }
  
  // Calculate center point
  SGVec3d cartCenter = SGVec3d::fromGeod(b.get_center());
  
  double clon = b.get_center_lon();
  double clat = b.get_center_lat();
  double height = b.get_height();
  double width = b.get_width();

  // Calculate vertices. By splitting the tile up into 4 quads on a
  // side we avoid curvature-of-the-earth problems; the error should
  // be less than .5 meters.
  SGGeod geod[5][5];
  SGVec3f normals[5][5];
  SGVec3d rel[5][5];
  double longInc = width * .25;
  double latInc = height * .25;
  double startLat = clat - height * .5;
  double startLon = clon - width * .5;
  for (int j = 0; j < 5; j++) {
    double lat = startLat + j * latInc;
    for (int i = 0; i < 5; i++) {
      geod[i][j] = SGGeod::fromDeg(startLon + i * longInc, lat);
      SGVec3d cart = SGVec3d::fromGeod(geod[i][j]);
      rel[i][j] = cart - cartCenter;
      normals[i][j] = toVec3f(normalize(cart));
    }
  }
    
  // Calculate texture coordinates
  point_list geod_nodes;
  geod_nodes.reserve(5 * 5);
  int_list rectangle;
  rectangle.reserve(5 * 5);
  for (int j = 0; j < 5; j++) {
    for (int i = 0; i < 5; ++i) {
      geod_nodes.push_back(Point3D(geod[i][j].getLongitudeDeg(),
                                   geod[i][j].getLatitudeDeg(),
                                   geod[i][j].getElevationM()));
      rectangle.push_back(j * 5 + i);
    }
  }
  point_list texs = sgCalcTexCoords( b, geod_nodes, rectangle, 
                                     1000.0 / tex_width );
  
  // Allocate osg structures
  osg::Vec3Array *vl = new osg::Vec3Array;
  osg::Vec3Array *nl = new osg::Vec3Array;
  osg::Vec2Array *tl = new osg::Vec2Array;

  for (int j = 0; j < 5; j++) {
    for (int i = 0; i < 5; ++i) {
      vl->push_back(rel[i][j].osg());
      nl->push_back(normals[i][j].osg());
      tl->push_back(texs[j * 5 + i].toSGVec2f().osg());
    }
  }
  
  osg::Vec4Array* cl = new osg::Vec4Array;
  cl->push_back(osg::Vec4(1, 1, 1, 1));
  
  osg::Geometry* geometry = new osg::Geometry;
  geometry->setVertexArray(vl);
  geometry->setNormalArray(nl);
  geometry->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
  geometry->setColorArray(cl);
  geometry->setColorBinding(osg::Geometry::BIND_OVERALL);
  geometry->setTexCoordArray(0, tl);

  osg::DrawElementsUShort* drawElements
      = new osg::DrawElementsUShort(GL_TRIANGLES, 32 * 3);
  fillDrawElements(5, 5, drawElements->begin());
  geometry->addPrimitiveSet(drawElements);

  osg::Geode* geode = new osg::Geode;
  geode->setName("Ocean tile");
  geode->addDrawable(geometry);
  geode->setStateSet(stateSet);

  osg::MatrixTransform* transform = new osg::MatrixTransform;
  transform->setName("Ocean");
  transform->setMatrix(osg::Matrix::translate(cartCenter.osg()));
  transform->addChild(geode);
  
  return transform;
}
