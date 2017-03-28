// sphere.cxx -- build an ssg sphere object
//
// Pulled straight out of MesaGLU/quadratic.c
//
// Original gluSphere code is Copyright (C) 1999-2000  Brian Paul and
// licensed under the GPL
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// $Id$

#include <simgear_config.h>
#include <simgear/compiler.h>
#include <simgear/constants.h>
#include <simgear/debug/logstream.hxx>

#include <iostream>
#include <cstdlib>

#include <osg/Node>
#include <osg/Geometry>
#include <osg/Geode>
#include <osg/Array>


// return a sphere object as an ssgBranch
osg::Node*
SGMakeSphere(double radius, int slices, int stacks)
{
    float rho, drho, dtheta;
    float s, t, ds, dt;
    int i, j, imin, imax;
    float nsign = 1.0;
    osg::Geode* geode = new osg::Geode;

    drho = SGD_PI / (float) stacks;
    dtheta = SGD_2PI / (float) slices;

    /* texturing: s goes from 0.0/0.25/0.5/0.75/1.0 at +y/+x/-y/-x/+y
       axis t goes from -1.0/+1.0 at z = -radius/+radius (linear along
       longitudes) cannot use triangle fan on texturing (s coord. at
       top/bottom tip varies) */

    ds = 1.0 / slices;
    dt = 1.0 / stacks;
    t = 1.0;  /* because loop now runs from 0 */
    imin = 0;
    imax = stacks;

    /* build slices as quad strips */
    for ( i = imin; i < imax; i++ ) {
        osg::Geometry* geometry = new osg::Geometry;
        osg::Vec3Array* vl = new osg::Vec3Array;
        osg::Vec3Array* nl = new osg::Vec3Array;
        osg::Vec2Array* tl = new osg::Vec2Array;

	rho = i * drho;
	s = 0.0;
	for ( j = 0; j <= slices; j++ ) {
	    double theta = (j == slices) ? 0.0 : j * dtheta;
	    double x = -sin(theta) * sin(rho);
	    double y = cos(theta) * sin(rho);
	    double z = nsign * cos(rho);

	    // glNormal3f( x*nsign, y*nsign, z*nsign );
            osg::Vec3 normal(x*nsign, y*nsign, z*nsign);
	    normal.normalize();
	    nl->push_back(normal);

	    // glTexCoord2f(s,t);
	    tl->push_back(osg::Vec2(s, t));

	    // glVertex3f( x*radius, y*radius, z*radius );
	    vl->push_back(osg::Vec3(x*radius, y*radius, z*radius));

	    x = -sin(theta) * sin(rho+drho);
	    y = cos(theta) * sin(rho+drho);
	    z = nsign * cos(rho+drho);

	    // glNormal3f( x*nsign, y*nsign, z*nsign );
            normal = osg::Vec3(x*nsign, y*nsign, z*nsign);
	    normal.normalize();
	    nl->push_back(normal);

	    // glTexCoord2f(s,t-dt);
	    tl->push_back(osg::Vec2(s, t-dt));
	    s += ds;

	    // glVertex3f( x*radius, y*radius, z*radius );
	    vl->push_back(osg::Vec3(x*radius, y*radius, z*radius));
	}

	if ( vl->size() != nl->size() ) {
            SG_LOG( SG_EVENT, SG_ALERT, "bad sphere1");
	    exit(-1);
	}
	if ( vl->size() != tl->size() ) {
            SG_LOG( SG_EVENT, SG_ALERT, "bad sphere2");
	    exit(-1);
	}

        // colors
        osg::Vec4Array* cl = new osg::Vec4Array;
        cl->push_back(osg::Vec4(1, 1, 1, 1));

        geometry->setUseDisplayList(false);
        geometry->setVertexArray(vl);
        geometry->setNormalArray(nl);
        geometry->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
        geometry->setTexCoordArray(0, tl);
        geometry->setColorArray(cl);
        geometry->setColorBinding(osg::Geometry::BIND_OVERALL);
        geometry->addPrimitiveSet(new osg::DrawArrays(GL_TRIANGLE_STRIP, 0, vl->size()));
        geode->addDrawable(geometry);

	t -= dt;
    }

    return geode;
}
