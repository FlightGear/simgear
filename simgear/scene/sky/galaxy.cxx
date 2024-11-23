/*
 * SPDX-FileName: galaxy.cxx
 * SPDX-FileComment: model the celestial sphere brightness by unresolved sources
 * SPDX-FileContributor: Chris Ringeval. Started November 2021.
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "galaxy.hxx"

#include <osg/MatrixTransform>

#include <simgear/scene/material/Effect.hxx>
#include <simgear/scene/material/EffectGeode.hxx>
#include <simgear/scene/util/SGReaderWriterOptions.hxx>

#include "sphere.hxx"

using namespace simgear;

osg::Node* SGGalaxy::build(double galaxy_size, const SGReaderWriterOptions* options)
{
    osg::ref_ptr<EffectGeode> orb = SGMakeSphere(galaxy_size, 32, 16);
    orb->setName("Galaxy");

    Effect* effect = makeEffect("Effects/galaxy", true, options);
    if (effect) {
        orb->setEffect(effect);
    }

    osg::ref_ptr<osg::MatrixTransform> galaxy_transform = new osg::MatrixTransform;
    galaxy_transform->addChild(orb);

    // reposition the Galaxy's texture, which is in galactic
    // coordinates, into the "fake" geocentric frame (which is carried
    // along our current position (p))
    //
    // Coordinates of the galactic north pole used with Gaia data (from
    // which our Milky Way Texture is built).
    //
    // https://www.cosmos.esa.int/web/gaia-users/archive/gedr3-documentation-pdf
    // Section 4.1.7.1 page 198
  
    const double galactic_north_pole_RA = 192.85948;
    const double galactic_north_pole_DEC = 27.12825;
    const double equatorial_north_pole_THETA = 122.93192;
   
    osg::Matrix RA, DEC, THETA;

    // RA origin at 90 degrees
    RA.makeRotate((galactic_north_pole_RA-90.0)*SGD_DEGREES_TO_RADIANS, osg::Vec3(0, 0, 1));
    // Rotate along rotated x-axis by -(90-DEC)
    DEC.makeRotate((galactic_north_pole_DEC-90.0)*SGD_DEGREES_TO_RADIANS, osg::Vec3(1, 0, 0));
    // Set the origin of the galactic longitude in Sagittarius, rotate
    // along rotated z-axis by -theta
    THETA.makeRotate(-equatorial_north_pole_THETA*SGD_DEGREES_TO_RADIANS, osg::Vec3(0, 0, 1));

    galaxy_transform->setMatrix(THETA*DEC*RA);
  
    return galaxy_transform.release();
}
