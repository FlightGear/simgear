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
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the
// Free Software Foundation, Inc., 59 Temple Place - Suite 330,
// Boston, MA  02111-1307, USA.
//
// $Id$


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/compiler.h>

#include STL_IOSTREAM

#include <plib/sg.h>
#include <plib/ssg.h>


// return a sphere object as an ssgBranch
ssgBranch *ssgMakeSphere( ssgSimpleState *state, ssgColourArray *cl,
			  double radius, int slices, int stacks,
			  ssgCallback predraw, ssgCallback postdraw )
{
    float rho, drho, theta, dtheta;
    float x, y, z;
    float s, t, ds, dt;
    int i, j, imin, imax;
    float nsign = 1.0;
    ssgBranch *sphere = new ssgBranch;
    sgVec2 vec2;
    sgVec3 vec3;

    drho = SG_PI / (float) stacks;
    dtheta = 2.0 * SG_PI / (float) slices;

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
	ssgVertexArray   *vl = new ssgVertexArray();
	ssgNormalArray   *nl = new ssgNormalArray();
	ssgTexCoordArray *tl = new ssgTexCoordArray();

	rho = i * drho;
	s = 0.0;
	for ( j = 0; j <= slices; j++ ) {
	    theta = (j == slices) ? 0.0 : j * dtheta;
	    x = -sin(theta) * sin(rho);
	    y = cos(theta) * sin(rho);
	    z = nsign * cos(rho);

	    // glNormal3f( x*nsign, y*nsign, z*nsign );
	    sgSetVec3( vec3, x*nsign, y*nsign, z*nsign );
	    sgNormalizeVec3( vec3 );
	    nl->add( vec3 );

	    // glTexCoord2f(s,t);
	    sgSetVec2( vec2, s, t );
	    tl->add( vec2 );

	    // glVertex3f( x*radius, y*radius, z*radius );
	    sgSetVec3( vec3, x*radius, y*radius, z*radius );
	    vl->add( vec3 );

	    x = -sin(theta) * sin(rho+drho);
	    y = cos(theta) * sin(rho+drho);
	    z = nsign * cos(rho+drho);

	    // glNormal3f( x*nsign, y*nsign, z*nsign );
	    sgSetVec3( vec3, x*nsign, y*nsign, z*nsign );
	    sgNormalizeVec3( vec3 );
	    nl->add( vec3 );

	    // glTexCoord2f(s,t-dt);
	    sgSetVec2( vec2, s, t-dt );
	    tl->add( vec2 );
	    s += ds;

	    // glVertex3f( x*radius, y*radius, z*radius );
	    sgSetVec3( vec3, x*radius, y*radius, z*radius );
	    vl->add( vec3 );
	}

	ssgLeaf *slice = 
	    new ssgVtxTable ( GL_TRIANGLE_STRIP, vl, nl, tl, cl );

	if ( vl->getNum() != nl->getNum() ) {
	    cout << "bad sphere1\n";
	    exit(-1);
	}
	if ( vl->getNum() != tl->getNum() ) {
	    cout << "bad sphere2\n";
	    exit(-1);
	}
	slice->setState( state );
	slice->setCallback( SSG_CALLBACK_PREDRAW, predraw );
	slice->setCallback( SSG_CALLBACK_POSTDRAW, postdraw );

	sphere->addKid( slice );

	t -= dt;
    }

    return sphere;
}
