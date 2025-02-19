/* -*-c++-*-
 * VPBMaterialHandler.cxx -- WS30 material-based generation handlers
 *
 * Copyright (C) 2021 Fahim Dalvi
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */

#include <osg/MatrixTransform>
#include <osgTerrain/TerrainTile>
#include <osgUtil/IntersectionVisitor>
#include <osgUtil/LineSegmentIntersector>

#include <simgear/math/sg_random.hxx>
#include <simgear/scene/material/Effect.hxx>
#include <simgear/scene/material/EffectGeode.hxx>
#include <simgear/scene/util/SGNodeMasks.hxx>
#include <simgear/scene/util/SGReaderWriterOptions.hxx>

#include "LightBin.hxx"
#include "TreeBin.hxx"
#include "VPBMaterialHandler.hxx"

using namespace osgTerrain;

namespace simgear {
// Common VPBMaterialHandler functions
bool VPBMaterialHandler::checkAgainstObjectMask(
    osg::Image *objectMaskImage, ImageChannel channel, double sampleProbability,
    double x, double y, float x_scale, float y_scale, const osg::Vec2d t_0,
    osg::Vec2d t_x, osg::Vec2d t_y) {

    osg::Vec2 t = osg::Vec2(t_0 + t_x * x + t_y * y);
    return checkAgainstObjectMask(objectMaskImage, channel, sampleProbability, x_scale, y_scale, t);
}

bool VPBMaterialHandler::checkAgainstObjectMask(
    osg::Image *objectMaskImage, ImageChannel channel, double sampleProbability,
    float x_scale, float y_scale, osg::Vec2d t) {
    if (objectMaskImage != NULL) {
        unsigned int x =
            (unsigned int)(objectMaskImage->s() * t.x() * x_scale) %
            objectMaskImage->s();
        unsigned int y =
            (unsigned int)(objectMaskImage->t() * t.y() * y_scale) %
            objectMaskImage->t();

        float color_value = objectMaskImage->getColor(x, y)[channel];
        return (sampleProbability > color_value);
    }
    return false;
}

double VPBMaterialHandler::det2(const osg::Vec2d a, const osg::Vec2d b) {
    return a.x() * b.y() - b.x() * a.y();
}

/** VegetationHandler implementation
 * The code has been ported as-is from the following source:
 * Simgear commit 6d71ab75:
 *  simgear/scene/tgdb/VPBTechnique.cxx : applyTrees()
 */
bool VegetationHandler::initialize(osg::ref_ptr<SGReaderWriterOptions> options,
                                   osg::ref_ptr<TerrainTile> terrainTile,
                                   osg::ref_ptr<SGMaterialCache> matcache) {
    bool use_random_vegetation = false;
    int vegetation_lod_level = 6;
    vegetation_density = 1.0;

    // Determine tree spacing, assuming base density of 1 tree per 100m^2,
    // though spacing is linear here, as is the
    // /sim/rendering/vegetation-density property.

    SGPropertyNode *propertyNode = options->getPropertyNode().get();

    if (propertyNode) {
        use_random_vegetation = propertyNode->getBoolValue(
            "/sim/rendering/random-vegetation", use_random_vegetation);
        vegetation_density = propertyNode->getFloatValue(
            "/sim/rendering/vegetation-density", vegetation_density);
        vegetation_lod_level = propertyNode->getIntValue(
            "/sim/rendering/static-lod/vegetation-lod-level",
            vegetation_lod_level);
    }

    // Do not generate vegetation for tiles too far away or if we explicitly
    // don't generate vegetation
    int level = terrainTile->getTileID().level;
    if ((!use_random_vegetation) ||
        (level < vegetation_lod_level)) {
        return false;
    }

    // Determine the minimum vegetation density.
    osgTerrain::Layer* colorLayer = terrainTile->getColorLayer(0);

    if (!colorLayer) {
        SG_LOG(SG_TERRAIN, SG_ALERT, "No landclass image for " << terrainTile->getTileID().x << " " << terrainTile->getTileID().y << " " << terrainTile->getTileID().level);
        return false;
    }

    osg::Image* image = colorLayer->getImage();
    if (!image || ! image->valid()) {
        SG_LOG(SG_TERRAIN, SG_ALERT, "No landclass image for " << terrainTile->getTileID().x << " " << terrainTile->getTileID().y << " " << terrainTile->getTileID().level);
        return false;
    }

    // Determine the maximum density of vegetation for this tile by
    // building a set of landclassess and looking them up in the material
    // cache.
    std::set <int>lcSet;
    for (int t = 0; t < image->t(); ++t) {
        for (int s = 0; s < image->s(); ++s) {
            const osg::Vec4 tc = image->getColor(s, t);
            int lc = int(std::round(tc.x() * 255.0));
            lcSet.insert(lc);
        }
    }

    const double MAX_MIN_MAT_COVERAGE = 100000.0;
    min_material_coverage = MAX_MIN_MAT_COVERAGE;
    for(auto lc = lcSet.begin(); lc != lcSet.end(); ++lc) {
        int l = *lc;
        auto mat = matcache->find(l);
        if (mat && (mat->get_wood_coverage() > 0.0) && (mat->get_wood_coverage() < min_material_coverage)) 
            min_material_coverage = mat->get_wood_coverage();
    }

    if (min_material_coverage == MAX_MIN_MAT_COVERAGE) 
        return false;

    bin = NULL;
    wood_density = 0.0;
    // This is the density of points we will generate across the patch before
    // applying the material vegetation density, object mask etc.  
    // Note that the units are m2.  I.e. the average area per piece of vegetation.
    // So a smaller number means more vegetation.
    //
    // vegentation_density ranges from 0.1 to 8, and is linear.  I.e. the are density factor varies from
    // 0.01 to 64.
    //
    // Maximum material.xml wood coverage is 4000m^2 - e.g. one tree for every 4000m^2 at medium density,
    // or one every 4000/64 = 62m^2 or every 4m linearly at maximum density.
    //  We also generate fewer trees at further out LoD levels.
    double level_factor = (double) ((7 - level) * (7 - level));
    min_coverage_m2 = (min_material_coverage / (vegetation_density * vegetation_density)) * level_factor;

    SG_LOG(SG_TERRAIN, SG_DEBUG, "Base point density for vegetation: " << min_material_coverage << " / " << vegetation_density << "^2 / " << level_factor << " = " << min_coverage_m2);
    return true;
}

void VegetationHandler::setLocation(const SGGeod loc, double r_E_lat,
                                    double r_E_lon) {
    delta_lat = sqrt(1000.0 / vegetation_density) / r_E_lat;
    delta_lon = sqrt(1000.0 / vegetation_density) /
                (r_E_lon * cos(loc.getLatitudeRad()));
}

bool VegetationHandler::handleNewMaterial(SGMaterial *mat) {
    if (mat->get_wood_coverage() <= 0)
        return false;

    // Wood coverage is relative to the value above.  E.g. we
    // will generate one tree for each point if the material
    // coverage value is equals to min_material_coverage.
    wood_density = min_material_coverage / mat->get_wood_coverage();

    bool found = false;

    for (SGTreeBinList::iterator iter = randomForest.begin();
         iter != randomForest.end(); iter++) {

        bin = *iter;

        if ((bin->texture           == mat->get_tree_texture()  ) &&
            (bin->teffect           == mat->get_tree_effect()   ) &&
            (bin->texture_varieties == mat->get_tree_varieties()) &&
            (bin->range             == mat->get_tree_range()    ) &&
            (bin->width             == mat->get_tree_width()    ) &&
            (bin->height            == mat->get_tree_height()   )   ) {
                found = true;
                break;
        }
    }

    if (!found) {
        bin = new TreeBin();
        bin->texture = mat->get_tree_texture();
        SG_LOG(SG_TERRAIN, SG_DEBUG, "Tree texture " << bin->texture);
        bin->normal_map = mat->get_tree_normal_map();
        bin->teffect = mat->get_tree_effect();
        SG_LOG(SG_TERRAIN, SG_DEBUG, "Tree effect " << bin->teffect);
        bin->range   = mat->get_tree_range();
        bin->width   = mat->get_tree_width();
        bin->height  = mat->get_tree_height();
        bin->texture_varieties = mat->get_tree_varieties();
        randomForest.push_back(bin);
    }

    return true;
}

bool VegetationHandler::handleIteration(
    SGMaterial* mat, osg::Image* objectMaskImage,
    const double lon, const double lat,
    osg::Vec2d p, const double D,
    const osg::Vec2d ll_O, const osg::Vec2d ll_x, const osg::Vec2d ll_y,
    const osg::Vec2d t_0, osg::Vec2d t_x, osg::Vec2d t_y,
    float x_scale, float y_scale, osg::Vec2f& pointInTriangle)
{
    const int lat_int = (lat + ll_O.y()) / delta_lat;
    const int lon_int = (lon + ll_O.x()) / delta_lon;

    if (mat->get_wood_coverage() <= 0)
        return false;
    if (pc_map_rand(lon_int, lat_int, 2) > wood_density)
        return false;

    if (mat->get_is_plantation()) {
        p = osg::Vec2d(lon + 0.1 * delta_lon * pc_map_norm(lon_int, lat_int, 0),
                       lat + 0.1 * delta_lat * pc_map_norm(lon_int, lat_int, 1));
    } else {
        p = osg::Vec2d(lon + delta_lon * pc_map_rand(lon_int, lat_int, 0),
                       lat + delta_lat * pc_map_rand(lon_int, lat_int, 1));
    }

    double x = det2(ll_x, p) / D;
    double y = det2(p, ll_y) / D;

    // Check for invalid triangle coordinates.
    if ((x < 0.0) || (y < 0.0) || (x + y > 1.0)) return false;

    // Check against any object mask using green (for trees) channel
    if (checkAgainstObjectMask(objectMaskImage, Green,
                               pc_map_rand(lon_int, lat_int, 3), x, y, x_scale,
                               y_scale, t_0, t_x, t_y)) {
        return false;
    }

    pointInTriangle.set(x, y);
    return true;
}
    
bool VegetationHandler::handleIterationTessellation(
    SGMaterial* mat, osg::Image* objectMaskImage,
    osg::Vec2d p, const double rand1, const double rand2,
    float x_scale, float y_scale)
{
    if (mat->get_wood_coverage() <= 0)
        return false;
    if (rand1 > wood_density)
        return false;

    //if (mat->get_is_plantation()) {
    //    p = osg::Vec2d(lon + 0.1 * delta_lon * pc_map_norm(lon_int, lat_int, 0),
    //                   lat + 0.1 * delta_lat * pc_map_norm(lon_int, lat_int, 1));
    //}

    // Check against any object mask using green (for trees) channel
    if (checkAgainstObjectMask(objectMaskImage, Green,
                               rand2, x_scale,
                               y_scale, p)) {
        return false;
    }
    return true;
}


void VegetationHandler::placeObject(const osg::Vec3 vp) {
    bin->insert(vp);
}

void VegetationHandler::finish(osg::ref_ptr<SGReaderWriterOptions> options,
                               osg::ref_ptr<osg::MatrixTransform> transform,
                               const SGGeod loc) {
    if (randomForest.size() > 0) {
        SG_LOG(SG_TERRAIN, SG_DEBUG,
               "Adding Random Forest " << randomForest.size());
        for (auto iter = randomForest.begin(); iter != randomForest.end();
             iter++) {
            TreeBin *treeBin = *iter;
            SG_LOG(SG_TERRAIN, SG_DEBUG,
                   "  " << treeBin->texture << " " << treeBin->getNumTrees());
        }

        osg::Group *trees = createForest(randomForest, options);
        trees->setNodeMask(SG_NODEMASK_TERRAIN_BIT);
        transform->addChild(trees);
    }
}

/** RandomLightsHandler implementation */
bool RandomLightsHandler::initialize(
    osg::ref_ptr<SGReaderWriterOptions> options,
    osg::ref_ptr<TerrainTile> terrainTile,
    osg::ref_ptr<SGMaterialCache> matcache) {
    SGPropertyNode *propertyNode = options->getPropertyNode().get();

    int lightLODLevel = 6;
    bool useRandomLighting = true;

    if (propertyNode) {
        // Turn on random lighting if OSM buildings are turned off
        useRandomLighting = !propertyNode->getBoolValue(
            "/sim/rendering/osm-buildings", useRandomLighting);
        lightLODLevel = propertyNode->getIntValue(
            "/sim/rendering/static-lod/light-lod-level", lightLODLevel);
    }

    // Do not generate lights for tiles too far away
    if ((!useRandomLighting) ||
        (terrainTile->getTileID().level < lightLODLevel)) {
        return false;
    }

    lightCoverage = 0.0;
    min_coverage_m2 = 1000.0;

    return true;
}

void RandomLightsHandler::setLocation(const SGGeod loc, double r_E_lat,
                                      double r_E_lon) {
    // Approximately 31m x 31m (sqrt(1000) x sqrt(1000)), covering 1000m^2
    //   defined as the minimum light coverage in the documentation
    // 1m latitudeDelta [degrees] = 360 [degrees] / (2 * PI * polarRadius)
    // 1m latitudeDelta [radians] = PI / 180 * latitudeDelta [degrees]
    // 31m latitudeDelta [radians] = sqrt(1000) / latitudeDelta [radians]
    delta_lat = sqrt(1000.0) / r_E_lat;

    // 1m longitudeDelta [degrees] = 360 [degrees] / (2 * PI * equitorialRadius
    //                                   * cos(latitude [radians]))
    // 1m longitudeDelta [radians] = PI / 180 * longitudeDelta [degrees]
    // 31m longitudeDelta [randians] = sqrt(1000) / longitudeDelta [radians]
    delta_lon = sqrt(1000.0) / (r_E_lon * cos(loc.getLatitudeRad()));
}

bool RandomLightsHandler::handleNewMaterial(SGMaterial *mat) {
    if (mat->get_light_coverage() <= 0)
        return false;

    if (bin == NULL) {
        bin = new LightBin();
    }

    lightCoverage = mat->get_light_coverage();

    return true;
}

bool RandomLightsHandler::handleIteration(
    SGMaterial* mat, osg::Image* objectMaskImage,
    const double lon, const double lat,
    osg::Vec2d p, const double D,
    const osg::Vec2d ll_O, const osg::Vec2d ll_x, const osg::Vec2d ll_y,
    const osg::Vec2d t_0, osg::Vec2d t_x, osg::Vec2d t_y,
    float x_scale, float y_scale, osg::Vec2f& pointInTriangle)
{
    const int lat_int = (lat + ll_O.y()) / delta_lat;
    const int lon_int = (lon + ll_O.x()) / delta_lon;

    if (mat->get_light_coverage() <= 0)
        return false;

    // Since we are scanning 31mx31m chunks, 1000/lightCoverage gives the
    //  probability of a particular 31x31 chunk having a light
    //  e.g. if lightCoverage = 10000m^2 (i.e. every light point must
    //  cover around 10000m^2), this roughly equates to
    //  sqrt(10000) * sqrt(10000) 1mx1m chunks, i.e. 100m x 100m, which
    //  translates to ~10 31mx31m chunks, giving us a probability of 1/10.
    if (pc_map_rand(lon_int, lat_int, 4) > (1000.0 / lightCoverage))
        return false;

    p = osg::Vec2d(lon + delta_lon * pc_map_rand(lon_int, lat_int, 0),
                   lat + delta_lat * pc_map_rand(lon_int, lat_int, 1));

    double x = det2(ll_x, p) / D;
    double y = det2(p, ll_y) / D;

    // Check for invalid triangle coordinates.
    if ((x < 0.0) || (y < 0.0) || (x + y > 1.0))
        return false;

    // Check against any object mask using blue (for lights) channel
    if (checkAgainstObjectMask(objectMaskImage, Blue,
                               pc_map_rand(lon_int, lat_int, 5), x, y, x_scale,
                               y_scale, t_0, t_x, t_y)) {
        return false;
    }

    pointInTriangle.set(x, y);

    return true;
}


bool RandomLightsHandler::handleIterationTessellation(
    SGMaterial* mat, osg::Image* objectMaskImage,
    osg::Vec2d p, const double rand1, const double rand2,
    float x_scale, float y_scale)
{
    if (mat->get_light_coverage() <= 0)
        return false;

    // Since we are scanning 31mx31m chunks, 1000/lightCoverage gives the
    //  probability of a particular 31x31 chunk having a light
    //  e.g. if lightCoverage = 10000m^2 (i.e. every light point must
    //  cover around 10000m^2), this roughly equates to
    //  sqrt(10000) * sqrt(10000) 1mx1m chunks, i.e. 100m x 100m, which
    //  translates to ~10 31mx31m chunks, giving us a probability of 1/10.
    if (rand1 > (1000.0 / lightCoverage))
        return false;

    // Check against any object mask using green (for trees) channel
    if (checkAgainstObjectMask(objectMaskImage, Blue,
                               rand2, x_scale,
                               y_scale, p)) {
        return false;
    }
    return true;
}

void RandomLightsHandler::placeObject(const osg::Vec3 vp)
{
    float zombie = pc_map_rand(vp.x(), vp.y() + vp.z(), 6);
    float factor = pc_map_rand(vp.x(), vp.y() + vp.z(), 7);
    factor *= factor;

    float bright = 1;
    SGVec4f color;
    if (zombie > 0.5) {
        // 50% chance of yellowish
        color = SGVec4f(0.9f, 0.9f, 0.3f, bright - factor * 0.2f);
    } else if (zombie > 0.15f) {
        // 35% chance of whitish
        color = SGVec4f(0.9, 0.9f, 0.8f, bright - factor * 0.2f);
    } else if (zombie > 0.05f) {
        // 10% chance of orangish
        color = SGVec4f(0.9f, 0.6f, 0.2f, bright - factor * 0.2f);
    } else {
        // 5% chance of redish
        color = SGVec4f(0.9f, 0.2f, 0.2f, bright - factor * 0.2f);
    }

    // Potential enhancment: Randomize light type (directional vs
    // omnidirectional, size, intensity) Sizes and Intensity tuning source:
    //  https://www.scgrp.com/StresscreteGroup/media/images/products/K118-Washington-LED-Spec-Sheet.pdf
    //  https://www.nationalcityca.gov/home/showpublisheddocument?id=19680
    double size = 30;
    double intensity = 500;
    double onPeriod = 2; // Turn on randomly around sunset

    if (bin == NULL) {
        bin = new LightBin();
    }

    // Place lights at 3m above ground
    bin->insert(
        SGVec3f(vp.x(), vp.y(), vp.z() + 3.0),
        size, intensity, onPeriod, color);
}

void RandomLightsHandler::finish(osg::ref_ptr<SGReaderWriterOptions> options,
                                 osg::ref_ptr<osg::MatrixTransform> transform,
                                 const SGGeod loc) {
    if (bin != NULL && bin->getNumLights() > 0) {
        SG_LOG(SG_TERRAIN, SG_DEBUG,
               "Adding Random Lights " << bin->getNumLights());

        transform->addChild(
            createLights(*bin, osg::Matrix::identity(), options));
    }
}
};
