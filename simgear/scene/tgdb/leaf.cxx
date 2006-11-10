// leaf.cxx -- function to build and ssg leaf from higher level data.
//
// Written by Curtis Olson, started October 1997.
//
// Copyright (C) 1997 - 2003  Curtis L. Olson  - http://www.flightgear.org/~curt
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
// $Id$


#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include <simgear/compiler.h>

#ifdef SG_MATH_EXCEPTION_CLASH
#  include <math.h>
#endif

#include STL_STRING

#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Group>
#include <osg/StateSet>
#include <osg/TriangleFunctor>

#include <simgear/debug/logstream.hxx>
#include <simgear/math/sg_random.h>
#include <simgear/scene/material/mat.hxx>
#include <simgear/scene/material/matlib.hxx>

#include "leaf.hxx"

SG_USING_STD(string);
SG_USING_STD(vector);


typedef vector < int > int_list;
typedef int_list::iterator int_list_iterator;
typedef int_list::const_iterator int_point_list_iterator;


static inline
osg::Vec3 random_pt_inside_tri(const osg::Vec3& n1, const osg::Vec3& n2,
                               const osg::Vec3& n3 )
{
    double a = sg_random();
    double b = sg_random();
    if ( a + b > 1.0 ) {
        a = 1.0 - a;
        b = 1.0 - b;
    }
    double c = 1 - a - b;

    return n1*a + n2*b + n3*c;
}

/// class to implement the TrinagleFunctor class
struct SGRandomSurfacePointsFill {
  osg::Vec3Array* lights;
  float factor;

  void operator () (const osg::Vec3& v1, const osg::Vec3& v2,
                    const osg::Vec3& v3, bool)
  {
    // Compute the area
    float area = 0.5*((v1 - v2)^(v3 - v2)).length();
    float num = area / factor;
    
    // generate a light point for each unit of area
    while ( num > 1.0 ) {
      lights->push_back(random_pt_inside_tri( v1, v2, v3 ));
      num -= 1.0;
    }
    // for partial units of area, use a zombie door method to
    // create the proper random chance of a light being created
    // for this triangle
    if ( num > 0.0 ) {
      if ( sg_random() <= num ) {
        // a zombie made it through our door
        lights->push_back(random_pt_inside_tri( v1, v2, v3 ));
      }
    }
  }
};

static void SGGenRandomSurfacePoints( osg::Geometry *leaf, double factor, 
                                      osg::Vec3Array *lights )
{
  osg::TriangleFunctor<SGRandomSurfacePointsFill> triangleFunctor;
  triangleFunctor.lights = lights;
  triangleFunctor.factor = factor;
  leaf->accept(triangleFunctor);
}


////////////////////////////////////////////////////////////////////////
// Scenery loaders.
////////////////////////////////////////////////////////////////////////

osg::Node* SGMakeLeaf( const string& path,
                     const GLenum ty, 
                     SGMaterialLib *matlib, const string& material,
                     const point_list& nodes, const point_list& normals,
                     const point_list& texcoords,
                     const int_list& node_index,
                     const int_list& normal_index,
                     const int_list& tex_index,
                     const bool calc_lights, osg::Vec3Array *lights )
{
    double tex_width = 1000.0, tex_height = 1000.0;
    osg::StateSet *state = 0;
    float coverage = -1;

    SGMaterial *mat = matlib->find( material );
    if ( mat == NULL ) {
        // see if this is an on the fly texture
        string file = path;
        string::size_type pos = file.rfind( "/" );
        file = file.substr( 0, pos );
        // cout << "current file = " << file << endl;
        file += "/";
        file += material;
        // cout << "current file = " << file << endl;
        if ( ! matlib->add_item( file ) ) {
            SG_LOG( SG_TERRAIN, SG_ALERT, 
                    "Ack! unknown usemtl name = " << material 
                    << " in " << path );
        } else {
            // locate our newly created material
            mat = matlib->find( material );
            if ( mat == NULL ) {
                SG_LOG( SG_TERRAIN, SG_ALERT, 
                        "Ack! bad on the fly material create = "
                        << material << " in " << path );
            }
        }
    }

    if ( mat != NULL ) {
        // set the texture width and height values for this
        // material
        tex_width = mat->get_xsize();
        tex_height = mat->get_ysize();
        state = mat->get_state();
        coverage = mat->get_light_coverage();
        // cout << "(w) = " << tex_width << " (h) = "
        //      << tex_width << endl;
    } else {
        coverage = -1;
    }

    int i;

    // vertices
    int size = node_index.size();
    if ( size < 1 ) {
        SG_LOG( SG_TERRAIN, SG_ALERT, "Woh! node list size < 1" );
        exit(-1);
    }
    osg::Vec3Array* vl = new osg::Vec3Array;
    vl->reserve(size);
    for ( i = 0; i < size; ++i ) {
        Point3D node = nodes[ node_index[i] ];
        vl->push_back(osg::Vec3(node[0], node[1], node[2]));
    }

    // normals
    osg::Vec3Array* nl = new osg::Vec3Array;
    nl->reserve(size);
    if ( normal_index.size() ) {
        // object file specifies normal indices (i.e. normal indices
        // aren't 'implied'
        for ( i = 0; i < size; ++i ) {
            Point3D normal = normals[ normal_index[i] ];
            nl->push_back(osg::Vec3(normal[0], normal[1], normal[2]));
        }
    } else {
        // use implied normal indices.  normal index = vertex index.
        for ( i = 0; i < size; ++i ) {
            Point3D normal = normals[ node_index[i] ];
            nl->push_back(osg::Vec3(normal[0], normal[1], normal[2]));
        }
    }

    // colors
    osg::Vec4Array* cl = new osg::Vec4Array;
    cl->push_back(osg::Vec4(1, 1, 1, 1));

    // texture coordinates
    size = tex_index.size();
    Point3D texcoord;
    osg::Vec2Array* tl = new osg::Vec2Array;
    tl->reserve(size);
    if ( size == 1 ) {
        Point3D texcoord = texcoords[ tex_index[0] ];
        osg::Vec2 tmp2(texcoord[0], texcoord[1]);
        if ( tex_width > 0 ) {
            tmp2[0] *= (1000.0 / tex_width);
        }
        if ( tex_height > 0 ) {
            tmp2[1] *= (1000.0 / tex_height);
        }
        tl -> push_back( tmp2 );
    } else if ( size > 1 ) {
        for ( i = 0; i < size; ++i ) {
            Point3D texcoord = texcoords[ tex_index[i] ];
            osg::Vec2 tmp2(texcoord[0], texcoord[1]);
            if ( tex_width > 0 ) {
                tmp2[0] *= (1000.0 / tex_width);
            }
            if ( tex_height > 0 ) {
                tmp2[1] *= (1000.0 / tex_height);
            }
            tl -> push_back( tmp2 );
        }
    }


    osg::Geometry* geometry = new osg::Geometry;
    geometry->setVertexArray(vl);
    geometry->setNormalArray(nl);
    geometry->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
    geometry->setColorArray(cl);
    geometry->setColorBinding(osg::Geometry::BIND_OVERALL);
    geometry->setTexCoordArray(0, tl);
    geometry->addPrimitiveSet(new osg::DrawArrays(ty, 0, vl->size()));
    osg::Geode* geode = new osg::Geode;
    geode->addDrawable(geometry);

    // lookup the state record
    geode->setStateSet(state);
    geode->setUserData( new SGMaterialUserData(mat) );

    if ( calc_lights ) {
        if ( coverage > 0.0 ) {
            if ( coverage < 10000.0 ) {
                SG_LOG(SG_INPUT, SG_ALERT, "Light coverage is "
                       << coverage << ", pushing up to 10000");
                coverage = 10000;
            }
            SGGenRandomSurfacePoints(geometry, coverage, lights );
        }
    }

    return geode;
}
