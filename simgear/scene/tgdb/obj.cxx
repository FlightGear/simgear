// obj.cxx -- routines to handle loading scenery and building the plib
//            scene graph.
//
// Written by Curtis Olson, started October 1997.
//
// Copyright (C) 1997  Curtis L. Olson  - http://www.flightgear.org/~curt
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

#include <list>

#include STL_STRING

#include <osg/Depth>
#include <osg/Fog>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Group>
#include <osg/LOD>
#include <osg/MatrixTransform>
#include <osg/StateSet>

#include <osgUtil/Optimizer>
#include <osgDB/WriteFile>

#include <simgear/bucket/newbucket.hxx>
#include <simgear/io/sg_binobj.hxx>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/math/sg_random.h>
#include <simgear/math/sg_types.hxx>
#include <simgear/misc/texcoord.hxx>
#include <simgear/scene/material/mat.hxx>
#include <simgear/scene/material/matlib.hxx>
#include <simgear/scene/util/SGUpdateVisitor.hxx>
#include <simgear/scene/util/SGNodeMasks.hxx>
#include <simgear/scene/tgdb/leaf.hxx>
#include <simgear/scene/tgdb/pt_lights.hxx>
#include <simgear/scene/tgdb/userdata.hxx>

#include "obj.hxx"

SG_USING_STD(string);
SG_USING_STD(list);

struct Leaf {
    GLenum type;
    int index;
};


// Generate an ocean tile
bool SGGenTile( const string& path, const SGBucket& b,
                SGMaterialLib *matlib, osg::Group* group )
{
    osg::StateSet *state = 0;

    double tex_width = 1000.0;

    // find Ocean material in the properties list
    SGMaterial *mat = matlib->find( "Ocean" );
    if ( mat != NULL ) {
        // set the texture width and height values for this
        // material
        tex_width = mat->get_xsize();
        
        // set ssgState
        state = mat->get_state();
    } else {
        SG_LOG( SG_TERRAIN, SG_ALERT, 
                "Ack! unknown usemtl name = " << "Ocean" 
                << " in " << path );
    }

    // Calculate center point
    SGVec3d cartCenter = SGVec3d::fromGeod(b.get_center());
    Point3D center = Point3D(cartCenter[0], cartCenter[1], cartCenter[2]);

    double clon = b.get_center_lon();
    double clat = b.get_center_lat();
    double height = b.get_height();
    double width = b.get_width();

    // Caculate corner vertices
    Point3D geod[4];
    geod[0] = Point3D( clon - width/2.0, clat - height/2.0, 0.0 );
    geod[1] = Point3D( clon + width/2.0, clat - height/2.0, 0.0 );
    geod[2] = Point3D( clon + width/2.0, clat + height/2.0, 0.0 );
    geod[3] = Point3D( clon - width/2.0, clat + height/2.0, 0.0 );

    Point3D rad[4];
    int i;
    for ( i = 0; i < 4; ++i ) {
        rad[i] = Point3D( geod[i].x() * SGD_DEGREES_TO_RADIANS,
                          geod[i].y() * SGD_DEGREES_TO_RADIANS,
                          geod[i].z() );
    }

    Point3D cart[4], rel[4];
    for ( i = 0; i < 4; ++i ) {
        cart[i] = sgGeodToCart(rad[i]);
        rel[i] = cart[i] - center;
        // cout << "corner " << i << " = " << cart[i] << endl;
    }

    // Calculate normals
    Point3D normals[4];
    for ( i = 0; i < 4; ++i ) {
        double length = cart[i].distance3D( Point3D(0.0) );
        normals[i] = cart[i] / length;
        // cout << "normal = " << normals[i] << endl;
    }

    // Calculate texture coordinates
    point_list geod_nodes;
    geod_nodes.clear();
    geod_nodes.reserve(4);
    int_list rectangle;
    rectangle.clear();
    rectangle.reserve(4);
    for ( i = 0; i < 4; ++i ) {
        geod_nodes.push_back( geod[i] );
        rectangle.push_back( i );
    }
    point_list texs = sgCalcTexCoords( b, geod_nodes, rectangle, 
                                       1000.0 / tex_width );

    // Allocate ssg structure
    osg::Vec3Array *vl = new osg::Vec3Array;
    osg::Vec3Array *nl = new osg::Vec3Array;
    osg::Vec2Array *tl = new osg::Vec2Array;

    for ( i = 0; i < 4; ++i ) {
        vl->push_back(osg::Vec3(rel[i].x(), rel[i].y(), rel[i].z()));
        nl->push_back(osg::Vec3(normals[i].x(), normals[i].y(), normals[i].z()));
        tl->push_back(osg::Vec2(texs[i].x(), texs[i].y()));
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
    geometry->addPrimitiveSet(new osg::DrawArrays(GL_TRIANGLE_FAN, 0, vl->size()));
    osg::Geode* geode = new osg::Geode;
    geode->setName(path);
    geode->addDrawable(geometry);
    geode->setStateSet(state);

    group->addChild(geode);

    return true;
}


/**
 * SSG callback for an in-range leaf of randomly-placed objects.
 *
 * This pretraversal callback is attached to a branch that is
 * traversed only when a leaf is in range.  If the leaf is not
 * currently prepared to be populated with randomly-placed objects,
 * this callback will prepare it (actual population is handled by
 * the tri_in_range_callback for individual triangles).
 *
 * @param entity The entity to which the callback is attached (not used).
 * @param mask The entity's traversal mask (not used).
 * @return Always 1, to allow traversal and culling to continue.
 */
// static int
// leaf_in_range_callback (ssgEntity * entity, int mask)
// {
//   SGLeafUserData * data = (SGLeafUserData *)entity->getUserData();

//   if (!data->is_filled_in) {
//                                 // Iterate through all the triangles
//                                 // and populate them.
//     int num_tris = data->leaf->getNumTriangles();
//     for ( int i = 0; i < num_tris; ++i ) {
//             data->setup_triangle(i);
//     }
//     data->is_filled_in = true;
//   }
//   return 1;
// }


/**
 * SSG callback for an out-of-range leaf of randomly-placed objects.
 *
 * This pretraversal callback is attached to a branch that is
 * traversed only when a leaf is out of range.  If the leaf is
 * currently prepared to be populated with randomly-placed objects (or
 * is actually populated), the objects will be removed.
 *
 * @param entity The entity to which the callback is attached (not used).
 * @param mask The entity's traversal mask (not used).
 * @return Always 0, to prevent any further traversal or culling.
 */
// static int
// leaf_out_of_range_callback (ssgEntity * entity, int mask)
// {
//   SGLeafUserData * data = (SGLeafUserData *)entity->getUserData();
//   if (data->is_filled_in) {
//     data->branch->removeAllKids();
//     data->is_filled_in = false;
//   }
//   return 0;
// }


/**
 * Randomly place objects on a surface.
 *
 * The leaf node provides the geometry of the surface, while the
 * material provides the objects and placement density.  Latitude
 * and longitude are required so that the objects can be rotated
 * to the world-up vector.  This function does not actually add
 * any objects; instead, it attaches an ssgRangeSelector to the
 * branch with callbacks to generate the objects when needed.
 *
 * @param leaf The surface where the objects should be placed.
 * @param branch The branch that will hold the randomly-placed objects.
 * @param center The center of the leaf in FlightGear coordinates.
 * @param material_name The name of the surface's material.
 */
static void
gen_random_surface_objects (osg::Drawable *leaf,
                            osg::Group *branch,
                            Point3D *center,
                            SGMaterial *mat )
{
  // OSGFIXME
#if 0
                                // If the surface has no triangles, return
                                // now.
    int num_tris = leaf->getNumTriangles();
    if (num_tris < 1)
        return;

                                // If the material has no randomly-placed
                                // objects, return now.
    if (mat->get_object_group_count() < 1)
        return;

                                // Calculate the geodetic centre of
                                // the tile, for aligning automatic
                                // objects.
    double xyz[3], lon_rad, lat_rad, alt_m;
    xyz[0] = center->x(); xyz[1] = center->y(); xyz[2] = center->z();
    sgCartToGeod(xyz, &lat_rad, &lon_rad, &alt_m);

                                // LOD for the leaf
                                // max random object range: 20000m
    osg::LOD * lod = new osg::LOD;
    branch->addChild(lod);

                                // Create the in-range and out-of-range
                                // branches.
    osg::Group * in_range = new osg::Group;
//     osg::Group * out_of_range = new osg::Group;
    lod->addChild(in_range, 0, 20000 /*OSGFIXME hardcoded visibility ???*/);
//     lod->addChild(out_of_range, 20000, 1e30);

    SGLeafUserData * data = new SGLeafUserData;
    data->is_filled_in = false;
    data->leaf = leaf;
    data->mat = mat;
    data->branch = in_range;
    data->sin_lat = sin(lat_rad);
    data->cos_lat = cos(lat_rad);
    data->sin_lon = sin(lon_rad);
    data->cos_lon = cos(lon_rad);

    in_range->setUserData(data);
    // OSGFIXME: implement random objects to be loaded when in sight
//     in_range->setTravCallback(SSG_CALLBACK_PRETRAV, leaf_in_range_callback);

    // OSGFIXME: implement deletion of tiles that are no longer used
//     out_of_range->setUserData(data);
//     out_of_range->setTravCallback(SSG_CALLBACK_PRETRAV,
//                                    leaf_out_of_range_callback);
//     out_of_range
//       ->addChild(new SGDummyBSphereEntity(leaf->getBSphere()->getRadius()));
#endif
}


// Ok, somehow polygon offset for lights ...
// Could never make the polygon offset for our lights get right.
// So, why not in this way ...
class SGLightOffsetTransform : public osg::Transform {
public:
#define SCALE_FACTOR 0.94
  virtual bool computeLocalToWorldMatrix(osg::Matrix& matrix,
                                         osg::NodeVisitor* nv) const
  {
    if (nv && nv->getVisitorType() == osg::NodeVisitor::CULL_VISITOR) {
      double scaleFactor = SCALE_FACTOR;
      osg::Vec3 center = nv->getEyePoint();
      osg::Matrix transform;
      transform(0,0) = scaleFactor;
      transform(1,1) = scaleFactor;
      transform(2,2) = scaleFactor;
      transform(3,0) = center[0]*(1 - scaleFactor);
      transform(3,1) = center[1]*(1 - scaleFactor);
      transform(3,2) = center[2]*(1 - scaleFactor);
      matrix.preMult(transform);
    }
    return true;
  }
  virtual bool computeWorldToLocalMatrix(osg::Matrix& matrix,
                                         osg::NodeVisitor* nv) const
  {
    if (nv && nv->getVisitorType() == osg::NodeVisitor::CULL_VISITOR) {
      double scaleFactor = 1/SCALE_FACTOR;
      osg::Vec3 center = nv->getEyePoint();
      osg::Matrix transform;
      transform(0,0) = scaleFactor;
      transform(1,1) = scaleFactor;
      transform(2,2) = scaleFactor;
      transform(3,0) = center[0]*(1 - scaleFactor);
      transform(3,1) = center[1]*(1 - scaleFactor);
      transform(3,2) = center[2]*(1 - scaleFactor);
      matrix.postMult(transform);
    }
    return true;
  }
#undef SCALE_FACTOR
};

static SGMaterial* findMaterial(const std::string& material,
                                const std::string& path,
                                SGMaterialLib *matlib)
{
  SGMaterial *mat = matlib->find( material );
  if (mat)
    return mat;

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
  return mat;
}


////////////////////////////////////////////////////////////////////////
// Scenery loaders.
////////////////////////////////////////////////////////////////////////

// Load an Binary obj file
static bool SGBinObjLoad( const string& path, const bool is_base,
                   Point3D& center,
                   SGMaterialLib *matlib,
                   bool use_random_objects,
                   osg::Group *local_terrain,
                   osg::Group *vasi_lights,
                   osg::Group *rwy_lights,
                   osg::Group *taxi_lights,
                   osg::Vec3Array *ground_lights )
{
    SGBinObject obj;

    if ( ! obj.read_bin( path ) ) {
        return false;
    }

    // reference point (center offset/bounding sphere)
    center = obj.get_gbs_center();

    point_list const& nodes = obj.get_wgs84_nodes();
    point_list const& normals = obj.get_normals();
    point_list const& texcoords = obj.get_texcoords();

    string material;
    int_list tex_index;

    group_list::size_type i;

    osg::Geode* geode = new osg::Geode;
    local_terrain->addChild( geode );

    // generate points
    string_list const& pt_materials = obj.get_pt_materials();
    group_list const& pts_v = obj.get_pts_v();
    group_list const& pts_n = obj.get_pts_n();
    for ( i = 0; i < pts_v.size(); ++i ) {
        // cout << "pts_v.size() = " << pts_v.size() << endl;
        if ( pt_materials[i].substr(0, 3) == "RWY" ) {
            // airport environment lighting
            SGVec3d up(center.x(), center.y(), center.z());
            // returns a transform -> lod -> leaf structure
            osg::Node *branch = SGMakeDirectionalLights( nodes, normals,
                                                         pts_v[i], pts_n[i],
                                                         matlib,
                                                         pt_materials[i], up );
            if ( pt_materials[i] == "RWY_VASI_LIGHTS" ) {
                vasi_lights->addChild( branch );
            } else if ( pt_materials[i] == "RWY_BLUE_TAXIWAY_LIGHTS"
                || pt_materials[i] == "RWY_GREEN_TAXIWAY_LIGHTS" )
            {
                taxi_lights->addChild( branch );
            } else {
                rwy_lights->addChild( branch );
            }
        } else {
            // other geometry
            SGMaterial *mat = findMaterial( pt_materials[i], path, matlib );
            tex_index.clear();
            osg::Drawable *leaf = SGMakeLeaf( path, GL_POINTS, mat,
                                        nodes, normals, texcoords,
                                        pts_v[i], pts_n[i], tex_index,
                                        false, ground_lights );
            


            geode->addDrawable( leaf );
        }
    }

    // Put all randomly-placed objects under a separate branch
    // (actually an ssgRangeSelector) named "random-models".
    osg::Group * random_object_branch = 0;
//     if (use_random_objects) {
//         osg::LOD* object_lod = new osg::LOD;
//         object_lod->setName("random-models");
//         geometry->addChild(object_lod);
//         random_object_branch = new osg::Group;
//         // Maximum 20km range for random objects
//         object_lod->addChild(random_object_branch, 0, 20000);
//     }

    typedef map<string,list<Leaf> > LeafMap;
    LeafMap leafMap;
    Leaf leaf;
    leaf.type = GL_TRIANGLES;
    string_list const& tri_materials = obj.get_tri_materials();
    group_list const& tris_v = obj.get_tris_v();
    group_list const& tris_n = obj.get_tris_n();
    group_list const& tris_tc = obj.get_tris_tc();
    for ( i = 0; i < tris_v.size(); i++ ) {
        leaf.index = i;
        leafMap[ tri_materials[i] ].push_back( leaf );
    }
    leaf.type = GL_TRIANGLE_STRIP;
    string_list const& strip_materials = obj.get_strip_materials();
    group_list const& strips_v = obj.get_strips_v();
    group_list const& strips_n = obj.get_strips_n();
    group_list const& strips_tc = obj.get_strips_tc();
    for ( i = 0; i < strips_v.size(); i++ ) {
        leaf.index = i;
        leafMap[ strip_materials[i] ].push_back( leaf );
    }
    leaf.type = GL_TRIANGLE_FAN;
    string_list const& fan_materials = obj.get_fan_materials();
    group_list const& fans_v = obj.get_fans_v();
    group_list const& fans_n = obj.get_fans_n();
    group_list const& fans_tc = obj.get_fans_tc();
    for ( i = 0; i < fans_v.size(); i++ ) {
        leaf.index = i;
        leafMap[ fan_materials[i] ].push_back( leaf );
    }

    LeafMap::iterator lmi = leafMap.begin();
    while ( lmi != leafMap.end() ) {
        SGMaterial *mat = findMaterial( lmi->first, path, matlib );
        list<Leaf> &leaf_list = lmi->second;
        list<Leaf>::iterator li = leaf_list.begin();
        while ( li != leaf_list.end() ) {
            Leaf &leaf = *li;
            int ind = leaf.index;
            if ( leaf.type == GL_TRIANGLES ) {
                osg::Drawable *leaf = SGMakeLeaf( path, GL_TRIANGLES, mat,
                                            nodes, normals, texcoords,
                                            tris_v[ind], tris_n[ind], tris_tc[ind],
                                            is_base, ground_lights );
                if ( random_object_branch ) {
                    if ( mat ) {
                        gen_random_surface_objects( leaf, random_object_branch,
                                                    &center, mat );
                    }
                }
                geode->addDrawable( leaf );
            } else if ( leaf.type == GL_TRIANGLE_STRIP ) {
                osg::Drawable *leaf = SGMakeLeaf( path, GL_TRIANGLE_STRIP, mat,
                                            nodes, normals, texcoords,
                                            strips_v[ind], strips_n[ind], strips_tc[ind],
                                            is_base, ground_lights );
                if ( random_object_branch ) {
                    if ( mat ) {
                        gen_random_surface_objects( leaf, random_object_branch,
                                                    &center, mat );
                    }
                }
                geode->addDrawable( leaf );
            } else {
                osg::Drawable *leaf = SGMakeLeaf( path, GL_TRIANGLE_FAN, mat,
                                            nodes, normals, texcoords,
                                            fans_v[ind], fans_n[ind], fans_tc[ind],
                                            is_base, ground_lights );
                if ( random_object_branch ) {
                    if ( mat ) {
                        gen_random_surface_objects( leaf, random_object_branch,
                                                    &center, mat );
                    }
                }
                geode->addDrawable( leaf );
            }
            ++li;
        }
        ++lmi;
    }

    return true;
}




static osg::Node*
gen_lights( SGMaterialLib *matlib, osg::Vec3Array *lights, int inc, float bright )
{
    // generate a repeatable random seed
    sg_srandom( (unsigned)(*lights)[0][0] );

    // Allocate ssg structure
    osg::Vec3Array *vl = new osg::Vec3Array;
    osg::Vec4Array *cl = new osg::Vec4Array;

    for ( unsigned i = 0; i < lights->size(); ++i ) {
        // this loop is slightly less efficient than it otherwise
        // could be, but we want a red light to always be red, and a
        // yellow light to always be yellow, etc. so we are trying to
        // preserve the random sequence.
        float zombie = sg_random();
        if ( i % inc == 0 ) {
            vl->push_back( (*lights)[i] );

            // factor = sg_random() ^ 2, range = 0 .. 1 concentrated towards 0
            float factor = sg_random();
            factor *= factor;

            osg::Vec4 color;
            if ( zombie > 0.5 ) {
                // 50% chance of yellowish
                color = osg::Vec4( 0.9, 0.9, 0.3, bright - factor * 0.2 );
            } else if ( zombie > 0.15 ) {
                // 35% chance of whitish
                color = osg::Vec4( 0.9, 0.9, 0.8, bright - factor * 0.2 );
            } else if ( zombie > 0.05 ) {
                // 10% chance of orangish
                color = osg::Vec4( 0.9, 0.6, 0.2, bright - factor * 0.2 );
            } else {
                // 5% chance of redish
                color = osg::Vec4( 0.9, 0.2, 0.2, bright - factor * 0.2 );
            }
            cl->push_back( color );
        }
    }

    // create ssg leaf
    osg::Geometry* geometry = new osg::Geometry;
    geometry->setVertexArray(vl);
    geometry->setColorArray(cl);
    geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    geometry->addPrimitiveSet(new osg::DrawArrays(GL_POINTS, 0, vl->size()));
    osg::Geode* geode = new osg::Geode;
    geode->addDrawable(geometry);

    // assign state
    SGMaterial *mat = matlib->find( "GROUND_LIGHTS" );
    geometry->setStateSet(mat->get_state());

    return geode;
}

class SGTileUpdateCallback : public osg::NodeCallback {
public:
  virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
  {
    assert(dynamic_cast<osg::Switch*>(node));
    assert(dynamic_cast<SGUpdateVisitor*>(nv));

    osg::Switch* lightSwitch = static_cast<osg::Switch*>(node);
    SGUpdateVisitor* updateVisitor = static_cast<SGUpdateVisitor*>(nv);

    // The current sun angle in degree
    float sun_angle = updateVisitor->getSunAngleDeg();

    // vasi is always on
    lightSwitch->setValue(0, true);
    if (sun_angle > 85 || updateVisitor->getVisibility() < 5000) {
      // runway and taxi
      lightSwitch->setValue(1, true);
      lightSwitch->setValue(2, true);
    } else {
      // runway and taxi
      lightSwitch->setValue(1, false);
      lightSwitch->setValue(2, false);
    }
    
    // ground lights
    if ( sun_angle > 95 )
      lightSwitch->setValue(5, true);
    else
      lightSwitch->setValue(5, false);
    if ( sun_angle > 92 )
      lightSwitch->setValue(4, true);
    else
      lightSwitch->setValue(4, false);
    if ( sun_angle > 89 )
      lightSwitch->setValue(3, true);
    else
      lightSwitch->setValue(3, false);

    traverse(node, nv);
  }
};

class SGRunwayLightFogUpdateCallback : public osg::StateAttribute::Callback {
public:
  virtual void operator () (osg::StateAttribute* sa, osg::NodeVisitor* nv)
  {
    assert(dynamic_cast<SGUpdateVisitor*>(nv));
    assert(dynamic_cast<osg::Fog*>(sa));
    SGUpdateVisitor* updateVisitor = static_cast<SGUpdateVisitor*>(nv);
    osg::Fog* fog = static_cast<osg::Fog*>(sa);
    fog->setMode(osg::Fog::EXP2);
    fog->setColor(updateVisitor->getFogColor().osg());
    fog->setDensity(updateVisitor->getRunwayFogExp2Density());
  }
};

class SGTaxiLightFogUpdateCallback : public osg::StateAttribute::Callback {
public:
  virtual void operator () (osg::StateAttribute* sa, osg::NodeVisitor* nv)
  {
    assert(dynamic_cast<SGUpdateVisitor*>(nv));
    assert(dynamic_cast<osg::Fog*>(sa));
    SGUpdateVisitor* updateVisitor = static_cast<SGUpdateVisitor*>(nv);
    osg::Fog* fog = static_cast<osg::Fog*>(sa);
    fog->setMode(osg::Fog::EXP2);
    fog->setColor(updateVisitor->getFogColor().osg());
    fog->setDensity(updateVisitor->getTaxiFogExp2Density());
  }
};

class SGGroundLightFogUpdateCallback : public osg::StateAttribute::Callback {
public:
  virtual void operator () (osg::StateAttribute* sa, osg::NodeVisitor* nv)
  {
    assert(dynamic_cast<SGUpdateVisitor*>(nv));
    assert(dynamic_cast<osg::Fog*>(sa));
    SGUpdateVisitor* updateVisitor = static_cast<SGUpdateVisitor*>(nv);
    osg::Fog* fog = static_cast<osg::Fog*>(sa);
    fog->setMode(osg::Fog::EXP2);
    fog->setColor(updateVisitor->getFogColor().osg());
    fog->setDensity(updateVisitor->getGroundLightsFogExp2Density());
  }
};


osg::Node*
SGLoadBTG(const std::string& path, SGMaterialLib *matlib, bool calc_lights, bool use_random_objects)
{
  osg::Group* vasiLights = new osg::Group;
  osg::StateSet* stateSet = vasiLights->getOrCreateStateSet();
  osg::Fog* fog = new osg::Fog;
  fog->setUpdateCallback(new SGRunwayLightFogUpdateCallback);
  stateSet->setAttribute(fog);

  osg::Group* rwyLights = new osg::Group;
  stateSet = rwyLights->getOrCreateStateSet();
  fog = new osg::Fog;
  fog->setUpdateCallback(new SGRunwayLightFogUpdateCallback);
  stateSet->setAttribute(fog);

  osg::Group* taxiLights = new osg::Group;
  stateSet = taxiLights->getOrCreateStateSet();
  fog = new osg::Fog;
  fog->setUpdateCallback(new SGTaxiLightFogUpdateCallback);
  stateSet->setAttribute(fog);

  osg::Group* groundLights0 = new osg::Group;
  stateSet = groundLights0->getOrCreateStateSet();
  fog = new osg::Fog;
  fog->setUpdateCallback(new SGGroundLightFogUpdateCallback);
  stateSet->setAttribute(fog);

  osg::Group* groundLights1 = new osg::Group;
  stateSet = groundLights1->getOrCreateStateSet();
  fog = new osg::Fog;
  fog->setUpdateCallback(new SGGroundLightFogUpdateCallback);
  stateSet->setAttribute(fog);

  osg::Group* groundLights2 = new osg::Group;
  stateSet = groundLights2->getOrCreateStateSet();
  fog = new osg::Fog;
  fog->setUpdateCallback(new SGGroundLightFogUpdateCallback);
  stateSet->setAttribute(fog);

  osg::Switch* lightSwitch = new osg::Switch;
  lightSwitch->setUpdateCallback(new SGTileUpdateCallback);
  lightSwitch->addChild(vasiLights, true);
  lightSwitch->addChild(rwyLights, true);
  lightSwitch->addChild(taxiLights, true);
  lightSwitch->addChild(groundLights0, true);
  lightSwitch->addChild(groundLights1, true);
  lightSwitch->addChild(groundLights2, true);

  osg::Group* lightGroup = new SGLightOffsetTransform;
  lightGroup->addChild(lightSwitch);
  unsigned nodeMask = ~0u;
  nodeMask &= ~SG_NODEMASK_CASTSHADOW_BIT;
  nodeMask &= ~SG_NODEMASK_RECIEVESHADOW_BIT;
  nodeMask &= ~SG_NODEMASK_PICK_BIT;
  nodeMask &= ~SG_NODEMASK_TERRAIN_BIT;
  lightGroup->setNodeMask(nodeMask);
  
  osg::Group* terrainGroup = new osg::Group;

  osg::ref_ptr<osg::Vec3Array> light_pts = new osg::Vec3Array;
  Point3D center;
  SGBinObjLoad(path, calc_lights, center, matlib, use_random_objects,
               terrainGroup, vasiLights, rwyLights, taxiLights, light_pts.get());

  if ( light_pts->size() ) {
    osg::Node *lights;
    
    lights = gen_lights( matlib, light_pts.get(), 4, 0.7 );
    groundLights0->addChild( lights );
    
    lights = gen_lights( matlib, light_pts.get(), 2, 0.85 );
    groundLights1->addChild( lights );
    
    lights = gen_lights( matlib, light_pts.get(), 1, 1.0 );
    groundLights2->addChild( lights );
  }

  // The toplevel transform for that tile.
  osg::MatrixTransform* transform = new osg::MatrixTransform;
  transform->setName(path);
  transform->setMatrix(osg::Matrix::translate(osg::Vec3d(center[0], center[1], center[2])));
  transform->addChild(terrainGroup);
  transform->addChild(lightGroup);

  return transform;
}
