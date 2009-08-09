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

#include <simgear/math/sg_random.h>
#include <simgear/math/SGMath.hxx>
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
                              std::vector<SGVec3f>& points)
  {
    unsigned num = getNumTriangles();
    for (unsigned i = 0; i < num; ++i) {
      triangle_ref triangleRef = getTriangleRef(i);
      SGVec3f v0 = getVertex(triangleRef[0]).vertex;
      SGVec3f v1 = getVertex(triangleRef[1]).vertex;
      SGVec3f v2 = getVertex(triangleRef[2]).vertex;
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
        points.push_back(randomPoint);
        unit -= coverage;
      }
    }
  }

  // Computes and adds random surface points to the points list for tree
  // coverage.
  void addRandomTreePoints(float wood_coverage, 
                           float tree_density,
                           float wood_size,
                           std::vector<SGVec3f>& points)
  {
    unsigned num = getNumTriangles();
    for (unsigned i = 0; i < num; ++i) {
      triangle_ref triangleRef = getTriangleRef(i);
      SGVec3f v0 = getVertex(triangleRef[0]).vertex;
      SGVec3f v1 = getVertex(triangleRef[1]).vertex;
      SGVec3f v2 = getVertex(triangleRef[2]).vertex;
      SGVec3f normal = cross(v1 - v0, v2 - v0);
      
      // Compute the area
      float area = 0.5f*length(normal);
      if (area <= SGLimitsf::min())
        continue;
      
      // For partial units of area, use a zombie door method to
      // create the proper random chance of a point being created
      // for this triangle
      float unit = area + mt_rand(&seed)*wood_coverage;

      int woodcount = (int) (unit / wood_coverage);

      for (unsigned j = 0; j < woodcount; j++) {

        if (wood_size < area) {
          // We need to place a wood within the triangle and populate it

          // Determine the center of the wood
          float x = mt_rand(&seed);
          float y = mt_rand(&seed);

          // Determine the size of this wood in m^2, and the number
          // of trees in the wood
          float ws = wood_size + wood_size * (mt_rand(&seed) - 0.5f);
          unsigned total_trees = ws / tree_density;
          float wood_length = sqrt(ws);

          // From our wood size, work out the fraction on the two axis.
          // This will be used as a factor when placing trees in the wood.
          float x_tree_factor = wood_length / length(v1 -v0);
          float y_tree_factor = wood_length / length(v2 -v0);

          for (unsigned k = 0; k <= total_trees; k++) {

            float a = x + x_tree_factor * (mt_rand(&seed) - 0.5f);
            float b = y + y_tree_factor * (mt_rand(&seed) - 0.5f);


            // In some cases, the triangle side lengths are so small that the
            // tree_factors become so large as to make placing the tree within
            // the triangle almost impossible. In this case, we place them
            // randomly across the triangle.
            if (a < 0.0f || a > 1.0f) a = mt_rand(&seed);
            if (b < 0.0f || b > 1.0f) b = mt_rand(&seed);
            
            if ( a + b > 1.0f ) {
              a = 1.0f - a;
              b = 1.0f - b;
            }
              
            float c = 1.0f - a - b;

            SGVec3f randomPoint = a*v0 + b*v1 + c*v2;

            points.push_back(randomPoint);
          }
        } else {
          // This triangle is too small to contain a complete wood, so just
          // distribute trees across it.
          unsigned total_trees = area / tree_density;

          for (unsigned k = 0; k <= total_trees; k++) {

            float a = mt_rand(&seed);
            float b = mt_rand(&seed);

            if ( a + b > 1.0f ) {
              a = 1.0f - a;
              b = 1.0f - b;
            }

            float c = 1.0f - a - b;

            SGVec3f randomPoint = a*v0 + b*v1 + c*v2;
            points.push_back(randomPoint);
          }
        }
      }
    }
  }
  
   void addRandomPoints(float coverage, 
                        std::vector<SGVec3f>& points)
  {
    unsigned num = getNumTriangles();
    for (unsigned i = 0; i < num; ++i) {
      triangle_ref triangleRef = getTriangleRef(i);
      SGVec3f v0 = getVertex(triangleRef[0]).vertex;
      SGVec3f v1 = getVertex(triangleRef[1]).vertex;
      SGVec3f v2 = getVertex(triangleRef[2]).vertex;
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
        points.push_back(randomPoint);
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
        vertices->push_back(getVertex(triangle[0]).vertex.osg());
        normals->push_back(getVertex(triangle[0]).normal.osg());
        texCoords->push_back(getVertex(triangle[0]).texCoord.osg());
      }
      deFacade.push_back(indexMap[triangle[0]]);

      if (indexMap[triangle[1]] == invalid) {
        indexMap[triangle[1]] = vertices->size();
        vertices->push_back(getVertex(triangle[1]).vertex.osg());
        normals->push_back(getVertex(triangle[1]).normal.osg());
        texCoords->push_back(getVertex(triangle[1]).texCoord.osg());
      }
      deFacade.push_back(indexMap[triangle[1]]);

      if (indexMap[triangle[2]] == invalid) {
        indexMap[triangle[2]] = vertices->size();
        vertices->push_back(getVertex(triangle[2]).vertex.osg());
        normals->push_back(getVertex(triangle[2]).normal.osg());
        texCoords->push_back(getVertex(triangle[2]).texCoord.osg());
      }
      deFacade.push_back(indexMap[triangle[2]]);
    }
    geometry->addPrimitiveSet(deFacade.getDrawElements());

    return geometry;
  }

  osg::Geometry* buildGeometry() const
  { return buildGeometry(getTriangles()); }

private:
  // Random seed for the triangle.
  mt seed;
};

#endif
