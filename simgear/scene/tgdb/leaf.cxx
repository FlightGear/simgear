// leaf.cxx -- function to build and ssg leaf from higher level data.
//
// Written by Curtis Olson, started October 1997.
//
// Copyright (C) 1997 - 2003  Curtis L. Olson  - curt@flightgear.org
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
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
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


static void random_pt_inside_tri( float *res,
                                  float *n1, float *n2, float *n3 )
{
    double a = sg_random();
    double b = sg_random();
    if ( a + b > 1.0 ) {
        a = 1.0 - a;
        b = 1.0 - b;
    }
    double c = 1 - a - b;

    res[0] = n1[0]*a + n2[0]*b + n3[0]*c;
    res[1] = n1[1]*a + n2[1]*b + n3[1]*c;
    res[2] = n1[2]*a + n2[2]*b + n3[2]*c;
}


void sgGenRandomSurfacePoints( ssgLeaf *leaf, double factor, 
                               ssgVertexArray *lights )
{
    int tris = leaf->getNumTriangles();
    if ( tris > 0 ) {
        short int n1, n2, n3;
        float *p1, *p2, *p3;
        sgVec3 result;

        // generate a repeatable random seed
        p1 = leaf->getVertex( 0 );
        unsigned int seed = (unsigned int)(fabs(p1[0]*100));
        sg_srandom( seed );

        for ( int i = 0; i < tris; ++i ) {
            leaf->getTriangle( i, &n1, &n2, &n3 );
            p1 = leaf->getVertex(n1);
            p2 = leaf->getVertex(n2);
            p3 = leaf->getVertex(n3);
            double area = sgTriArea( p1, p2, p3 );
            double num = area / factor;

            // generate a light point for each unit of area
            while ( num > 1.0 ) {
                random_pt_inside_tri( result, p1, p2, p3 );
                lights->add( result );
                num -= 1.0;
            }
            // for partial units of area, use a zombie door method to
            // create the proper random chance of a light being created
            // for this triangle
            if ( num > 0.0 ) {
                if ( sg_random() <= num ) {
                    // a zombie made it through our door
                    random_pt_inside_tri( result, p1, p2, p3 );
                    lights->add( result );
                }
            }
        }
    }
}


ssgVertexArray *sgGenRandomSurfacePoints( ssgLeaf *leaf, double factor ) {
    ssgVertexArray *result = new ssgVertexArray();
    sgGenRandomSurfacePoints( leaf, factor, result );

    return result;
}


////////////////////////////////////////////////////////////////////////
// Scenery loaders.
////////////////////////////////////////////////////////////////////////

ssgLeaf *sgMakeLeaf( const string& path,
                     const GLenum ty, 
                     SGMaterialLib *matlib, const string& material,
                     const point_list& nodes, const point_list& normals,
                     const point_list& texcoords,
                     const int_list& node_index,
                     const int_list& normal_index,
                     const int_list& tex_index,
                     const bool calc_lights, ssgVertexArray *lights )
{
    double tex_width = 1000.0, tex_height = 1000.0;
    ssgSimpleState *state = NULL;
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

    sgVec2 tmp2;
    sgVec3 tmp3;
    sgVec4 tmp4;
    int i;

    // vertices
    int size = node_index.size();
    if ( size < 1 ) {
        SG_LOG( SG_TERRAIN, SG_ALERT, "Woh! node list size < 1" );
        exit(-1);
    }
    ssgVertexArray *vl = new ssgVertexArray( size );
    Point3D node;
    for ( i = 0; i < size; ++i ) {
        node = nodes[ node_index[i] ];
        sgSetVec3( tmp3, node[0], node[1], node[2] );
        vl -> add( tmp3 );
    }

    // normals
    Point3D normal;
    ssgNormalArray *nl = new ssgNormalArray( size );
    if ( normal_index.size() ) {
        // object file specifies normal indices (i.e. normal indices
        // aren't 'implied'
        for ( i = 0; i < size; ++i ) {
            normal = normals[ normal_index[i] ];
            sgSetVec3( tmp3, normal[0], normal[1], normal[2] );
            nl -> add( tmp3 );
        }
    } else {
        // use implied normal indices.  normal index = vertex index.
        for ( i = 0; i < size; ++i ) {
            normal = normals[ node_index[i] ];
            sgSetVec3( tmp3, normal[0], normal[1], normal[2] );
            nl -> add( tmp3 );
        }
    }

    // colors
    ssgColourArray *cl = new ssgColourArray( 1 );
    sgSetVec4( tmp4, 1.0, 1.0, 1.0, 1.0 );
    cl->add( tmp4 );

    // texture coordinates
    size = tex_index.size();
    Point3D texcoord;
    ssgTexCoordArray *tl = new ssgTexCoordArray( size );
    if ( size == 1 ) {
        texcoord = texcoords[ tex_index[0] ];
        sgSetVec2( tmp2, texcoord[0], texcoord[1] );
        sgSetVec2( tmp2, texcoord[0], texcoord[1] );
        if ( tex_width > 0 ) {
            tmp2[0] *= (1000.0 / tex_width);
        }
        if ( tex_height > 0 ) {
            tmp2[1] *= (1000.0 / tex_height);
        }
        tl -> add( tmp2 );
    } else if ( size > 1 ) {
        for ( i = 0; i < size; ++i ) {
            texcoord = texcoords[ tex_index[i] ];
            sgSetVec2( tmp2, texcoord[0], texcoord[1] );
            if ( tex_width > 0 ) {
                tmp2[0] *= (1000.0 / tex_width);
            }
            if ( tex_height > 0 ) {
                tmp2[1] *= (1000.0 / tex_height);
            }
            tl -> add( tmp2 );
        }
    }

    ssgLeaf *leaf = new ssgVtxTable ( ty, vl, nl, tl, cl );

    // lookup the state record

    leaf->setState( state );

    if ( calc_lights ) {
        if ( coverage > 0.0 ) {
            if ( coverage < 10000.0 ) {
                SG_LOG(SG_INPUT, SG_ALERT, "Light coverage is "
                       << coverage << ", pushing up to 10000");
                coverage = 10000;
            }
            sgGenRandomSurfacePoints(leaf, coverage, lights );
        }
    }

    return leaf;
}
