/* -*-c++-*-
 *
 * Copyright (C) 2006-2007 Mathias Froehlich, Tim Moore
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

#include <math.h>
#include <simgear/compiler.h>

#include <osg/Geode>
#include <osg/Geometry>
#include <osg/MatrixTransform>
#include <osg/StateSet>

#include <simgear/bucket/newbucket.hxx>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/math/sg_types.hxx>
#include <simgear/misc/texcoord.hxx>
#include <simgear/scene/material/Effect.hxx>
#include <simgear/scene/material/EffectGeode.hxx>
#include <simgear/scene/material/mat.hxx>
#include <simgear/scene/material/matlib.hxx>
#include <simgear/scene/model/BoundingVolumeBuildVisitor.hxx>
#include <simgear/scene/util/OsgMath.hxx>
#include <simgear/scene/util/VectorArrayAdapter.hxx>
#include <simgear/scene/util/SGNodeMasks.hxx>

using namespace simgear;
// Ocean tile with curvature and apron to hide cracks. The cracks are
// mostly with adjoining coastal tiles that assume a flat ocean
// between corners of a tile; they also hide the micro cracks between
// adjoining ocean tiles. This is probably over-engineered, but it
// serves as a testbed for some things that will come later.

// Helper class for building and accessing the mesh. The layout of the
// points in the mesh is a little wacky. First is the bottom row of
// the points for the apron. Next is the left apron point, the points
// in the mesh, and the right apron point, for each of the rows of the
// mesh; the points for the top apron come last. This order should
// help with things like vertex caching in the OpenGL driver, though
// it may be superfluous for such a small mesh.
namespace
{
class OceanMesh {
public:
    OceanMesh(int latP, int lonP):
        latPoints(latP),
        lonPoints(lonP),
        geoPoints(latPoints * lonPoints + 2 * (lonPoints + latPoints)),
        geod_nodes(latPoints * lonPoints),
        vl(new osg::Vec3Array(geoPoints)),
        nl(new osg::Vec3Array(geoPoints)),
        tl(new osg::Vec2Array(geoPoints)),
        vlArray(*vl, lonPoints + 2, lonPoints, 1),
        nlArray(*nl, lonPoints + 2, lonPoints, 1),
        tlArray(*tl, lonPoints + 2, lonPoints, 1)
    {
        int numPoints = latPoints * lonPoints;
        geod = new SGGeod[numPoints];
        normals = new SGVec3f[numPoints];
        rel = new SGVec3d[numPoints];
    }
    
    ~OceanMesh()
    {
        delete[] geod;
        delete[] normals;
        delete[] rel;
    }
    
    const int latPoints, lonPoints;
    const int geoPoints;
    SGGeod* geod;
    SGVec3f* normals;
    SGVec3d* rel;

    std::vector<SGGeod> geod_nodes;

    osg::Vec3Array* vl;
    osg::Vec3Array* nl;
    osg::Vec2Array* tl;
    VectorArrayAdapter<osg::Vec3Array> vlArray;
    VectorArrayAdapter<osg::Vec3Array> nlArray;
    VectorArrayAdapter<osg::Vec2Array> tlArray;

    void calcMesh(const SGVec3d& cartCenter, const SGQuatd& orient,
                  double clon, double clat,
                  double height, double width, double tex_width);
    void calcApronPt(int latIdx, int lonIdx, int latInner, int lonInner,
                     int destIdx, double tex_width);
    void calcApronPts(double tex_width);
    
};

void OceanMesh::calcMesh(const SGVec3d& cartCenter, const SGQuatd& orient,
                         double clon, double clat,
                         double height, double width, double tex_width)
{
    // Calculate vertices. By splitting the tile up into 4 quads on a
    // side we avoid curvature-of-the-earth problems; the error should
    // be less than .5 meters.
    double longInc = width * .25;
    double latInc = height * .25;
    double startLat = clat - height * .5;
    double startLon = clon - width * .5;
    for (int j = 0; j < latPoints; j++) {
        double lat = startLat + j * latInc;
        for (int i = 0; i < lonPoints; i++) {
            int index = (j * lonPoints) + i;
            geod[index] = SGGeod::fromDeg(startLon + i * longInc, lat);
            SGVec3d cart = SGVec3d::fromGeod(geod[index]);
            rel[index] = orient.transform(cart - cartCenter);
            normals[index] = toVec3f(orient.transform(normalize(cart)));
        }
    }
    
    // Calculate texture coordinates
    typedef std::vector<SGGeod> GeodVector;
    
    GeodVector geod_nodes(latPoints * lonPoints);
    VectorArrayAdapter<GeodVector> geodNodesArray(geod_nodes, lonPoints);
    int_list rectangle(latPoints * lonPoints);
    VectorArrayAdapter<int_list> rectArray(rectangle, lonPoints);
    for (int j = 0; j < latPoints; j++) {
        for (int i = 0; i < lonPoints; i++) {
            int index = (j * lonPoints) + i;
            geodNodesArray(j, i) = geod[index];
            rectArray(j, i) = index;
        }
    }
    
    typedef std::vector<SGVec2f> Vec2Array;
    Vec2Array texs = sgCalcTexCoords( clat, geod_nodes, rectangle, 
                                       1000.0 / tex_width );
                                
                                       
    VectorArrayAdapter<Vec2Array> texsArray(texs, lonPoints);
  
    for (int j = 0; j < latPoints; j++) {
        for (int i = 0; i < lonPoints; ++i) {
            int index = (j * lonPoints) + i;
            vlArray(j, i) = toOsg(rel[index]);
            nlArray(j, i) = toOsg(normals[index]);
            tlArray(j, i) = toOsg(texsArray(j, i));
        }
    }

}

// Apron points. For each point on the edge we'll go 150
// metres "down" and 40 metres "out" to create a nice overlap. The
// texture should be applied according to this dimension. The
// normals of the apron polygons will be the same as the those of
// the points on the edge to better disguise the apron.
void OceanMesh::calcApronPt(int latIdx, int lonIdx, int latInner, int lonInner,
                            int destIdx, double tex_width)
{
    static const float downDist = 150.0f;
    static const float outDist = 40.0f;
    // Get vector along edge, in the right direction to make a cross
    // product with the normal vector that will point out from the
    // mesh.
    osg::Vec3f edgePt = vlArray(latIdx, lonIdx);
    osg::Vec3f edgeVec;
    if (lonIdx == lonInner) {   // bottom or top edge
        if (lonIdx > 0)
            edgeVec = vlArray(latIdx, lonIdx - 1) - edgePt;
        else
            edgeVec = edgePt - vlArray(latIdx, lonIdx + 1);
        if (latIdx > latInner)
            edgeVec = -edgeVec;  // Top edge
    } else {                     // right or left edge
        if (latIdx > 0)
            edgeVec = edgePt - vlArray(latIdx - 1, lonIdx);
        else
            edgeVec = vlArray(latIdx + 1, lonIdx) - edgePt;
        if (lonIdx > lonInner)  // right edge
            edgeVec = -edgeVec;
    }
    edgeVec.normalize();
    osg::Vec3f outVec = nlArray(latIdx, lonIdx) ^ edgeVec;
    (*vl)[destIdx]
        = edgePt - nlArray(latIdx, lonIdx) * downDist + outVec * outDist;
    (*nl)[destIdx] = nlArray(latIdx, lonIdx);
    static const float apronDist
        = sqrtf(downDist * downDist  + outDist * outDist);
    float texDelta = apronDist / tex_width;
    if (lonIdx == lonInner) {
        if (latIdx > latInner)
            (*tl)[destIdx]
                = tlArray(latIdx, lonIdx) + osg::Vec2f(0.0f, texDelta);
        else
            (*tl)[destIdx]
                = tlArray(latIdx, lonIdx) - osg::Vec2f(0.0f, texDelta);
    } else {
        if (lonIdx > lonInner)
            (*tl)[destIdx]
                = tlArray(latIdx, lonIdx) + osg::Vec2f(texDelta, 0.0f);
        else
            (*tl)[destIdx]
                = tlArray(latIdx, lonIdx) - osg::Vec2f(texDelta, 0.0f);
    }
}

void OceanMesh::calcApronPts(double tex_width)
{
    for (int i = 0; i < lonPoints; i++)
        calcApronPt(0, i, 1, i, i, tex_width);
    int topApronOffset = latPoints + (2 + lonPoints) * latPoints;
    for (int i = 0; i < lonPoints; i++)
        calcApronPt(latPoints - 1, i, latPoints - 2, i,
                    i + topApronOffset, tex_width);
    for (int i = 0; i < latPoints; i++) {
        calcApronPt(i, 0, i, 1, lonPoints + i * (lonPoints + 2), tex_width);
        calcApronPt(i, lonPoints - 1, i, lonPoints - 2,
                    lonPoints + i * (lonPoints + 2) + 1 + lonPoints, tex_width);
    }
}

// Enter the vertices of triangles that fill one row of the
// mesh. The vertices are entered in counter-clockwise order.
void fillDrawElementsRow(int width, short row0Start, short row1Start,
                         osg::DrawElementsUShort::vector_type::iterator&
                         elements)
{
    short row0Idx = row0Start;
    short row1Idx = row1Start;
    for (int i = 0; i < width - 1; i++, row0Idx++, row1Idx++) {
        *elements++ = row0Idx;
        *elements++ = row0Idx + 1;
        *elements++ = row1Idx;
        *elements++ = row1Idx;
        *elements++ = row0Idx + 1;
        *elements++ = row1Idx + 1;
    }
}

void fillDrawElementsWithApron(short height, short width,
                               osg::DrawElementsUShort::vector_type::iterator
                               elements)
{
    // First apron row
    fillDrawElementsRow(width, 0, width + 1, elements);
    for (short i = 0; i < height - 1; i++)
        fillDrawElementsRow(width + 2, width + i * (width + 2),
                            width + (i + 1) * (width + 2),
                            elements);
    // Last apron row
    short topApronBottom = width + (height - 1) * (width + 2) + 1;
    fillDrawElementsRow(width, topApronBottom, topApronBottom + width + 1,
                        elements);
}
}

osg::Node* SGOceanTile(const SGBucket& b, SGMaterialLib *matlib, int latPoints, int lonPoints)
{
    Effect *effect = 0;

    double tex_width = 1000.0;
  
    // find Ocean material in the properties list
    SGMaterialCache* matcache = matlib->generateMatCache(b.get_center());
    SGMaterial* mat = matcache->find( "Ocean" );
    delete matcache;

    if ( mat != NULL ) {
        // set the texture width and height values for this
        // material
        tex_width = mat->get_xsize();
    
        // set OSG State
        effect = mat->get_effect();
    } else {
        SG_LOG( SG_TERRAIN, SG_ALERT, "Ack! unknown use material name = Ocean");
    }
    OceanMesh grid(latPoints, lonPoints);
    // Calculate center point
    SGVec3d cartCenter = SGVec3d::fromGeod(b.get_center());
    SGGeod geodPos = SGGeod::fromCart(cartCenter);
    SGQuatd hlOr = SGQuatd::fromLonLat(geodPos)*SGQuatd::fromEulerDeg(0, 0, 180);

    double clon = b.get_center_lon();
    double clat = b.get_center_lat();
    double height = b.get_height();
    double width = b.get_width();

    grid.calcMesh(cartCenter, hlOr, clon, clat, height, width, tex_width);
    grid.calcApronPts(tex_width);
  
    osg::Vec4Array* cl = new osg::Vec4Array;
    cl->push_back(osg::Vec4(1, 1, 1, 1));
  
    osg::Geometry* geometry = new osg::Geometry;
    geometry->setDataVariance(osg::Object::STATIC);
    geometry->setVertexArray(grid.vl);
    geometry->setNormalArray(grid.nl);
    geometry->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
    geometry->setColorArray(cl);
    geometry->setColorBinding(osg::Geometry::BIND_OVERALL);
    geometry->setTexCoordArray(0, grid.tl);

    // Allocate the indices for triangles in the mesh and the apron
    osg::DrawElementsUShort* drawElements
        = new osg::DrawElementsUShort(GL_TRIANGLES,
                                      6 * ((latPoints - 1) * (lonPoints + 1)
                                           + 2 * (latPoints - 1)));
    fillDrawElementsWithApron(latPoints, lonPoints, drawElements->begin());
    geometry->addPrimitiveSet(drawElements);

    EffectGeode* geode = new EffectGeode;
    geode->setName("Ocean tile");
    geode->setEffect(effect);
    geode->addDrawable(geometry);
    geode->runGenerators(geometry);

    osg::MatrixTransform* transform = new osg::MatrixTransform;
    transform->setName("Ocean");
    transform->setMatrix(osg::Matrix::rotate(toOsg(hlOr))*
                         osg::Matrix::translate(toOsg(cartCenter)));
    transform->addChild(geode);
    transform->setNodeMask( ~(simgear::CASTSHADOW_BIT | simgear::MODELLIGHT_BIT) );

    // Create a BVH at this point.  This is normally provided by the file loader, but as we create the 
    // geometry programmatically, no file loader is involved.
    BoundingVolumeBuildVisitor bvhBuilder(false);
    transform->accept(bvhBuilder);
    
    return transform;
}
