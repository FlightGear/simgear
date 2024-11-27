/*
 * SPDX-FileName: stars.cxx
 * SPDX-FileComment: model the stars (and planets)
 * SPDX-FileContributor: Written by Durk Talsma. Originally started October 1997.
 * SPDX-FileContributor: Based upon algorithms and data kindly provided by Mr. Paul Schlyter (pausch@saaf.se).
 * SPDX-FileContributor: Separated out rendering pieces and converted to ssg by Curt Olson, March 2000.
 * SPDX-FileContributor: Ported to the OpenGL core profile by Fernando García Liñán, 2024.
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "stars.hxx"

#include <unordered_map>

#include <osg/Geometry>

#include <simgear/scene/material/Effect.hxx>
#include <simgear/scene/material/EffectGeode.hxx>
#include <simgear/scene/util/SGReaderWriterOptions.hxx>

using namespace simgear;

namespace {

// Star surface temperature in K associated to a given spectral type
// https://sites.uni.edu/morgans/astro/course/Notes/section2/spectraltemps.html
const std::unordered_map<char, double> k_star_temperature_map = {
    {'W', 70000},
    {'O', 50000},
    {'B', 21000},
    {'p', 21000},
    {'A', 8650},
    {'F', 6650},
    {'G', 5650},
    {'K', 4600},
    {'M', 3000},
    {'C', 3000},
    {'S', 3000},
    {'N', 3000},
};

/*
 * Implements Planck's law.
 * Returns the spectral irradiance in W * m^-2 * m^-1 emitted by a black body in
 * thermal equilibrium at a temperature T in Kelvin and for a given wavelength
 * in meters.
 */
double plancks_law(double lambda, double T)
{
    constexpr double pi = 3.14159265358979323846; // Can't use SGD_PI, we need constexpr
    constexpr double c  = 299'792'458.0;   // Speed of light, m * s^-1
    constexpr double h  = 6.626070040e-34; // Planck's constant, J * s
    constexpr double kb = 1.380649e-23;    // Boltzmann constant, J * K^-1
    return (8.0 * pi * h * c*c)
        / (std::pow(lambda, 5) * (std::exp(h * c / (lambda * kb * T)) - 1.0));
}

/*
 * Implements Stefan-Boltzmann's law.
 * Returns the irradiance in W * m^-2 emitted by a black body at a temperature T.
 */
double stefan_boltzmann_law(double T)
{
    constexpr double sigma = 5.670374419e-8; // Stefan-Boltzmann constant, W * m^-2 * K^-4
    return sigma * std::pow(T, 4);
}

/*
 * For a black body at a temperature T emitting an irradiance in W * m^-2,
 * return four spectral irradiance samples corresponding to the wavelengths used
 * by HDR's atmospheric scattering approximation (630, 560, 490 and 430 nm).
 */
std::array<double, 4> spectral_radiance_vec4(double irradiance, double T)
{
    std::array<double, 4> result{};
    constexpr double wavelengths[4] = {630e-9, 560e-9, 490e-9, 430e-9}; // meters
    for (auto i = 0; i < 4; ++i) {
        // Normalize the spectral irradiance obtained with Planck's law by
        // dividing by the total irradiance obtained through Stefan-Boltzmann's
        // law.
        //
        // The normalized values are then multiplied by the given irradiance,
        // obtaining an spectral irradiance in W * m^-2 * nm^-1.
        //
        // This irradiance is then converted to radiance with an empirical
        // conversion factor.
        result[i] = irradiance * plancks_law(wavelengths[i], T)
            * 1e-9 / stefan_boltzmann_law(T);
    }
    return result;
}

/*
 * Return the irradiance at the Earth in W * m^-2 for a given a stellar visual
 * magnitude. This calculation already discounts atmospheric absorption (0.4).
 */
double irradiance_from_magnitude(double magnitude)
{
    return std::pow(10, 0.4 * (-magnitude - 19.0 + 0.4));
}

} // anonymous namespace

osg::Node* SGStars::build(int num, const SGStarData::Star* star_data, double star_dist,
                          const SGReaderWriterOptions* options)
{
    osg::ref_ptr<EffectGeode> geode = new EffectGeode;
    geode->setName("Stars");

    Effect* effect = makeEffect("Effects/stars", true, options);
    if (effect) {
        geode->setEffect(effect);
    }

    osg::ref_ptr<osg::Vec3Array> vl = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec4Array> il = new osg::Vec4Array;
    vl->reserve(num);
    il->reserve(num);

    for (int i = 0; i < num; ++i) {
        // Position the star arbitrarily far away
        osg::Vec3 pos(star_dist * std::cos(star_data[i].ra) * std::cos(star_data[i].dec),
                      star_dist * std::sin(star_data[i].ra) * std::cos(star_data[i].dec),
                      star_dist * std::sin(star_data[i].dec));
        vl->push_back(pos);

        // Get the star's surface temperature based on the spectral type
        double temperature = 5800.0;
        auto itr = k_star_temperature_map.find(star_data[i].spec[0]);
        if (itr != k_star_temperature_map.end()) {
            temperature = itr->second;
        } else {
            SG_LOG(SG_ASTRO, SG_WARN, "Found star with unknown spectral type "
                  << star_data[i].spec);
        }

        double irradiance = irradiance_from_magnitude(star_data[i].mag);
        std::array<double, 4> si = spectral_radiance_vec4(irradiance, temperature);
        il->push_back(osg::Vec4(si[0], si[1], si[2], si[3]));
    }

    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry;
    geometry->setUseVertexBufferObjects(true);
    geometry->setVertexArray(vl);
    geometry->setVertexAttribArray(1, il, osg::Array::BIND_PER_VERTEX);
    geometry->addPrimitiveSet(new osg::DrawArrays(GL_POINTS, 0, num));

    geode->addDrawable(geometry);

    return geode.release();
}
