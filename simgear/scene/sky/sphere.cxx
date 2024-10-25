/*
 * SPDX-FileName: sphere.cxx
 * SPDX-FileComment: build a sphere object. Pulled straight out of MesaGLU/quadratic.c
 * SPDX-FileContributor: Pulled straight out of MesaGLU/quadratic.c
 * SPDX-FileContributor: Original gluSphere code Copyright (C) 1999-2000  Brian Paul licensed under the GPL
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <osg/Array>
#include <osg/Geometry>

#include <simgear/debug/logstream.hxx>
#include <simgear/scene/material/EffectGeode.hxx>

using namespace simgear;

EffectGeode*
SGMakeSphere(double radius, int slices, int stacks)
{
    float rho, drho, dtheta;
    float s, t, ds, dt;
    int i, j, imin, imax;
    float nsign = 1.0;

    EffectGeode* geode = new EffectGeode;

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

        assert(vl->size() == nl->size());
        assert(vl->size() == tl->size());

        geometry->setUseVertexBufferObjects(true);
        geometry->setVertexArray(vl);
        geometry->setNormalArray(nl);
        geometry->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
        geometry->setTexCoordArray(0, tl);
        geometry->addPrimitiveSet(new osg::DrawArrays(GL_TRIANGLE_STRIP, 0, vl->size()));

        geode->addDrawable(geometry);

        t -= dt;
    }

    return geode;
}
