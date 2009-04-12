// dome.cxx -- model sky with an upside down "bowl"
//
// Written by Curtis Olson, started December 1997.
// SSG-ified by Curtis Olson, February 2000.
//
// Copyright (C) 1997-2000  Curtis L. Olson  - http://www.flightgear.org/~curt
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

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include <math.h>

#include <simgear/compiler.h>

#include <osg/Array>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Node>
#include <osg/Math>
#include <osg/MatrixTransform>
#include <osg/Material>
#include <osg/ShadeModel>
#include <osg/PrimitiveSet>

#include <simgear/debug/logstream.hxx>
#include <simgear/math/Math.hxx>
#include <simgear/scene/util/VectorArrayAdapter.hxx>

#include "dome.hxx"

using namespace osg;
using namespace simgear;

// proportions of max dimensions fed to the build() routine
static const float center_elev = 1.0;

namespace
{
struct DomeParam
{
    float radius;
    float elev;
} domeParams[] = {{.5, .8660},  // 60deg from horizon
                  {.8660, .5},  // 30deg from horizon
                  // Original dome horizon vertices
                  {0.9701, 0.2425}, {0.9960, 0.0885},
                  {1.0, 0.0}, {0.9922, -0.1240}};

const int numRings = sizeof(domeParams) / sizeof(domeParams[0]);
const int numBands = 12;
const int halfBands = numBands/2;
}

static const float upper_radius = 0.9701; // (.6, 0.15)
static const float upper_elev = 0.2425;

static const float middle_radius = 0.9960; // (.9, .08)
static const float middle_elev = 0.0885;

static const float lower_radius = 1.0;
static const float lower_elev = 0.0;

static const float bottom_radius = 0.9922; // (.8, -.1)
static const float bottom_elev = -0.1240;


// Constructor
SGSkyDome::SGSkyDome( void ) {
    asl = 0;
}


// Destructor
SGSkyDome::~SGSkyDome( void ) {
}

// Generate indices for a dome mesh. Assume a center vertex at 0, then
// rings of vertices. Each ring's vertices are stored together. An
// even number of longitudinal bands are assumed.

namespace
{
// Calculate the index of a vertex in the grid by using its address in
// the array that holds its location.
struct GridIndex
{
    VectorArrayAdapter<Vec3Array> gridAdapter;
    Vec3Array& grid;
    GridIndex(Vec3Array& array, int rowStride, int baseOffset) :
        gridAdapter(array, rowStride, baseOffset), grid(array)
    {
    }
    unsigned short operator() (int ring, int band)
    {
        return (unsigned short)(&gridAdapter(ring, band) - &grid[0]);
    }
};
}
void SGSkyDome::makeDome(int rings, int bands, DrawElementsUShort& elements)
{
    std::back_insert_iterator<DrawElementsUShort> pusher
        = std::back_inserter(elements);
    GridIndex grid(*dome_vl, numBands, 1);
    for (int i = 0; i < bands; i += 2) {
        *pusher = 0;  *pusher = grid(0, i);  *pusher = grid(0, i + 1);  
        // down a band
        for (int j = 0; j < rings - 1; ++j) {
            *pusher = grid(j, i);  *pusher = grid(j, i + 1);
            *pusher = grid(j + 1, i + 1);
            *pusher = grid(j, i);  *pusher =  grid(j + 1, i + 1);
            *pusher =  grid(j + 1, i);
        }
        // and up the next one
        for (int j = rings - 1; j > 0; --j) {
            *pusher = grid(j, i + 1);  *pusher = grid(j - 1, i + 1);
            *pusher = grid(j, (i + 2) % bands);
            *pusher = grid(j, (i + 2) % bands); *pusher = grid(j - 1, i + 1);
            *pusher = grid(j - 1, (i + 2) % bands);
        }
        *pusher = grid(0, i + 1);  *pusher = 0;
        *pusher = grid(0, (i + 2) % bands);
    }
}

// initialize the sky object and connect it into our scene graph
osg::Node*
SGSkyDome::build( double hscale, double vscale ) {

    osg::Geode* geode = new osg::Geode;

    // set up the state
    osg::StateSet* stateSet = geode->getOrCreateStateSet();
    stateSet->setRenderBinDetails(-10, "RenderBin");

    osg::ShadeModel* shadeModel = new osg::ShadeModel;
    shadeModel->setMode(osg::ShadeModel::SMOOTH);
    stateSet->setAttributeAndModes(shadeModel);
    stateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    stateSet->setMode(GL_FOG, osg::StateAttribute::OFF);
    stateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
    stateSet->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);
    stateSet->setMode(GL_BLEND, osg::StateAttribute::OFF);
    stateSet->setMode(GL_ALPHA_TEST, osg::StateAttribute::OFF);
    osg::Material* material = new osg::Material;
    stateSet->setAttribute(material);

    dome_vl = new osg::Vec3Array(1 + numRings * numBands);
    dome_cl = new osg::Vec3Array(1 + numRings * numBands);
    // generate the raw vertex data

    (*dome_vl)[0].set(0.0, 0.0, center_elev * vscale);
    simgear::VectorArrayAdapter<Vec3Array> vertices(*dome_vl, numBands, 1);

    for ( int i = 0; i < numBands; ++i ) {
        double theta = (i * 30) * SGD_DEGREES_TO_RADIANS;
        double sTheta = hscale*sin(theta);
        double cTheta = hscale*cos(theta);
        for (int j = 0; j < numRings; ++j) {
            vertices(j, i).set(cTheta * domeParams[j].radius,
                               sTheta * domeParams[j].radius,
                               domeParams[j].elev * vscale);
        }
    }

    DrawElementsUShort* domeElements
        = new osg::DrawElementsUShort(GL_TRIANGLES);
    makeDome(numRings, numBands, *domeElements);
    osg::Geometry* geom = new Geometry;
    geom->setName("Dome Elements");
    geom->setUseDisplayList(false);
    geom->setVertexArray(dome_vl.get());
    geom->setColorArray(dome_cl.get());
    geom->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    geom->setNormalBinding(osg::Geometry::BIND_OFF);
    geom->addPrimitiveSet(domeElements);
    geode->addDrawable(geom);
    // force a repaint of the sky colors with ugly defaults
    repaint(SGVec3f(1, 1, 1), SGVec3f(1, 1, 1), SGVec3f(1, 1, 1), 0.0, 5000.0 );
    dome_transform = new osg::MatrixTransform;
    dome_transform->addChild(geode);

    return dome_transform.get();
}

static void fade_to_black(osg::Vec3 sky_color[], float asl, int count) {
    const float ref_asl = 10000.0f;
    const float d = exp( - asl / ref_asl );
    for(int i = 0; i < count ; i++)
        sky_color[i] *= d;
}

inline void clampColor(osg::Vec3& color)
{
    color.x() = osg::clampTo(color.x(), 0.0f, 1.0f);
    color.y() = osg::clampTo(color.y(), 0.0f, 1.0f);
    color.z() = osg::clampTo(color.z(), 0.0f, 1.0f);
}

// repaint the sky colors based on current value of sun_angle, sky,
// and fog colors.  This updates the color arrays for ssgVtxTable.
// sun angle in degrees relative to verticle
// 0 degrees = high noon
// 90 degrees = sun rise/set
// 180 degrees = darkest midnight
bool
SGSkyDome::repaint( const SGVec3f& sun_color, const SGVec3f& sky_color,
                    const SGVec3f& fog_color, double sun_angle, double vis )
{
    SGVec3f outer_param, outer_diff;
    SGVec3f middle_param, middle_diff;

    // Check for sunrise/sunset condition
    if (sun_angle > 80) {
	// 0.0 - 0.4
        double sunAngleFactor = 10.0 - fabs(90.0 - sun_angle);
        static const SGVec3f outerConstant(1.0 / 20.0, 1.0 / 40.0, -1.0 / 30.0);
        static const SGVec3f middleConstant(1.0 / 40.0, 1.0 / 80.0, 0.0);
        outer_param = sunAngleFactor * outerConstant;
        middle_param = sunAngleFactor * middleConstant;
	outer_diff = (1.0 / 6.0) * outer_param;
	middle_diff = (1.0 / 6.0) * middle_param;
    } else {
        outer_param = SGVec3f(0, 0, 0);
	middle_param = SGVec3f(0, 0, 0);
	outer_diff = SGVec3f(0, 0, 0);
	middle_diff = SGVec3f(0, 0, 0);
    }
    // printf("  outer_red_param = %.2f  outer_red_diff = %.2f\n", 
    //        outer_red_param, outer_red_diff);

    // calculate transition colors between sky and fog
    SGVec3f outer_amt = outer_param;
    SGVec3f middle_amt = middle_param;

    //
    // First, recalulate the basic colors
    //

    // Magic factors for coloring the sky according visibility and
    // zenith angle.
    const double cvf = osg::clampBelow(vis, 45000.0);
    const double vis_factor = osg::clampTo((vis - 1000.0) / 2000.0, 0.0, 1.0);
    const float upperVisFactor = 1.0 - vis_factor * (0.7 + 0.3 * cvf/45000);
    const float middleVisFactor = 1.0 - vis_factor * (0.1 + 0.85 * cvf/45000);

    (*dome_cl)[0] = sky_color.osg();
    simgear::VectorArrayAdapter<Vec3Array> colors(*dome_cl, numBands, 1);
    const double saif = sun_angle/SG_PI;
    static const SGVec3f blueShift(0.8, 1.0, 1.2);
    const SGVec3f skyFogDelta = sky_color - fog_color;
    const SGVec3f sunSkyDelta = sun_color - sky_color;
    // For now the colors of the upper two rings are linearly
    // interpolated between the zenith color and the first horizon
    // ring color.
    
    for (int i = 0; i < halfBands+1; i++) {
        SGVec3f diff = mult(skyFogDelta, blueShift);
        diff *= (0.8 + saif - ((halfBands-i)/10));
        colors(2, i) = (sky_color - upperVisFactor * diff).osg();
        colors(3, i) = (sky_color - middleVisFactor * diff + middle_amt).osg();
        colors(4, i) = (fog_color + outer_amt).osg();
        colors(0, i) = simgear::math::lerp(sky_color.osg(), colors(2, i), .3942);
        colors(1, i) = simgear::math::lerp(sky_color.osg(), colors(2, i), .7885);
        for (int j = 0; j < numRings - 1; ++j)
            clampColor(colors(j, i));
        outer_amt -= outer_diff;
        middle_amt -= middle_diff;
    }

    for (int i = halfBands+1; i < numBands; ++i)
        for (int j = 0; j < 5; ++j)
            colors(j, i) = colors(j, numBands - i);

    fade_to_black(&(*dome_cl)[0], asl * center_elev, 1);
    for (int i = 0; i < numRings - 1; ++i)
        fade_to_black(&colors(i, 0), (asl+0.05f) * domeParams[i].elev,
                      numBands);

    for ( int i = 0; i < numBands; i++ )
        colors(numRings - 1, i) = fog_color.osg();
    dome_cl->dirty();
    return true;
}


// reposition the sky at the specified origin and orientation
// lon specifies a rotation about the Z axis
// lat specifies a rotation about the new Y axis
// spin specifies a rotation about the new Z axis (and orients the
// sunrise/set effects
bool
SGSkyDome::reposition( const SGVec3f& p, double _asl,
                       double lon, double lat, double spin ) {
    asl = _asl;

    osg::Matrix T, LON, LAT, SPIN;

    // Translate to view position
    // Point3D zero_elev = current_view.get_cur_zero_elev();
    // xglTranslatef( zero_elev.x(), zero_elev.y(), zero_elev.z() );
    T.makeTranslate( p.osg() );

    // printf("  Translated to %.2f %.2f %.2f\n", 
    //        zero_elev.x, zero_elev.y, zero_elev.z );

    // Rotate to proper orientation
    // printf("  lon = %.2f  lat = %.2f\n",
    //        lon * SGD_RADIANS_TO_DEGREES,
    //        lat * SGD_RADIANS_TO_DEGREES);
    // xglRotatef( lon * SGD_RADIANS_TO_DEGREES, 0.0, 0.0, 1.0 );
    LON.makeRotate(lon, osg::Vec3(0, 0, 1));

    // xglRotatef( 90.0 - f->get_Latitude() * SGD_RADIANS_TO_DEGREES,
    //             0.0, 1.0, 0.0 );
    LAT.makeRotate(90.0 * SGD_DEGREES_TO_RADIANS - lat, osg::Vec3(0, 1, 0));

    // xglRotatef( l->sun_rotation * SGD_RADIANS_TO_DEGREES, 0.0, 0.0, 1.0 );
    SPIN.makeRotate(spin, osg::Vec3(0, 0, 1));

    dome_transform->setMatrix( SPIN*LAT*LON*T );
    return true;
}
