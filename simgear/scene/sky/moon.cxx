/*
 * SPDX-FileName: moon.cxx
 * SPDX-FileComment: model earth's moon
 * SPDX-FileContributor: Written by Durk Talsma. Originally started October 1997.
 * SPDX-FileContributor: Based upon algorithms and data kindly provided by Mr. Paul Schlyter (pausch@saaf.se).
 * SPDX-FileContributor: Separated out rendering pieces and converted to ssg by Curt Olson, March 2000.
 * SPDX-FileContributor: Ported to the OpenGL core profile by Fernando García Liñán, 2024.
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <simgear/scene/material/Effect.hxx>
#include <simgear/scene/material/EffectGeode.hxx>
#include <simgear/scene/util/SGReaderWriterOptions.hxx>

#include "sphere.hxx"
#include "moon.hxx"

using namespace simgear;

osg::Node* SGMoon::build(double moon_size, const SGReaderWriterOptions* options)
{
    osg::ref_ptr<EffectGeode> orb = SGMakeSphere(moon_size, 40, 20);
    orb->setName("Moon");

    Effect* effect = makeEffect("Effects/moon", true, options);
    if (effect) {
        orb->setEffect(effect);
    }

    moon_transform = new osg::MatrixTransform;
    moon_transform->addChild(orb);
    return moon_transform.get();
}

/*
 * Reposition the moon at the specified right ascension and declination from the
 * center of Earth. Because the view is actually offset by our current position
 * (p), we first evaluate our current position w.r.t the Moon and then shift to
 * the articial center of earth before shifting to the rendered moon distance.
 * This allows to implement any parallax effects.
 *
 * moon_dist_bare is expected to not change during the rendering, it gives us
 * the normalisation factors between real distances and units used in the
 * rendering.
 *
 * moon_dist_fact is any extra factors to put the moon further or closer.
 */
bool SGMoon::reposition(double rightAscension, double declination,
                        double moon_dist_bare, double moon_dist_factor,
                        double lst, double lat, double alt)
{
    osg::Matrix TE, T2, RA, DEC;

    // semi mayor axis
    const double moon_a_in_rearth = 60.266600;
    // average (we could also account for equatorial streching like in
    // oursun.cxx if required)
    const double earth_radius_in_meters = 6371000.0;
    //local hour angle in radians
    double lha;
    // in unit of the rendering
    double moon_dist;
    double earth_radius, viewer_radius;
    double xp, yp, zp;

    //rendered earth radius according to what has been specified by
    //moon_dist_bare
    earth_radius = moon_dist_bare/moon_a_in_rearth;

    //how far are we from the center of Earth
    viewer_radius = (1.0 + alt/earth_radius_in_meters)*earth_radius;

    // the local hour angle of the moon, .i.e. its angle with respect
    // to the meridian of the viewer
    lha = lst - rightAscension;

    // the shift vector of the observer w.r.t. earth center (funny
    // convention on x?)
    zp = viewer_radius * sin(lat);
    yp = viewer_radius * cos(lat)*cos(lha);
    xp = viewer_radius * cos(lat)*sin(-lha);

    //rotate along the z axis
    RA.makeRotate(rightAscension - 90.0 * SGD_DEGREES_TO_RADIANS, osg::Vec3(0, 0, 1));
    //rotate along the rotated x axis
    DEC.makeRotate(declination, osg::Vec3(1, 0, 0));

    //move to the center of Earth
    TE.makeTranslate(osg::Vec3(-xp,-yp,-zp));

    //move the moon from the center of Earth to moon_dist
    moon_dist = moon_dist_bare * moon_dist_factor;
    T2.makeTranslate(osg::Vec3(0, moon_dist, 0));

    // cout << " viewer radius= " << viewer_radius << endl;
    // cout << " xp yp zp= " << xp <<" " << yp << " " << zp << endl;
    // cout << " lha= " << lha << endl;
    // cout << " moon_dist_bare= " << moon_dist_bare << endl;
    // cout << " moon_dist_factor= " << moon_dist_factor << endl;
    // cout << " moon_dist= " << moon_dist << endl;

    moon_transform->setMatrix(T2*TE*DEC*RA);

    return true;
}

