/* -*-c++-*-
 * VPBMaterialHandler.hxx -- WS30 material-based generation handlers
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

#ifndef VPBMATERIALHANDLER
#define VPBMATERIALHANDLER 1

#include "LightBin.hxx"
#include "TreeBin.hxx"

using namespace osgTerrain;

namespace simgear {

/** Abstract class to represent all material handler classes
 *
 * Extending and implementing the virtual functions in VPBMaterialHandler
 * enables performing arbitrary actions based on landclass materials in
 * an efficient manner. One example is generating things based on the
 * landclass, such as trees, lights or buildings.
 */
class VPBMaterialHandler {
  protected:
    // Deltas to define granularity of scanline
    double delta_lat;
    double delta_lon;
    double max_density_m2;

    VPBMaterialHandler() : delta_lat(0.0), delta_lon(0.0), max_density_m2(0.0) {}
    virtual ~VPBMaterialHandler() {}

    enum ImageChannel { Red, Green, Blue, Alpha };

    // Check against object mask, if any. Returns true if the given point
    // should be masked, false otherwise.
    bool checkAgainstObjectMask(osg::Image *objectMaskImage,
                                ImageChannel channel, double sampleProbability,
                                double x, double y, float x_scale,
                                float y_scale, const osg::Vec2d t_0,
                                osg::Vec2d t_x, osg::Vec2d t_y);
    bool checkAgainstObjectMask(osg::Image *objectMaskImage,
                                ImageChannel channel, double sampleProbability,
                                float x_scale, float y_scale, const osg::Vec2d t);

    double det2(const osg::Vec2d a, const osg::Vec2d b);

  public:
    // Initialize internal state and return true if the handler should be called
    // for the current tile.
    virtual bool initialize(osg::ref_ptr<SGReaderWriterOptions> options,
                            osg::ref_ptr<TerrainTile> terrainTile) = 0;

    // Set the internal state (e.g. deltas) based on the current tile's location
    virtual void setLocation(const SGGeod loc, double r_E_lat,
                             double r_E_lon) = 0;

    // Function that is called when a new material/landclass is detected during
    // the scanline reading process. Return false if the new material is
    // irrelevant to the handler
    virtual bool handleNewMaterial(SGMaterial *mat) = 0;

    // Function that is called for each point in the scanline reading process
    // Return false if the material is irrelevant to the handler
    // Return true if the point should be used to place an object, updating
    //  the pointInTriangle variable with the x and y location within the
    //  current triangle
    virtual bool handleIteration(SGMaterial* mat, osg::Image* objectMaskImage,
                                 const double lon, const double lat,
                                 osg::Vec2d p, const double D,
                                 const osg::Vec2d ll_O, const osg::Vec2d ll_x,
                                 const osg::Vec2d ll_y, const osg::Vec2d t_0,
                                 osg::Vec2d t_x, osg::Vec2d t_y,
                                 float x_scale, float y_scale,
                                 osg::Vec2f& pointInTriangle) = 0;

    virtual bool handleIterationTessellation(SGMaterial* mat, osg::Image* objectMaskImage,
                                osg::Vec2d p, const double rand1, const double rand2,
                                 float x_scale, float y_scale) = 0;

    // Place an object at the point given by vp
    virtual void placeObject(const osg::Vec3 vp) = 0;

    // Function that is called after the scanline is complete
    virtual void finish(osg::ref_ptr<SGReaderWriterOptions> options,
                        osg::ref_ptr<osg::MatrixTransform> transform,
                        const SGGeod loc) = 0;

    double get_delta_lat() { return delta_lat; };
    double get_delta_lon() { return delta_lon; };

    double get_max_density_m2() { return max_density_m2; };
};

/** Vegetation handler
 *
 * This handler takes care of generating vegetation and trees. The code has
 * been ported as-is from the following source:
 * Simgear commit 6d71ab75:
 *  simgear/scene/tgdb/VPBTechnique.cxx : applyTrees()
 */
class VegetationHandler : public VPBMaterialHandler {
  public:
    VegetationHandler() {}
    virtual ~VegetationHandler() {}

    bool initialize(osg::ref_ptr<SGReaderWriterOptions> options,
                    osg::ref_ptr<TerrainTile> terrainTile);
    void setLocation(const SGGeod loc, double r_E_lat, double r_E_lon);
    bool handleNewMaterial(SGMaterial *mat);
    bool handleIteration(SGMaterial* mat, osg::Image* objectMaskImage,
                         const double lon, const double lat, osg::Vec2d p,
                         const double D, const osg::Vec2d ll_O,
                         const osg::Vec2d ll_x, const osg::Vec2d ll_y,
                         const osg::Vec2d t_0, osg::Vec2d t_x, osg::Vec2d t_y,
                         float x_scale, float y_scale, osg::Vec2f& pointInTriangle);
    bool handleIterationTessellation(SGMaterial* mat, osg::Image* objectMaskImage,
                          osg::Vec2d p, const double rand1, const double rand2,
                          float x_scale, float y_scale);
    void placeObject(const osg::Vec3 vp);
    void finish(osg::ref_ptr<SGReaderWriterOptions> options,
                osg::ref_ptr<osg::MatrixTransform> transform, const SGGeod loc);

  private:
    float vegetation_density;
    SGTreeBinList randomForest;

    TreeBin *bin = NULL;
    float wood_coverage = 0.0;
};

/**  Random Lighting handler
 *
 * This handler takes care of generating random tile lighting, taking into
 * account light-coverage of various materials contained in the tile. Lights
 * are currently generated if OSM Buildings are turned off.
 */
class RandomLightsHandler : public VPBMaterialHandler {
  public:
    RandomLightsHandler() {}
    virtual ~RandomLightsHandler() {}

    bool initialize(osg::ref_ptr<SGReaderWriterOptions> options,
                    osg::ref_ptr<TerrainTile> terrainTile);
    void setLocation(const SGGeod loc, double r_E_lat, double r_E_lon);
    bool handleNewMaterial(SGMaterial *mat);
    bool handleIteration(SGMaterial* mat, osg::Image* objectMaskImage,
                         const double lon, const double lat, osg::Vec2d p,
                         const double D, const osg::Vec2d ll_O,
                         const osg::Vec2d ll_x, const osg::Vec2d ll_y,
                         const osg::Vec2d t_0, osg::Vec2d t_x, osg::Vec2d t_y,
                         float x_scale, float y_scale, osg::Vec2f& pointInTriangle);
    bool handleIterationTessellation(SGMaterial* mat, osg::Image* objectMaskImage,
                                osg::Vec2d p, const double rand1, const double rand2,
                                 float x_scale, float y_scale);
    void placeObject(const osg::Vec3 vp);
    void finish(osg::ref_ptr<SGReaderWriterOptions> options,
                osg::ref_ptr<osg::MatrixTransform> transform, const SGGeod loc);

  private:
    LightBin *bin = NULL;
    float lightCoverage = 0.0;
};

}; // namespace simgear

#endif
