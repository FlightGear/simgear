/*
 * SPDX-FileName: dome.cxx
 * SPDX-FileComment: model sky with an upside down "bowl"
 * SPDX-FileCopyrightText: Copyright (C) 1997-2000  Curtis L. Olson  - http://www.flightgear.org/~curt
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <iterator>

#include <simgear/debug/logstream.hxx>
#include <simgear/scene/util/OsgMath.hxx>
#include <simgear/scene/util/SGReaderWriterOptions.hxx>
#include <simgear/scene/util/VectorArrayAdapter.hxx>
#include <simgear/scene/material/Effect.hxx>
#include <simgear/scene/material/EffectGeode.hxx>

#include "dome.hxx"

using namespace osg;
using namespace simgear;

namespace {

// proportions of max dimensions fed to the build() routine
const float center_elev = 1.0f;

const int numRings = 64; //sizeof(domeParams) / sizeof(domeParams[0]);
const int numBands = 64; // 12

// Make dome a bit over half sphere
const float domeAngle = 120.0f;

const float bandDelta = 360.0f / numBands;
const float ringDelta = domeAngle / (numRings + 1);


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

} // anonymous namespace


osg::Node* SGSkyDome::build(double hscale, double vscale, SGReaderWriterOptions* options)
{
    EffectGeode* geode = new EffectGeode;
    geode->setName("Skydome");
    geode->setCullingActive(false); // Prevent skydome from being culled away

    Effect* effect = makeEffect("Effects/skydome", true, options);
    if(effect) {
        geode->setEffect(effect);
    }

    dome_vl = new osg::Vec3Array(2 + numRings * numBands);

    // generate the raw vertex data
    (*dome_vl)[0].set(0.0, 0.0,  center_elev * vscale);
    (*dome_vl)[1].set(0.0, 0.0, -center_elev * vscale);
    simgear::VectorArrayAdapter<Vec3Array> vertices(*dome_vl, numBands, 2);

    for (int i = 0; i < numBands; ++i) {
        double theta = (i * bandDelta) * SGD_DEGREES_TO_RADIANS;
        double sTheta = hscale*sin(theta);
        double cTheta = hscale*cos(theta);
        for (int j = 0; j < numRings; ++j) {
            vertices(j, i).set(cTheta * sin((j+1)*ringDelta*SGD_DEGREES_TO_RADIANS), //domeParams[j].radius,
                               sTheta * sin((j+1)*ringDelta*SGD_DEGREES_TO_RADIANS),// domeParams[j].radius,
                               vscale * cos((j+1)*ringDelta*SGD_DEGREES_TO_RADIANS)); //domeParams[j].elev * vscale);
        }
    }

    DrawElementsUShort* domeElements = new osg::DrawElementsUShort(GL_TRIANGLES);
    makeDome(numRings, numBands, *domeElements);

    osg::Geometry* geom = new Geometry;
    geom->setName("Dome Elements");
    geom->setUseVertexBufferObjects(true);
    geom->setVertexArray(dome_vl.get());
    geom->setNormalBinding(osg::Geometry::BIND_OFF);
    geom->addPrimitiveSet(domeElements);

    geode->addDrawable(geom);

    dome_transform = new osg::MatrixTransform;
    dome_transform->setName("Skydome transform");
    dome_transform->addChild(geode);

    return dome_transform.get();
}

bool SGSkyDome::reposition(const SGVec3f& p, double _asl,
                           double lon, double lat, double spin)
{
    asl = _asl;

    osg::Matrix T, LON, LAT, SPIN;
    // Translate to view position
    T.makeTranslate(toOsg(p));
    // Rotate to proper orientation
    LON.makeRotate(lon, osg::Vec3(0, 0, 1));
    LAT.makeRotate(90.0 * SGD_DEGREES_TO_RADIANS - lat, osg::Vec3(0, 1, 0));
    SPIN.makeRotate(spin, osg::Vec3(0, 0, 1));

    dome_transform->setMatrix(SPIN * LAT * LON * T);
    return true;
}

/*
 * Generate indices for a dome mesh. Assume a center vertex at 0, then
 * rings of vertices. Each ring's vertices are stored together. An
 * even number of longitudinal bands are assumed.
 */
void SGSkyDome::makeDome(int rings, int bands, DrawElementsUShort& elements)
{
    std::back_insert_iterator<DrawElementsUShort> pusher
        = std::back_inserter(elements);
    GridIndex grid(*dome_vl, numBands, 2);
    for (int i = 0; i < bands; i++) {
        *pusher = 0;  *pusher = grid(0, (i+1)%bands);  *pusher = grid(0, i);
        // down a band
        for (int j = 0; j < rings - 1; ++j) {
            *pusher = grid(j, i);  *pusher = grid(j, (i + 1)%bands);
            *pusher = grid(j + 1, (i + 1)%bands);
            *pusher = grid(j, i);  *pusher =  grid(j + 1, (i + 1)%bands);
            *pusher =  grid(j + 1, i);
        }
        // Cap
        *pusher = grid(rings - 1, i); *pusher = grid(rings - 1, (i + 1)%bands);
        *pusher = 1;
    }
}
