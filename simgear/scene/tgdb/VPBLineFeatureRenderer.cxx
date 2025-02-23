// VPBLineFeatureRenderer.cxx -- Mesh renderer for line features
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
#include <simgear/scene/tgdb/VPBTechnique.hxx>
#include <simgear/scene/util/SGNodeMasks.hxx>
#include <simgear/scene/util/SGReaderWriterOptions.hxx>
#include <simgear/scene/util/SGSceneFeatures.hxx>

#include "VPBLineFeatureRenderer.hxx"
#include "VPBMaterialHandler.hxx"

using namespace osgTerrain;
using namespace simgear;

VPBLineFeatureRenderer::VPBLineFeatureRenderer(osg::ref_ptr<TerrainTile> tile)
{
    _tileLevel = tile->getTileID().level;
    _masterLocator = tile->getLocator();
}

void VPBLineFeatureRenderer::applyLineFeatures(BufferData& buffer, osg::ref_ptr<SGReaderWriterOptions> options, osg::ref_ptr<SGMaterialCache> matcache)
{
    
    unsigned int line_features_lod_range = 6;
    float minWidth = 9999.9;

    SGPropertyNode_ptr propertyNode = options->getPropertyNode();

    if (propertyNode) {
        const SGPropertyNode* static_lod = propertyNode->getNode("/sim/rendering/static-lod");
        line_features_lod_range = static_lod->getIntValue("line-features-lod-level", line_features_lod_range);
        if (static_lod->getChildren("lod-level").size() > _tileLevel) {
            minWidth = static_lod->getChildren("lod-level")[_tileLevel]->getFloatValue("line-features-min-width", minWidth);
        }
    }

    if (! matcache) {
        SG_LOG(SG_TERRAIN, SG_ALERT, "Unable to get materials library to generate roads");
        return;
    }    

    if (_tileLevel < line_features_lod_range) {
        // Do not generate line features for tiles too far away
        return;
    }

    SG_LOG(SG_TERRAIN, SG_DEBUG, "Generating line features of width > " << minWidth << " for tile LoD level " << _tileLevel);

    Atlas* atlas = matcache->getAtlas();
    SGMaterial* mat = 0;

    if (! buffer._lineFeatures) buffer._lineFeatures = new osg::Group();

    // Get all appropriate roads.  We assume that the VPB terrain tile is smaller than a Bucket size.
    LightBin lightbin;
    const osg::Vec3d world = buffer._transform->getMatrix().getTrans();

    const SGGeod loc = SGGeod::fromCart(toSG(world));
    const SGBucket bucket = SGBucket(loc);
    std::string material_name = "";
    for (auto roads = _lineFeatureLists.begin(); roads != _lineFeatureLists.end(); ++roads) {
        auto r = *roads;
        if (r.first != bucket) continue;
        LineFeatureBinList roadBins = r.second;

        for (LineFeatureBinList::iterator rb = roadBins.begin(); rb != roadBins.end(); ++rb)
        {
            if (material_name != (*rb)->getMaterial()) {
                // Cache the material to reduce lookups.
                mat = matcache->find((*rb)->getMaterial());
                material_name = (*rb)->getMaterial();
            }

            if (!mat) {
                SG_LOG(SG_TERRAIN, SG_ALERT, "Unable to find material " << (*rb)->getMaterial() << " at " << loc << " " << bucket);
                continue;
            }    

            //  Generate a geometry for this set of roads.
            osg::Vec3Array* v = new osg::Vec3Array;
            osg::Vec2Array* t = new osg::Vec2Array;
            osg::Vec3Array* n = new osg::Vec3Array;
            osg::Vec4Array* c = new osg::Vec4Array;
            std::vector<osg::Vec3f>* lights = new std::vector<osg::Vec3f>;
            std::vector<float>* rotations = new std::vector<float>;

            auto lineFeatures = (*rb)->getLineFeatures();

            for (auto r = lineFeatures.begin(); r != lineFeatures.end(); ++r) {
                if (r->_width > minWidth) generateLineFeature(buffer, *r, buffer._transform->getMatrix(), v, t, n, lights, rotations, mat);
            }

            if (v->size() == 0) {
                v->unref();
                t->unref();
                n->unref();
                c->unref();
                delete lights;
                delete rotations;
                continue;
            }

            c->push_back(osg::Vec4d(1.0,1.0,1.0,1.0));

            osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry;
            geometry->setVertexArray(v);
            geometry->setTexCoordArray(0, t, osg::Array::BIND_PER_VERTEX);
            geometry->setTexCoordArray(1, t, osg::Array::BIND_PER_VERTEX);
            geometry->setNormalArray(n, osg::Array::BIND_PER_VERTEX);
            geometry->setColorArray(c, osg::Array::BIND_OVERALL);
            geometry->setUseDisplayList( false );
            geometry->setUseVertexBufferObjects( true );
            geometry->addPrimitiveSet( new osg::DrawArrays( GL_TRIANGLES, 0, v->size()) );

            EffectGeode* geode = new EffectGeode;
            geode->addDrawable(geometry);

            geode->setMaterial(mat);
            geode->setEffect(mat->get_one_effect(0));
            geode->runGenerators(geometry);
            geode->setNodeMask( ~(simgear::CASTSHADOW_BIT | simgear::MODELLIGHT_BIT) );

            osg::StateSet* stateset = geode->getOrCreateStateSet();
            stateset->addUniform(new osg::Uniform(VPBTechnique::MODEL_OFFSET, (osg::Vec3f) buffer._transform->getMatrix().getTrans()));

            atlas->addUniforms(stateset);

            buffer._lineFeatures->addChild(geode);

            if (lights->size() > 0) {
                const double size = mat->get_light_edge_size_cm();
                const double intensity = mat->get_light_edge_intensity_cd();
                const SGVec4f color = mat->get_light_edge_colour();
                const double horiz = mat->get_light_edge_angle_horizontal_deg();
                const double vertical = mat->get_light_edge_angle_vertical_deg();
                const std::string lampPostModel = mat->get_light_model();

                // Assume street lights point down.
                osg::Vec3d up = world;
                up.normalize();
                const SGVec3f direction = toSG(- (osg::Vec3f) up);

                std::for_each(lights->begin(), lights->end(), 
                    [&, size, intensity, color, direction, horiz, vertical] (osg::Vec3f p) { lightbin.insert(toSG(p), size, intensity, 1, color, direction, horiz, vertical); } );

                if (lampPostModel != "") {
                    
                    ObjectInstanceBin streetlampBin = ObjectInstanceBin(lampPostModel);
                    for (std::size_t idx = 0; idx < lights->size(); ++idx) {
                        streetlampBin.insert(lights->at(idx), osg::Vec3f(rotations->at(idx), 0.0f, 0.0f));
                    }

                    if (streetlampBin.getNumInstances() > 0) buffer._transform->addChild(createObjectInstances(streetlampBin, osg::Matrix::identity(), options));
                }
            }
            delete lights;
            delete rotations;
        }
    }

    if (buffer._lineFeatures->getNumChildren() > 0) {
        // We have some line features, so add them
        SG_LOG(SG_TERRAIN, SG_DEBUG, "Generated " <<  buffer._lineFeatures->getNumChildren() << " roads of width > " << minWidth << "m for tile LoD level " << _tileLevel);
        buffer._transform->addChild(buffer._lineFeatures.get());
    }

    if (lightbin.getNumLights() > 0) buffer._transform->addChild(createLights(lightbin, osg::Matrix::identity(), options));
}

void VPBLineFeatureRenderer::generateLineFeature(BufferData& buffer, LineFeatureBin::LineFeature road, osg::Matrix localToWorldMatrix, osg::Vec3Array* v, osg::Vec2Array* t, osg::Vec3Array* n, std::vector<osg::Vec3f>* lights, std::vector<float>* rotations, SGMaterial* mat)
{
    const unsigned int ysize = mat->get_ysize();
    const bool   light_edge_offset = mat->get_light_edge_offset();
    const bool   light_edge_left = mat->get_light_edge_left();
    const bool   light_edge_right = mat->get_light_edge_right();
    const double light_edge_spacing = mat->get_light_edge_spacing_m();
    const double light_edge_height = mat->get_light_edge_height_m();
    const double x0 = mat->get_line_feature_tex_x0();
    const double x1 = mat->get_line_feature_tex_x1();
    const double elevation_offset_m = mat->get_line_feature_offset_m();

    osg::Vec3d modelCenter = localToWorldMatrix.getTrans();

    // We clip to the tile in a geocentric space, as that's what the road information
    // is in.
    osg::Vec3d modelNormal = modelCenter;
    modelNormal.normalize();
    TileBounds tileBounds(buffer._masterLocator, modelNormal);
    std::list<osg::Vec3d> nodes = tileBounds.clipToTile(road._nodes);

    // However the geometry is in Z-up space, so "up" is simply (0,0,1)
    osg::Vec3d up = osg::Vec3d(0.0,0.0,1.0);

    // Rotation from the geocentric coordinates to a Z-up coordinate system
    osg::Quat rot = localToWorldMatrix.getRotate().inverse();

    // We need at least two node to make a road.
    if (nodes.size() < 2) return; 

    osg::Vec3d ma, mb;
    std::list<osg::Vec3d> roadPoints;
    auto road_iter = nodes.begin();

    ma = getMeshIntersection(buffer, rot * (*road_iter - modelCenter));
    road_iter++;

    for (; road_iter != nodes.end(); road_iter++) {
        mb = getMeshIntersection(buffer, rot * (*road_iter - modelCenter));
        auto esl = VPBElevationSlice::computeVPBElevationSlice(buffer._landGeometry, ma, mb, up);

        for(auto eslitr = esl.begin(); eslitr != esl.end(); ++eslitr) {
            roadPoints.push_back(*eslitr);
        }

        // Now traverse the next segment
        ma = mb;
    }

    if (roadPoints.size() == 0) return;

    // We now have a series of points following the topography of the elevation mesh.

    auto iter = roadPoints.begin();
    osg::Vec3d start = *iter;
    iter++;

    osg::Vec3d last_spanwise =  (*iter - start)^ up;
    last_spanwise.normalize();

    float yTexBaseA = 0.0f;
    float yTexBaseB = 0.0f;
    float last_light_distance = 0.0f;

    for (; iter != roadPoints.end(); iter++) {

        osg::Vec3d end = *iter;

        // Ignore tiny segments - likely artifacts of the elevation slicer
        if ((end - start).length2() < 1.0) continue;

        // Find a spanwise vector
        osg::Vec3d spanwise = ((end-start) ^ up);
        spanwise.normalize();

        // Define the road extents
        const osg::Vec3d a = start - last_spanwise * road._width * 0.5 + up * elevation_offset_m;
        const osg::Vec3d b = start + last_spanwise * road._width * 0.5 + up * elevation_offset_m;
        const osg::Vec3d c = end   - spanwise * road._width * 0.5 + up * elevation_offset_m;
        const osg::Vec3d d = end   + spanwise * road._width * 0.5 + up * elevation_offset_m;

        // Determine the x and y texture coordinates for the edges
        const float yTexA = yTexBaseA + (c-a).length() / ysize;
        const float yTexB = yTexBaseB + (d-b).length() / ysize;

        // Now generate two triangles, .
        v->push_back(a);
        v->push_back(b);
        v->push_back(c);

        t->push_back(osg::Vec2d(x0, yTexBaseA));
        t->push_back(osg::Vec2d(x1, yTexBaseB));
        t->push_back(osg::Vec2d(x0, yTexA));

        v->push_back(b);
        v->push_back(d);
        v->push_back(c);

        t->push_back(osg::Vec2d(x1, yTexBaseB));
        t->push_back(osg::Vec2d(x1, yTexB));
        t->push_back(osg::Vec2d(x0, yTexA));

        // Normal is straight from the quad
        osg::Vec3d normal = -(end-start)^spanwise;
        normal.normalize();
        for (unsigned int i = 0; i < 6; i++) n->push_back(normal);

        // Heading is from the spanwise vector, which will be coordinates
        // on a unit circle on the x-y plane.  acos returns in range 0-pi, 
        // so we need to adjust to cover the full -pi - pi range.  Fortunately
        // this is easy.
        double theta = acos(spanwise * osg::Vec3d(1.0,0.0,0.0)) *180.0/M_PI;
        if (spanwise.y() < 0.0f) {
            theta = - theta;
        }

        start = end;
        yTexBaseA = yTexA;
        yTexBaseB = yTexB;
        last_spanwise = spanwise;
        float edge_length = (c-a).length();
        float start_a = last_light_distance;
        float start_b = start_a;

        if ((road._attributes == 1) && (light_edge_spacing > 0.0)) {
            // We have some edge lighting.  Traverse edges a-c and b-d adding lights as appropriate.
            
            // Handle the case where lights are on alternate sides of the road rather than in pairs
            if (light_edge_offset) start_b = fmodf(start_b + light_edge_spacing * 0.5, light_edge_spacing);

            osg::Vec3f p1 = (c-a);
            p1.normalize();

            if (light_edge_left) {
                while (start_a < edge_length) {
                    lights->push_back(a + (osg::Vec3f) p1 * start_a + up * (light_edge_height + 1.0));
                    rotations->push_back(theta - 180.0); // Left side assumed to require rotation
                    start_a += light_edge_spacing;
                }
            }

            if (light_edge_right) {
                osg::Vec3f p2 = (d-b);
                p2.normalize();

                while (start_b < edge_length) {
                    lights->push_back(b + (osg::Vec3f) p2 * start_b + up * (light_edge_height + 1.0));
                    rotations->push_back(theta); //Right side assumed to not to require rotation.
                    start_b += light_edge_spacing;
                }
            }

            // Determine the position for the first light on the next road segment.
            if (light_edge_left) {
                last_light_distance = fmodf(start_a + edge_length, light_edge_spacing);
            } else {
                last_light_distance = fmodf(start_b + edge_length, light_edge_spacing);
            }
        }
    }
}

void VPBLineFeatureRenderer::addLineFeatureList(SGBucket bucket, LineFeatureBinList roadList)
{
    if (roadList.empty()) return;

    // Block to mutex the List alone
    {
        const std::lock_guard<std::mutex> lock(VPBLineFeatureRenderer::_lineFeatureLists_mutex); // Lock the _lineFeatureLists for this scope
        _lineFeatureLists.push_back(std::pair(bucket, roadList));
    }

}

void VPBLineFeatureRenderer::unloadFeatures(SGBucket bucket)
{
    SG_LOG(SG_TERRAIN, SG_DEBUG, "Erasing all features with entry " << bucket);

    {
        const std::lock_guard<std::mutex> locklines(VPBLineFeatureRenderer::_lineFeatureLists_mutex); // Lock the _lineFeatureLists for this scope
        for (auto lf = _lineFeatureLists.begin(); lf != _lineFeatureLists.end(); ++lf) {
            if (lf->first == bucket) {
                SG_LOG(SG_TERRAIN, SG_DEBUG, "Unloading  line feature for " << bucket);
                auto linefeatureBinList = lf->second;
                linefeatureBinList.clear();
            }
        }
    }
}

// Find the intersection of a given SGGeod with the terrain mesh
osg::Vec3d VPBLineFeatureRenderer::getMeshIntersection(BufferData& buffer, osg::Vec3d pt) 
{
    osg::ref_ptr<osgUtil::LineSegmentIntersector> intersector;
    intersector = new osgUtil::LineSegmentIntersector(pt + osg::Vec3d(0.0, 0.0, -100.0), pt + osg::Vec3d(0.0, 0.0, 8000.0));
    osgUtil::IntersectionVisitor visitor(intersector.get());
    buffer._landGeometry->accept(visitor);

    if (intersector->containsIntersections()) {
        // We have an intersection with the terrain model, so return it
        return intersector->getFirstIntersection().getWorldIntersectPoint();
    } else {
        // No intersection.  Likely this point is outside our geometry.  So just return the original element.
        return pt;
    }
}
