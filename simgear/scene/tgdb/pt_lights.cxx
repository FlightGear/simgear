// pt_lights.cxx -- build a 'directional' light on the fly
//
// Written by Curtis Olson, started March 2002.
//
// Copyright (C) 2002  Curtis L. Olson  - http://www.flightgear.org/~curt
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

#include <osg/Array>
#include <osg/Geometry>
#include <osg/Geode>
#include <osg/LOD>
#include <osg/MatrixTransform>
#include <osg/NodeCallback>
#include <osg/NodeVisitor>
#include <osg/Switch>

#include <simgear/scene/material/mat.hxx>
#include <simgear/screen/extensions.hxx>
#include <simgear/math/sg_random.h>

#include "vasi.hxx"

#include "pt_lights.hxx"

// static variables for use in ssg callbacks
bool SGPointLightsUseSprites = false;
bool SGPointLightsEnhancedLighting = false;
bool SGPointLightsDistanceAttenuation = false;


// Specify the way we want to draw directional point lights (assuming the
// appropriate extensions are available.)

void SGConfigureDirectionalLights( bool use_point_sprites,
                                   bool enhanced_lighting,
                                   bool distance_attenuation ) {
    SGPointLightsUseSprites = use_point_sprites;
    SGPointLightsEnhancedLighting = enhanced_lighting;
    SGPointLightsDistanceAttenuation = distance_attenuation;
}

static void calc_center_point( const point_list &nodes,
                               const int_list &pnt_i,
                               Point3D& result ) {
    double minx = nodes[pnt_i[0]][0];
    double maxx = nodes[pnt_i[0]][0];
    double miny = nodes[pnt_i[0]][1];
    double maxy = nodes[pnt_i[0]][1];
    double minz = nodes[pnt_i[0]][2];
    double maxz = nodes[pnt_i[0]][2];

    for ( unsigned int i = 0; i < pnt_i.size(); ++i ) {
        Point3D pt = nodes[pnt_i[i]];
        if ( pt[0] < minx ) { minx = pt[0]; }
        if ( pt[0] > maxx ) { minx = pt[0]; }
        if ( pt[1] < miny ) { miny = pt[1]; }
        if ( pt[1] > maxy ) { miny = pt[1]; }
        if ( pt[2] < minz ) { minz = pt[2]; }
        if ( pt[2] > maxz ) { minz = pt[2]; }
    }

    result = Point3D((minx + maxx) / 2.0, (miny + maxy) / 2.0,
                     (minz + maxz) / 2.0);
}


static osg::Node*
gen_dir_light_group( const point_list &nodes,
                     const point_list &normals,
                     const int_list &pnt_i,
                     const int_list &nml_i,
                     const SGMaterial *mat,
                     const osg::Vec3& up, bool vertical, bool vasi )
{
    Point3D center;
    calc_center_point( nodes, pnt_i, center );
    // cout << center[0] << "," << center[1] << "," << center[2] << endl;


    // find a vector perpendicular to the normal.
    osg::Vec3 perp1;
    if ( !vertical ) {
        // normal isn't vertical so we can use up as our first vector
        perp1 = up;
    } else {
        // normal is vertical so we have to work a bit harder to
        // determine our first vector
        osg::Vec3 pt1(nodes[pnt_i[0]][0], nodes[pnt_i[0]][1],
                      nodes[pnt_i[0]][2] );
        osg::Vec3 pt2(nodes[pnt_i[1]][0], nodes[pnt_i[1]][1],
                      nodes[pnt_i[1]][2] );

        perp1 = pt2 - pt1;
    }
    perp1.normalize();

    osg::Vec3Array *vl = new osg::Vec3Array;
    osg::Vec3Array *nl = new osg::Vec3Array;
    osg::Vec4Array *cl = new osg::Vec4Array;

    unsigned int i;
    for ( i = 0; i < pnt_i.size(); ++i ) {
        Point3D ppt = nodes[pnt_i[i]] - center;
        osg::Vec3 pt(ppt[0], ppt[1], ppt[2]);
        osg::Vec3 normal(normals[nml_i[i]][0], normals[nml_i[i]][1],
                         normals[nml_i[i]][2] );

        // calculate a vector perpendicular to dir and up
        osg::Vec3 perp2 = normal^perp1;

        // front face
        osg::Vec3 tmp3 = pt;
        vl->push_back( tmp3 );
        tmp3 += perp1;
        vl->push_back( tmp3 );
        tmp3 += perp2;
        vl->push_back( tmp3 );

        nl->push_back( normal );
        nl->push_back( normal );
        nl->push_back( normal );

        cl->push_back(osg::Vec4(1, 1, 1, 1));
        cl->push_back(osg::Vec4(1, 1, 1, 0));
        cl->push_back(osg::Vec4(1, 1, 1, 0));
    }

    osg::Geometry* geometry = new osg::Geometry;
    geometry->setName("Dir Lights " + mat->get_names().front());
    geometry->setVertexArray(vl);
    geometry->setNormalArray(nl);
    geometry->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
    geometry->setColorArray(cl);
    geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    geometry->addPrimitiveSet(new osg::DrawArrays(GL_TRIANGLES, 0, vl->size()));
    osg::Geode* geode = new osg::Geode;
    geode->addDrawable(geometry);

    if (vasi) {
        // this one is dynamic in its colors, so do not bother with dlists
        geometry->setUseDisplayList(false);
        geometry->setUseVertexBufferObjects(false);
        osg::Vec3 dir(normals[nml_i[0]][0], normals[nml_i[0]][1],
                      normals[nml_i[0]][2]);

        // calculate the reference position of this vasi and use it
        // to init the vasi structure
        Point3D ppt = nodes[pnt_i[0]] - center;
        osg::Vec3 pt(ppt[0], ppt[1], ppt[2]);
        // up is the "up" vector which is also
        // the reference center point of this tile.  The reference
        // center + the coordinate of the first light gives the actual
        // location of the first light.
        pt += up;

        // Set up the callback
        geode->setCullCallback(new SGVasiUpdateCallback(cl, pt, up, dir));
    }

    if ( mat != NULL ) {
        geode->setStateSet(mat->get_state());
    } else {
        SG_LOG( SG_TERRAIN, SG_ALERT, "Warning: material = NULL" );
    }

    // put an LOD on each lighting component
    osg::LOD *lod = new osg::LOD;
    lod->addChild( geode, 0, 20000 );

    // create the transformation.
    osg::MatrixTransform *trans = new osg::MatrixTransform;
    trans->setMatrix(osg::Matrixd::translate(osg::Vec3d(center[0], center[1], center[2])));
    trans->addChild( lod );

    return trans;
}

static osg::Node *gen_reil_lights( const point_list &nodes,
                                   const point_list &normals,
                                   const int_list &pnt_i,
                                   const int_list &nml_i,
                                   SGMaterialLib *matlib,
                                   const osg::Vec3& up )
{
    Point3D center;
    calc_center_point( nodes, pnt_i, center );
    // cout << center[0] << "," << center[1] << "," << center[2] << endl;

    osg::Vec3 nup = up;
    nup.normalize();

    osg::Vec3Array   *vl = new osg::Vec3Array;
    osg::Vec3Array   *nl = new osg::Vec3Array;
    osg::Vec4Array   *cl = new osg::Vec4Array;

    unsigned int i;
    for ( i = 0; i < pnt_i.size(); ++i ) {
        Point3D ppt = nodes[pnt_i[i]] - center;
        osg::Vec3 pt(ppt[0], ppt[1], ppt[2]);
        osg::Vec3 normal(normals[nml_i[i]][0], normals[nml_i[i]][1],
                         normals[nml_i[i]][2] );

        // calculate a vector perpendicular to dir and up
        osg::Vec3 perp = normal^up;

        // front face
        osg::Vec3 tmp3 = pt;
        vl->push_back( tmp3 );
        tmp3 += nup;
        vl->push_back( tmp3 );
        tmp3 += perp;
        vl->push_back( tmp3 );

        nl->push_back( normal );
        nl->push_back( normal );
        nl->push_back( normal );

        cl->push_back(osg::Vec4(1, 1, 1, 1));
        cl->push_back(osg::Vec4(1, 1, 1, 0));
        cl->push_back(osg::Vec4(1, 1, 1, 0));
    }

    SGMaterial *mat = matlib->find( "RWY_WHITE_LIGHTS" );

    osg::Geometry* geometry = new osg::Geometry;
    geometry->setName("Reil Lights " + mat->get_names().front());
    geometry->setVertexArray(vl);
    geometry->setNormalArray(nl);
    geometry->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
    geometry->setColorArray(cl);
    geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    geometry->addPrimitiveSet(new osg::DrawArrays(GL_TRIANGLES, 0, vl->size()));
    osg::Geode* geode = new osg::Geode;
    geode->addDrawable(geometry);

    if ( mat != NULL ) {
        geode->setStateSet( mat->get_state() );
    } else {
        SG_LOG( SG_TERRAIN, SG_ALERT,
                "Warning: can't find material = RWY_WHITE_LIGHTS" );
    }

    // OSGFIXME
//     leaf->setCallback( SSG_CALLBACK_PREDRAW, StrobePreDraw );
//     leaf->setCallback( SSG_CALLBACK_POSTDRAW, StrobePostDraw );

    // OSGFIXME: implement an update callback that switches on/off
    // based on the osg::FrameStamp
    osg::Switch *reil = new osg::Switch;
//     reil->setDuration( 60 );
//     reil->setLimits( 0, 2 );
//     reil->setMode( SSG_ANIM_SHUTTLE );
//     reil->control( SSG_ANIM_START );

    // need to add this twice to work around an ssg bug
    reil->addChild(geode, true);
   
    // put an LOD on each lighting component
    osg::LOD *lod = new osg::LOD;
    lod->addChild( reil, 0, 12000 /*OSGFIXME: hardcoded here?*/);

    // create the transformation.
    osg::MatrixTransform *trans = new osg::MatrixTransform;
    trans->setMatrix(osg::Matrixd::translate(osg::Vec3d(center[0], center[1], center[2])));
    trans->addChild( lod );

    return trans;
}


static osg::Node *gen_odals_lights( const point_list &nodes,
                                               const point_list &normals,
                                               const int_list &pnt_i,
                                               const int_list &nml_i,
                                               SGMaterialLib *matlib,
                                               const osg::Vec3& up )
{
    Point3D center;
    calc_center_point( nodes, pnt_i, center );
    // cout << center[0] << "," << center[1] << "," << center[2] << endl;

    // OSGFIXME: implement like above
//     osg::Switch *odals = new osg::Switch;
    osg::Group *odals = new osg::Group;

    // we don't want directional lights here
    SGMaterial *mat = matlib->find( "GROUND_LIGHTS" );
    if ( mat == NULL ) {
        SG_LOG( SG_TERRAIN, SG_ALERT,
                "Warning: can't material = GROUND_LIGHTS" );
    }

    osg::Vec3Array   *vl = new osg::Vec3Array;
    osg::Vec4Array   *cl = new osg::Vec4Array;
     
    cl->push_back(osg::Vec4(1, 1, 1, 1));

    // center line strobes
    for ( unsigned i = pnt_i.size() - 1; i >= 2; --i ) {
        Point3D ppt = nodes[pnt_i[i]] - center;
        osg::Vec3 pt(ppt[0], ppt[1], ppt[2]);
        vl->push_back(pt);
    }

    // runway end strobes
     
    Point3D ppt = nodes[pnt_i[0]] - center;
    osg::Vec3 pt(ppt[0], ppt[1], ppt[2]);
    vl->push_back(pt);

    ppt = nodes[pnt_i[1]] - center;
    pt = osg::Vec3(ppt[0], ppt[1], ppt[2]);
    vl->push_back(pt);

    osg::Geometry* geometry = new osg::Geometry;
    geometry->setName("Odal Lights " + mat->get_names().front());
    geometry->setVertexArray(vl);
    geometry->setColorArray(cl);
    geometry->setColorBinding(osg::Geometry::BIND_OVERALL);
    geometry->addPrimitiveSet(new osg::DrawArrays(GL_POINTS, 0, vl->size()));
    osg::Geode* geode = new osg::Geode;
    geode->addDrawable(geometry);

    geode->setStateSet( mat->get_state() );
    // OSGFIXME
//         leaf->setCallback( SSG_CALLBACK_PREDRAW, StrobePreDraw );
//         leaf->setCallback( SSG_CALLBACK_POSTDRAW, StrobePostDraw );

    odals->addChild( geode );

    // setup animition

//     odals->setDuration( 10 );
//     odals->setLimits( 0, pnt_i.size() - 1 );
//     odals->setMode( SSG_ANIM_SHUTTLE );
//     odals->control( SSG_ANIM_START );
   
    // put an LOD on each lighting component
    osg::LOD *lod = new osg::LOD;
    lod->addChild( odals, 0, 12000 /*OSGFIXME hardcoded visibility*/ );

    // create the transformation.
    osg::MatrixTransform *trans = new osg::MatrixTransform;
    trans->setMatrix(osg::Matrixd::translate(osg::Vec3d(center[0], center[1], center[2])));
    trans->addChild(lod);

    return trans;
}

class SGRabbitUpdateCallback : public osg::NodeCallback {
public:
  SGRabbitUpdateCallback(double duration) :
    mBaseTime(sg_random()), mDuration(duration)
  {
    if (fabs(mDuration) < 1e-3)
      mDuration = 1e-3;
    mBaseTime -= mDuration*floor(mBaseTime/mDuration);
  }

  virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
  {
    assert(dynamic_cast<osg::Switch*>(node));
    osg::Switch* sw = static_cast<osg::Switch*>(node);
    double frameTime = nv->getFrameStamp()->getReferenceTime();
    double timeDiff = (frameTime - mBaseTime)/mDuration;
    double reminder = timeDiff - unsigned(floor(timeDiff));
    unsigned nChildren = sw->getNumChildren();
    unsigned activeChild = unsigned(nChildren*reminder);
    if (nChildren <= activeChild)
      activeChild = nChildren;
    sw->setSingleChildOn(activeChild);

    osg::NodeCallback::operator()(node, nv);
  }
public:
  double mBaseTime;
  double mDuration;
};


static osg::Node *gen_rabbit_lights( const point_list &nodes,
                                     const point_list &normals,
                                     const int_list &pnt_i,
                                     const int_list &nml_i,
                                     SGMaterialLib *matlib,
                                     const osg::Vec3& up )
{
    Point3D center;
    calc_center_point( nodes, pnt_i, center );
    // cout << center[0] << "," << center[1] << "," << center[2] << endl;

    osg::Vec3 nup = up;
    nup.normalize();

    // OSGFIXME: implement like above ...
    osg::Switch *rabbit = new osg::Switch;
    rabbit->setUpdateCallback(new SGRabbitUpdateCallback(10));

    SGMaterial *mat = matlib->find( "RWY_WHITE_LIGHTS" );
    if ( mat == NULL ) {
        SG_LOG( SG_TERRAIN, SG_ALERT,
                "Warning: can't material = RWY_WHITE_LIGHTS" );
    }

    for ( int i = pnt_i.size() - 1; i >= 0; --i ) {
        osg::Vec3Array   *vl = new osg::Vec3Array;
        osg::Vec3Array   *nl = new osg::Vec3Array;
        osg::Vec4Array   *cl = new osg::Vec4Array;


        Point3D ppt = nodes[pnt_i[i]] - center;
        osg::Vec3 pt(ppt[0], ppt[1], ppt[2]);
        osg::Vec3 normal(normals[nml_i[i]][0], normals[nml_i[i]][1],
                         normals[nml_i[i]][2] );

        // calculate a vector perpendicular to dir and up
        osg::Vec3 perp = normal^nup;

        // front face
        osg::Vec3 tmp3 = pt;
        vl->push_back( tmp3 );
        tmp3 += nup;
        vl->push_back( tmp3 );
        tmp3 += perp;
        vl->push_back( tmp3 );

        nl->push_back(normal);

        cl->push_back(osg::Vec4(1, 1, 1, 1));
        cl->push_back(osg::Vec4(1, 1, 1, 0));
        cl->push_back(osg::Vec4(1, 1, 1, 0));

        osg::Geometry* geometry = new osg::Geometry;
        geometry->setName("Rabbit Lights " + mat->get_names().front());
        geometry->setVertexArray(vl);
        geometry->setNormalArray(nl);
        geometry->setNormalBinding(osg::Geometry::BIND_OVERALL);
        geometry->setColorArray(cl);
        geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
        geometry->addPrimitiveSet(new osg::DrawArrays(GL_TRIANGLES, 0, vl->size()));
        osg::Geode* geode = new osg::Geode;
        geode->addDrawable(geometry);
        
        geode->setStateSet( mat->get_state() );

        // OSGFIXME
//         leaf->setCallback( SSG_CALLBACK_PREDRAW, StrobePreDraw );
//         leaf->setCallback( SSG_CALLBACK_POSTDRAW, StrobePostDraw );

        rabbit->addChild( geode );
    }

//     rabbit->setDuration( 10 );
//     rabbit->setLimits( 0, pnt_i.size() - 1 );
//     rabbit->setMode( SSG_ANIM_SHUTTLE );
//     rabbit->control( SSG_ANIM_START );
   
    // put an LOD on each lighting component
    osg::LOD *lod = new osg::LOD;
    lod->addChild( rabbit, 0, 12000 /*OSGFIXME: hadcoded*/ );

    // create the transformation.
    osg::MatrixTransform *trans = new osg::MatrixTransform;
    trans->setMatrix(osg::Matrixd::translate(osg::Vec3d(center[0], center[1], center[2])));
    trans->addChild(lod);

    return trans;
}


osg::Node *SGMakeDirectionalLights( const point_list &nodes,
                                    const point_list &normals,
                                    const int_list &pnt_i,
                                    const int_list &nml_i,
                                    SGMaterialLib *matlib,
                                    const string &material,
                                    const SGVec3d& dup )
{
    osg::Vec3 nup = toVec3f(dup).osg();
    nup.normalize();

    SGMaterial *mat = matlib->find( material );

    if ( material == "RWY_REIL_LIGHTS" ) {
        // cout << "found a reil" << endl;
        return gen_reil_lights( nodes, normals, pnt_i, nml_i,
                                matlib, nup );
    } else if ( material == "RWY_ODALS_LIGHTS" ) {
        // cout << "found a odals" << endl;
        return gen_odals_lights( nodes, normals, pnt_i, nml_i,
                                 matlib, nup );
    } else if ( material == "RWY_SEQUENCED_LIGHTS" ) {
        // cout << "found a rabbit" << endl;
        return gen_rabbit_lights( nodes, normals, pnt_i, nml_i,
                                  matlib, nup );
    } else if ( material == "RWY_VASI_LIGHTS" ) {
        return gen_dir_light_group( nodes, normals, pnt_i,
                                    nml_i, mat, nup, false, true );
    } else if ( material == "RWY_BLUE_TAXIWAY_LIGHTS" ) {
        return gen_dir_light_group( nodes, normals, pnt_i, nml_i, mat, nup,
                                    true, false );
    } else {
        return gen_dir_light_group( nodes, normals, pnt_i, nml_i, mat, nup,
                                    false, false );
    }

    return NULL;
}
