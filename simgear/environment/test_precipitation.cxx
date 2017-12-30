/**************************************************************************
 * test_precipitation.cxx -- unit-tests for SGPrecipitation class
 *
 * Copyright (C) 2017  Scott Giese (xDraconian) - <scttgs0@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * $Id$
 **************************************************************************/

#include <cmath>
#include <memory>

#include <osg/Group>

#include <simgear/constants.h>
#include <simgear/misc/test_macros.hxx>

#include "precipitation.hxx"

using std::cout;
using std::cerr;
using std::endl;


class SGPrecipitationTestFixture : public SGPrecipitation
{
public:
    osg::ref_ptr<osg::Group> _group;

    void test_configuration()
    {
        _group = build();

        SG_VERIFY(getEnabled());
        SG_VERIFY(_precipitationEffect);
        SG_CHECK_EQUAL_EP(_rain_intensity, 0.0f);
        SG_CHECK_EQUAL_EP(_snow_intensity, 0.0f);
        SG_VERIFY(!_freeze);
    }

    void test_rain()
    {
        int count = 500;

        do {
            if (--count == 0) {
                cerr << "error: method setRainIntensity took too long.";
                exit(EXIT_FAILURE);
            }

            float saveRainIntensity = _rain_intensity;

            setRainIntensity(0.4f);
            update();

            SG_CHECK_GE(_rain_intensity, saveRainIntensity);
        } while (std::fabs(_rain_intensity - 0.4f) > 0.00001f);

        SG_CHECK_EQUAL_EP2(_rain_intensity, 0.4f, 0.00001f);
    }

    void test_rain_external()
    {
        int count = 500;

        do {
            if (--count == 0) {
                cerr << "error: method setRainIntensity-external took too long.";
                exit(EXIT_FAILURE);
            }

            float saveRainIntensity = _rain_intensity;

            setRainIntensity(0.25f);
            update();

            SG_CHECK_LE(_rain_intensity, saveRainIntensity);
        } while (std::fabs(_rain_intensity - 0.25f) > 0.00001f);

        // call once more to ensure we have intensity precisely set
        setRainIntensity(0.25f);
        update();

        SG_CHECK_EQUAL_EP2(_rain_intensity, 0.25f, 0.00001f);
    }

    void test_freeze()
    {
        float saveParticleSize = _precipitationEffect->getParticleSize();
        float saveParticleSpeed = std::fabs(_precipitationEffect->getParticleSpeed());

        // change rain to snow
        // expect particle size to increase
        // expect particle speed to decrease
        setFreezing(true);
        update();

        SG_CHECK_GT(_precipitationEffect->getParticleSize(), saveParticleSize);
        SG_CHECK_LT(std::fabs(_precipitationEffect->getParticleSpeed()), saveParticleSpeed);
    }

    void test_snow()
    {
        int count = 500;

        do {
            if (--count == 0) {
                cerr << "error: method setSnowIntensity took too long.";
                exit(EXIT_FAILURE);
            }

            float saveSnowIntensity = _snow_intensity;

            // not a typo - when freezing is enabled, snow intensity is keep in sync with rain intensity
            setRainIntensity(0.3f);
            update();

            SG_CHECK_LE(_snow_intensity, saveSnowIntensity);
        } while (std::fabs(_snow_intensity - 0.3f) > 0.00001f);

        SG_CHECK_EQUAL_EP2(_snow_intensity, 0.3f, 0.00001f);
    }

    void test_snow_external()
    {
        setRainDropletSize(0.025f);
        setSnowFlakeSize(0.04f);
        setDropletExternal(true);

        int count = 600;

        do {
            if (--count == 0) {
                cerr << "error: method setSnowIntensity-external took too long.";
                exit(EXIT_FAILURE);
            }

            float saveSnowIntensity = _snow_intensity;

            setSnowIntensity(0.55f);
            update();

            SG_CHECK_GE(_snow_intensity, saveSnowIntensity);
        } while (std::fabs(_snow_intensity - 0.55f) > 0.00001f);

        // call once more to ensure we have intensity precisely set
        setSnowIntensity(0.55f);
        update();

        SG_CHECK_EQUAL_EP2(_snow_intensity, 0.55f, 0.00001f);
    }

    void test_unfreeze()
    {
        float saveParticleSize = _precipitationEffect->getParticleSize();
        float saveParticleSpeed = std::fabs(_precipitationEffect->getParticleSpeed());

        // change snow to rain
        // expect particle size to decrease
        // expect particle speed to increase
        setFreezing(false);
        update();

        SG_CHECK_LT(_precipitationEffect->getParticleSize(), saveParticleSize);
        SG_CHECK_GT(std::fabs(_precipitationEffect->getParticleSpeed()), saveParticleSpeed);
    }

    void test_no_precipitation()
    {
        setEnabled(false);
        SG_VERIFY(!getEnabled());

        update();

        // intensity drops to zero, so we should see this reflected in the particles
        SG_CHECK_EQUAL_EP(_precipitationEffect->getParticleSize(), 0.01f);
        SG_CHECK_EQUAL_EP(_precipitationEffect->getParticleSpeed(), -2.0f);

        setEnabled(true);
    }

    void test_illumination()
    {
        setIllumination(0.87f);
        SG_CHECK_EQUAL_EP(_illumination, 0.87f);
    }

    void test_clipping()
    {
        setClipDistance(6.5);
        SG_CHECK_EQUAL_EP(_clip_distance, 6.5f);
    }

    void test_wind()
    {
        setWindProperty(87.0f, 0.7f);
        auto vec = _wind_vec;
        SG_CHECK_EQUAL_EP2(vec[0], 0.011166f, 0.00001f);
        SG_CHECK_EQUAL_EP2(vec[1], -0.213068f, 0.00001f);
        SG_CHECK_EQUAL_EP2(vec[2], 0.0f, 0.00001f);
    }
};


int main(int argc, char* argv[])
{
    auto fixture = std::unique_ptr<SGPrecipitationTestFixture>(new SGPrecipitationTestFixture);

    fixture->test_configuration();
    fixture->test_rain();
    fixture->test_freeze();
    fixture->test_snow();
    fixture->test_unfreeze();
    fixture->test_no_precipitation();

    fixture->test_snow_external();
    fixture->test_rain_external();
    fixture->test_illumination();
    fixture->test_clipping();
    fixture->test_wind();
    fixture->test_no_precipitation();

    cout << "all tests passed OK" << endl;

    return EXIT_SUCCESS;
}
