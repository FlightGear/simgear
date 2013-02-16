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

#ifndef SG_TEXTURED_TRIANGLE_BIN_HXX
#define SG_TEXTURED_TRIANGLE_BIN_HXX

#include <osg/Array>
#include <osg/Geometry>
#include <osg/PrimitiveSet>
#include <osg/Texture2D>
#include <stdio.h>

#include <simgear/math/sg_random.h>
#include <simgear/scene/util/OsgMath.hxx>
#include "SGTriangleBin.hxx"



struct SGVertNormTex {
  SGVertNormTex()
  { }
  SGVertNormTex(const SGVec3f& v, const SGVec3f& n, const SGVec2f& t) :
    vertex(v), normal(n), texCoord(t)
  { }
  struct less
  {
    inline bool operator() (const SGVertNormTex& l,
                            const SGVertNormTex& r) const
    {
      if (l.vertex < r.vertex) return true;
      else if (r.vertex < l.vertex) return false;
      else if (l.normal < r.normal) return true;
      else if (r.normal < l.normal) return false;
      else return l.texCoord < r.texCoord;
    }
  };

  SGVec3f vertex;
  SGVec3f normal;
  SGVec2f texCoord;
};

// Use a DrawElementsUShort if there are few enough vertices,
// otherwise fallback to DrawElementsUInt. Hide the differences
// between the two from the rest of the code.
//
// We don't bother with DrawElementsUByte because that is generally
// not an advantage on modern hardware.
class DrawElementsFacade {
public:
    DrawElementsFacade(unsigned numVerts) :
        _ushortElements(0), _uintElements(0)
    {
        if (numVerts > 65535)
            _uintElements
                = new osg::DrawElementsUInt(osg::PrimitiveSet::TRIANGLES);
        else
            _ushortElements
                = new osg::DrawElementsUShort(osg::PrimitiveSet::TRIANGLES);
    }
    
    void push_back(unsigned val)
    {
        if (_uintElements)
            _uintElements->push_back(val);
        else
            _ushortElements->push_back(val);
    }

    osg::DrawElements* getDrawElements()
    {
        if (_uintElements)
            return _uintElements;
        return _ushortElements;
    }
protected:
    osg::DrawElementsUShort* _ushortElements;
    osg::DrawElementsUInt* _uintElements;
};

class SGTexturedTriangleBin : public SGTriangleBin<SGVertNormTex> {
public:
  SGTexturedTriangleBin()
  {
    mt_init(&seed, 123);
  }

  // Computes and adds random surface points to the points list.
  // The random points are computed with a density of (coverage points)/1
  // The points are offsetted away from the triangles in
  // offset * positive normal direction.
  void addRandomSurfacePoints(float coverage, float offset,
                              osg::Texture2D* object_mask,
                              std::vector<SGVec3f>& points)
  {
    unsigned num = getNumTriangles();
    for (unsigned i = 0; i < num; ++i) {
      triangle_ref triangleRef = getTriangleRef(i);
      SGVec3f v0 = getVertex(triangleRef[0]).vertex;
      SGVec3f v1 = getVertex(triangleRef[1]).vertex;
      SGVec3f v2 = getVertex(triangleRef[2]).vertex;
      SGVec2f t0 = getVertex(triangleRef[0]).texCoord;
      SGVec2f t1 = getVertex(triangleRef[1]).texCoord;
      SGVec2f t2 = getVertex(triangleRef[2]).texCoord;
      SGVec3f normal = cross(v1 - v0, v2 - v0);
      
      // Compute the area
      float area = 0.5f*length(normal);
      if (area <= SGLimitsf::min())
        continue;
      
      // For partial units of area, use a zombie door method to
      // create the proper random chance of a light being created
      // for this triangle
      float unit = area + mt_rand(&seed)*coverage;
      
      SGVec3f offsetVector = offset*normalize(normal);
      // generate a light point for each unit of area

      while ( coverage < unit ) {

        float a = mt_rand(&seed);
        float b = mt_rand(&seed);

        if ( a + b > 1 ) {
          a = 1 - a;
          b = 1 - b;
        }
        float c = 1 - a - b;
        SGVec3f randomPoint = offsetVector + a*v0 + b*v1 + c*v2;
        
        if (object_mask != NULL) {
          SGVec2f texCoord = a*t0 + b*t1 + c*t2;
          
          // Check this random point against the object mask
          // red channel.
          osg::Image* img = object_mask->getImage();            
          unsigned int x = (int) (img->s() * texCoord.x()) % img->s();
          unsigned int y = (int) (img->t() * texCoord.y()) % img->t();
          
          if (mt_rand(&seed) < img->getColor(x, y).r()) {                
            points.push_back(randomPoint);        
          }                    
        } else {      
          // No object mask, so simply place the object  
          points.push_back(randomPoint);        
        }
        unit -= coverage;        
      }
    }
  }

  // Computes and adds random surface points to the points list for tree
  // coverage.
  void addRandomTreePoints(float wood_coverage, 
                           osg::Texture2D* object_mask,
                           float vegetation_density,
                           float cos_max_density_angle,
                           float cos_zero_density_angle,
                           std::vector<SGVec3f>& points)
  {
    unsigned num = getNumTriangles();
    for (unsigned i = 0; i < num; ++i) {
      triangle_ref triangleRef = getTriangleRef(i);
      SGVec3f v0 = getVertex(triangleRef[0]).vertex;
      SGVec3f v1 = getVertex(triangleRef[1]).vertex;
      SGVec3f v2 = getVertex(triangleRef[2]).vertex;
      SGVec2f t0 = getVertex(triangleRef[0]).texCoord;
      SGVec2f t1 = getVertex(triangleRef[1]).texCoord;
      SGVec2f t2 = getVertex(triangleRef[2]).texCoord;
      SGVec3f normal = cross(v1 - v0, v2 - v0);
      
      // Ensure the slope isn't too steep by checking the
      // cos of the angle between the slope normal and the
      // vertical (conveniently the z-component of the normalized
      // normal) and values passed in.                   
      float alpha = normalize(normal).z();
      float slope_density = 1.0;
      
      if (alpha < cos_zero_density_angle) 
        continue; // Too steep for any vegetation      
      
      if (alpha < cos_max_density_angle) {
        slope_density = 
          (alpha - cos_zero_density_angle) / (cos_max_density_angle - cos_zero_density_angle);
      }
      
      // Compute the area
      float area = 0.5f*length(normal);
      if (area <= SGLimitsf::min())
        continue;
        
      // Determine the number of trees, taking into account vegetation
      // density (which is linear) and the slope density factor.
      // Use a zombie door method to create the proper random chance 
      // of a tree being created for partial values.
      int woodcount = (int) (vegetation_density * vegetation_density * 
                             slope_density *
                             area / wood_coverage + mt_rand(&seed));
      
      for (int j = 0; j < woodcount; j++) {
        float a = mt_rand(&seed);
        float b = mt_rand(&seed);

        if ( a + b > 1.0f ) {
          a = 1.0f - a;
          b = 1.0f - b;
        }

        float c = 1.0f - a - b;

        SGVec3f randomPoint = a*v0 + b*v1 + c*v2;
        
        if (object_mask != NULL) {
          SGVec2f texCoord = a*t0 + b*t1 + c*t2;
          
          // Check this random point against the object mask
          // green (for trees) channel. 
          osg::Image* img = object_mask->getImage();            
          unsigned int x = (int) (img->s() * texCoord.x()) % img->s();
          unsigned int y = (int) (img->t() * texCoord.y()) % img->t();
          
          if (mt_rand(&seed) < img->getColor(x, y).g()) {  
            // The red channel contains the rotation for this object                                  
            points.push_back(randomPoint);
          }
        } else {
          points.push_back(randomPoint);
        }                
      }
    }
  }
  
   void addRandomPoints(double coverage, 
                        double spacing,
                        osg::Texture2D* object_mask,
                        std::vector<std::pair<SGVec3f, float> >& points)
  {
    unsigned num = getNumTriangles();
    for (unsigned i = 0; i < num; ++i) {
      triangle_ref triangleRef = getTriangleRef(i);
      SGVec3f v0 = getVertex(triangleRef[0]).vertex;
      SGVec3f v1 = getVertex(triangleRef[1]).vertex;
      SGVec3f v2 = getVertex(triangleRef[2]).vertex;
      SGVec2f t0 = getVertex(triangleRef[0]).texCoord;
      SGVec2f t1 = getVertex(triangleRef[1]).texCoord;
      SGVec2f t2 = getVertex(triangleRef[2]).texCoord;
      SGVec3f normal = cross(v1 - v0, v2 - v0);
      
      // Compute the area
      float area = 0.5f*length(normal);
      if (area <= SGLimitsf::min())
        continue;
      
      // for partial units of area, use a zombie door method to
      // create the proper random chance of an object being created
      // for this triangle.
      double num = area / coverage + mt_rand(&seed);

      // place an object each unit of area
      while ( num > 1.0 ) {
        float a = mt_rand(&seed);
        float b = mt_rand(&seed);
        if ( a + b > 1 ) {
          a = 1 - a;
          b = 1 - b;
        }
        float c = 1 - a - b;
        SGVec3f randomPoint = a*v0 + b*v1 + c*v2;
        
        // Check that the point is sufficiently far from
        // the edge of the triangle by measuring the distance
        // from the three lines that make up the triangle.        
        if (((length(cross(randomPoint - v0, randomPoint - v1)) / length(v1 - v0)) > spacing) &&
            ((length(cross(randomPoint - v1, randomPoint - v2)) / length(v2 - v1)) > spacing) &&
            ((length(cross(randomPoint - v2, randomPoint - v0)) / length(v0 - v2)) > spacing)   )
        {
          if (object_mask != NULL) {
            SGVec2f texCoord = a*t0 + b*t1 + c*t2;
            
            // Check this random point against the object mask
            // blue (for buildings) channel. 
            osg::Image* img = object_mask->getImage();            
            unsigned int x = (int) (img->s() * texCoord.x()) % img->s();
            unsigned int y = (int) (img->t() * texCoord.y()) % img->t();
            
            if (mt_rand(&seed) < img->getColor(x, y).b()) {  
              // The red channel contains the rotation for this object                                  
              points.push_back(std::make_pair(randomPoint, img->getColor(x,y).r()));
            }
          } else {
            points.push_back(std::make_pair(randomPoint, static_cast<float>(mt_rand(&seed))));
          }        
        }
        num -= 1.0;
      }
    }
  }

  osg::Geometry* buildGeometry(const TriangleVector& triangles) const
  {
    // Do not build anything if there is nothing in here ...
    if (empty() || triangles.empty())
      return 0;

    // FIXME: do not include all values here ...
    osg::Vec3Array* vertices = new osg::Vec3Array;
    osg::Vec3Array* normals = new osg::Vec3Array;
    osg::Vec2Array* texCoords = new osg::Vec2Array;

    osg::Vec4Array* colors = new osg::Vec4Array;
    colors->push_back(osg::Vec4(1, 1, 1, 1));

    osg::Geometry* geometry = new osg::Geometry;
    geometry->setVertexArray(vertices);
    geometry->setNormalArray(normals);
    geometry->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
    geometry->setColorArray(colors);
    geometry->setColorBinding(osg::Geometry::BIND_OVERALL);
    geometry->setTexCoordArray(0, texCoords);

    const unsigned invalid = ~unsigned(0);
    std::vector<unsigned> indexMap(getNumVertices(), invalid);

    DrawElementsFacade deFacade(vertices->size());
    for (index_type i = 0; i < triangles.size(); ++i) {
      triangle_ref triangle = triangles[i];
      if (indexMap[triangle[0]] == invalid) {
        indexMap[triangle[0]] = vertices->size();
        vertices->push_back(toOsg(getVertex(triangle[0]).vertex));
        normals->push_back(toOsg(getVertex(triangle[0]).normal));
        texCoords->push_back(toOsg(getVertex(triangle[0]).texCoord));
      }
      deFacade.push_back(indexMap[triangle[0]]);

      if (indexMap[triangle[1]] == invalid) {
        indexMap[triangle[1]] = vertices->size();
        vertices->push_back(toOsg(getVertex(triangle[1]).vertex));
        normals->push_back(toOsg(getVertex(triangle[1]).normal));
        texCoords->push_back(toOsg(getVertex(triangle[1]).texCoord));
      }
      deFacade.push_back(indexMap[triangle[1]]);

      if (indexMap[triangle[2]] == invalid) {
        indexMap[triangle[2]] = vertices->size();
        vertices->push_back(toOsg(getVertex(triangle[2]).vertex));
        normals->push_back(toOsg(getVertex(triangle[2]).normal));
        texCoords->push_back(toOsg(getVertex(triangle[2]).texCoord));
      }
      deFacade.push_back(indexMap[triangle[2]]);
    }
    geometry->addPrimitiveSet(deFacade.getDrawElements());

    return geometry;
  }

  osg::Geometry* buildGeometry() const
  { return buildGeometry(getTriangles()); }
  
  int getTextureIndex() const
  {
    if (empty() || getNumTriangles() == 0)
      return 0;

    triangle_ref triangleRef = getTriangleRef(0);
    SGVec3f v0 = getVertex(triangleRef[0]).vertex;
      
    return floor(v0.x());
  }

private:
  // Random seed for the triangle.
  mt seed;
};

#endif
