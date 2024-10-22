// VPBRasterRenderer.cxx -- Raster renderer for water and other features
//
// Copyright (C) 2024 Stuart Buchanan
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#include <cmath>
#include <tuple>

#include <osgTerrain/TerrainTile>
#include <osgTerrain/Terrain>

#include <osgUtil/LineSegmentIntersector>
#include <osgUtil/IntersectionVisitor>
#include <osgUtil/MeshOptimizers>
#include <osgUtil/Tessellator>

#include <osgDB/FileUtils>
#include <osgDB/ReadFile>

#include <osg/io_utils>
#include <osg/Texture2D>
#include <osg/Texture2DArray>
#include <osg/Texture1D>
#include <osg/Program>
#include <osg/Math>
#include <osg/Timer>

#include <simgear/bvh/BVHSubTreeCollector.hxx>
#include <simgear/debug/logstream.hxx>
#include <simgear/math/sg_random.hxx>
#include <simgear/math/SGMath.hxx>
#include <simgear/scene/material/Effect.hxx>
#include <simgear/scene/material/EffectGeode.hxx>
#include <simgear/scene/model/model.hxx>
#include <simgear/scene/tgdb/VPBElevationSlice.hxx>
#include <simgear/scene/tgdb/VPBTileBounds.hxx>
#include <simgear/scene/util/SGNodeMasks.hxx>
#include <simgear/scene/util/SGReaderWriterOptions.hxx>
#include <simgear/scene/util/SGSceneFeatures.hxx>

#include "VPBRasterRenderer.hxx"
#include "VPBMaterialHandler.hxx"

using namespace osgTerrain;
using namespace simgear;
using std::max;
using std::min;


VPBRasterRenderer::VPBRasterRenderer(const SGPropertyNode* propertyNode, osg::ref_ptr<TerrainTile> tile, const osg::Vec3d world, unsigned int tile_width, unsigned int tile_height) :
_tile(tile),_world(world),_tile_width(tile_width),_tile_height(tile_height)
{
    const std::lock_guard<std::mutex> lock(VPBRasterRenderer::_defaultCoastlineTexture_mutex); 
    if (VPBRasterRenderer::_defaultCoastlineTexture == 0) {
        osg::ref_ptr<osg::Image> defaultCoastImage = new osg::Image();
        defaultCoastImage->allocateImage(1, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE);
        defaultCoastImage->setColor(osg::Vec4f(0.0f,0.0f,0.0f,0.0f), 0,0);

        VPBRasterRenderer::_defaultCoastlineTexture = new osg::Texture2D(defaultCoastImage);
        VPBRasterRenderer::_defaultCoastlineTexture->setMaxAnisotropy(16.0f);
        VPBRasterRenderer::_defaultCoastlineTexture->setResizeNonPowerOfTwoHint(false);
        VPBRasterRenderer::_defaultCoastlineTexture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::NEAREST_MIPMAP_NEAREST);
        VPBRasterRenderer::_defaultCoastlineTexture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::NEAREST_MIPMAP_NEAREST);
        VPBRasterRenderer::_defaultCoastlineTexture->setWrap(osg::Texture::WRAP_S,osg::Texture::CLAMP_TO_EDGE);
        VPBRasterRenderer::_defaultCoastlineTexture->setWrap(osg::Texture::WRAP_T,osg::Texture::CLAMP_TO_EDGE);
    }

    if (propertyNode) {
        const SGPropertyNode* static_lod = propertyNode->getNode("/sim/rendering/static-lod");
        _waterTextureSize =         static_lod->getIntValue("water-texture-size", _coast_features_lod_range);
        _coast_features_lod_range = static_lod->getIntValue("coastline-lod-level", _coast_features_lod_range);
        _coastWidth =               static_lod->getFloatValue("coastline-width", _coastWidth);
    }

    _masterLocator = tile->getLocator();

}

osg::ref_ptr<osg::Texture2D> VPBRasterRenderer::generateCoastTexture() {

    if ((_coastFeatureLists.size() == 0 ) || (_tile->getTileID().level < _coast_features_lod_range)) {
        return _defaultCoastlineTexture;
    }

    osg::ref_ptr<osg::Image> coastImage = generateCoastImage();

    if (coastImage) {
        osg::ref_ptr<osg::Texture2D> coastTexture = new osg::Texture2D();
        coastTexture->setImage(coastImage);
        coastTexture->setMaxAnisotropy(16.0f);
        coastTexture->setResizeNonPowerOfTwoHint(false);
        coastTexture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::NEAREST_MIPMAP_NEAREST);
        coastTexture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::NEAREST_MIPMAP_NEAREST);
        coastTexture->setWrap(osg::Texture::WRAP_S,osg::Texture::CLAMP_TO_EDGE);
        coastTexture->setWrap(osg::Texture::WRAP_T,osg::Texture::CLAMP_TO_EDGE);
        return coastTexture;
    } else {
        return VPBRasterRenderer::_defaultCoastlineTexture;
    }
}

osg::ref_ptr<osg::Image> VPBRasterRenderer::generateCoastImage() {
    // Get all appropriate coasts.  We assume that the VPB terrain tile is smaller than a Bucket size.
    float tileSize = sqrt(_tile_width * _tile_width + _tile_height * _tile_height);
    const SGGeod loc = SGGeod::fromCart(toSG(_world));
    const SGBucket bucket = SGBucket(loc);
    // We're in Earth-centered coordinates, so "up" is simply directly away from (0,0,0)
    osg::Vec3d up = _world;
    up.normalize();

    TileBounds tileBounds(_masterLocator, up);

    bool coastsFound = false;

    // Do a quick pass to check that there is some coastline to generate here. This is worth doing
    // because the LoD scheme results in many tiles with no coastlines.
    for (auto coasts = _coastFeatureLists.begin(); coasts != _coastFeatureLists.end(); ++coasts) {
        if (coasts->first != bucket) continue;
        const CoastlineBinList coastBins = coasts->second;

        for (auto rb = coastBins.begin(); !coastsFound && (rb != coastBins.end()); ++rb)
        {
            auto coastFeatures = (*rb)->getCoastlines();
            for (auto r = coastFeatures.begin(); !coastsFound && (r != coastFeatures.end()); ++r) {
                for (auto p = r->_nodes.begin(); !coastsFound && (p != r->_nodes.end()); ++p) {
                    if (tileBounds.insideTile(*p)) {
                        coastsFound = true;
                    }
                }
            }
        }
    }

    if (coastsFound) {
        // We have at least one coast here, so generate a coastline texture for the tile and
        // render to it.
        osg::Image* coastImage = new osg::Image();
        coastImage->allocateImage(_waterTextureSize, _waterTextureSize, 1, GL_RGBA, GL_UNSIGNED_BYTE);
        for (unsigned int y = 0; y < _waterTextureSize; ++y) {
            for (unsigned int x = 0; x < _waterTextureSize; ++x) {
                coastImage->setColor(osg::Vec4(0u,0u,0u,0u), x, y);
            }
        }

        for (auto coasts = _coastFeatureLists.begin(); coasts != _coastFeatureLists.end(); ++coasts) {
            if (coasts->first != bucket) continue;
            const CoastlineBinList coastBins = coasts->second;

            for (auto rb = coastBins.begin(); rb != coastBins.end(); ++rb)
            {
                auto coastFeatures = (*rb)->getCoastlines();

                for (auto r = coastFeatures.begin(); r != coastFeatures.end(); ++r) {
                    auto clipped = tileBounds.clipToTile(r->_nodes);
                    if (clipped.size() > 1) {                    
                        // We need at least two points to render a line.
                        LineFeatureBin::LineFeature line = LineFeatureBin::LineFeature(clipped, _coastWidth);
                        addCoastline(coastImage, line, _waterTextureSize, tileSize, _coastWidth);
                    }
                }
            }
        }

        return coastImage;
    }

    return NULL;
}



void VPBRasterRenderer::writeShoreStripe(osg::Image* waterTexture, unsigned int waterTextureSize, float tileSize, float coastWidth, float x, float y, int dx, int dy) {

    // We need to create a shoreline of width coastWidth to define it better and to hide any discrepancy in the underlying terrain mesh
    // which may be either mis-aligned or simply of insufficient resolution.
    float waterTextureResolution = tileSize / waterTextureSize;
    int width = (int) coastWidth / waterTextureResolution;
    int aboveHWLtxUnits = 0.3*width;
    int belowHWLtxUnits = width - aboveHWLtxUnits;

    // Above the high water mark we just want sand.  However there can be interpolation artifacts right at the edge causing a border or water. To avoid
    // this we create an extra texel where the G channel is used to ensure we get a hard fall-off to the underlying texture
    // on the adjacent sand texture unit.
    updateWaterTexture(waterTexture, waterTextureSize, osg::Vec4(0, 255u, 0u, 255u), x - dx*(aboveHWLtxUnits + 1), y -dy*(aboveHWLtxUnits + 1));
    
    // Create the sand above the high water mark
    for (int d = 0; d < aboveHWLtxUnits; d++) {
        updateWaterTexture(waterTexture, waterTextureSize, osg::Vec4(0u, 0u, 255u, 255u), x - dx*d, y -dy*d);
    }

    // Create the shoreline below the high water level which will gradually merge into the underlying terrain mesh
    for (int d = 0; d < belowHWLtxUnits; d++) {
        updateWaterTexture(waterTexture, waterTextureSize, osg::Vec4(0u, 0u, 255u, 255u)*(belowHWLtxUnits -d)/belowHWLtxUnits, x + dx*d, y + dy*d);
    }
}

void VPBRasterRenderer::addCoastline(osg::Image* waterTexture, LineFeatureBin::LineFeature line, unsigned int waterTextureSize, float tileSize, float coastWidth) {
    

    if (line._nodes.size() < 2) { 
        SG_LOG(SG_TERRAIN, SG_ALERT, "Coding error - LineFeatureBin::LineFeature with fewer than two nodes"); 
        return; 
    }

    auto iter = line._nodes.begin();

    // LineFeature is in Model coordinates, but for the rasterization we will use local coordinates.
    osg::Vec3d start;
    bool success = _masterLocator->convertModelToLocal(*iter, start);
    if (!success) {
        SG_LOG(SG_TERRAIN, SG_ALERT, "Unable to convert from model coordinates to local: " << *iter); 
        return; 
    }

    iter++;
    for (; iter != line._nodes.end(); iter++) {
        osg::Vec3d end;
        success = _masterLocator->convertModelToLocal(*iter, end);
        if (!success) {
            SG_LOG(SG_TERRAIN, SG_ALERT, "Unable to convert from model coordinates to local: " << *iter); 
            continue; 
        }

        // We now have two points in local (2d) space, so rasterize the line between them onto the water texture.
        // We use a simple version of a DDA to render the lines.  We should replace this with a scanline algorithm to handle coastlines
        // and waterbodies efficiently.
        // Do everything in the texture coordinates.
        float dx = (end.x() - start.x()) * waterTextureSize;
        float dy = (end.y() - start.y()) * waterTextureSize;
        float step = abs(dy);
        bool steep = true;
        if (abs(dx) >= abs(dy)) {
            step = abs(dx);
            steep = false;
        }
        dx = dx / step;
        dy = dy / step;
        float x = start.x() * waterTextureSize;
        float y = start.y() * waterTextureSize;
        int i = 0;
        while (i <= step) {
            if ((x >= 0.0f) && (y >= 0.0f) && (x < waterTextureSize) && (y < waterTextureSize)) {
                // The line defines the Mean High Water Level.  By definition, the sea is always to
                // the right of the line.  We want to fill in a section of sea and shore to cover up any issues 
                // between the OSM data and the landclass texture.
                if (steep) {
                    // A steep line, so we are should add width in the x-axis
                    if (dy < 0) {
                        // We are travelling downwards, so the seaward side is -x
                        writeShoreStripe(waterTexture, waterTextureSize, tileSize, coastWidth, x , y, -1, 0);
                    } else {
                        // We are travelling upwards, so the seaward side is +x
                        writeShoreStripe(waterTexture, waterTextureSize, tileSize, coastWidth, x , y, 1, 0);
                    }
                } else {
                    // Not a steep line, so we should add width in the y axis
                    if (dx < 0) {
                        // We are travelling right to left, so the seaward side is +y
                        writeShoreStripe(waterTexture, waterTextureSize, tileSize, coastWidth, x , y, 0, 1);
                    } else {
                        // We are travelling left to right, so the seaward side is -y
                        writeShoreStripe(waterTexture, waterTextureSize, tileSize, coastWidth, x , y, 0, -1);
                    }
                }
            } 

            x = x + dx;
            y = y + dy;
            i = i + 1;
        }

        start = end;
    }
}



// Update the color of a particular texture pixel, clipping to the texture and not over-writing a larger value.
void VPBRasterRenderer::updateWaterTexture(osg::Image* waterTexture, unsigned int waterTextureSize, osg::Vec4 color, float x, float y) {
    if (floor(x) < 0.0) return;
    if (floor(x) > (waterTextureSize -1)) return;
    if (floor(y) < 0.0) return;
    if (floor(y) > (waterTextureSize -1)) return;
    auto clamp = [waterTextureSize](float l) { return (unsigned int) fmaxf(0, fminf(floor(l), waterTextureSize - 1)); };
    unsigned int clamped_x = clamp(x);
    unsigned int clamped_y = clamp(y);    
    osg::Vec4 new_color = max(waterTexture->getColor(clamped_x, clamped_y), color);
    waterTexture->setColor(new_color, clamped_x, clamped_y);
}

void VPBRasterRenderer::addCoastlineList(SGBucket bucket, CoastlineBinList coastline)
{
    if (coastline.empty()) return;

    // Block to mutex the List alone
    {
        const std::lock_guard<std::mutex> lock(VPBRasterRenderer::_coastFeatureLists_mutex); // Lock the _lineFeatureLists for this scope
        _coastFeatureLists.push_back(std::pair(bucket, coastline));
    }

}

void VPBRasterRenderer::unloadFeatures(SGBucket bucket)
{
    SG_LOG(SG_TERRAIN, SG_DEBUG, "Erasing all features with entry " << bucket);

    {
        const std::lock_guard<std::mutex> lockcoasts(VPBRasterRenderer::_coastFeatureLists_mutex); // Lock the _coastFeatureLists for this scope
        for (auto lf = _coastFeatureLists.begin(); lf != _coastFeatureLists.end(); ++lf) {
            if (lf->first == bucket) {
                SG_LOG(SG_TERRAIN, SG_DEBUG, "Unloading  area feature for " << bucket);
                auto coastfeatureBinList = lf->second;
                coastfeatureBinList.clear();
            }
        }
    }
}