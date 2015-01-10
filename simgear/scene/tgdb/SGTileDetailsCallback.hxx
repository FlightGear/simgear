// obj.cxx -- routines to handle loading scenery and building the plib
//            scene graph.
//
// Written by Curtis Olson, started October 1997.
//
// Copyright (C) 1997  Curtis L. Olson  - http://www.flightgear.org/~curt
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
//
// $Id$


#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include <osg/LOD>
#include <osgUtil/Simplifier>

#include <boost/foreach.hpp>

#include <simgear/scene/material/matmodel.hxx>
#include <simgear/scene/model/SGOffsetTransform.hxx>
#include <simgear/scene/util/QuadTreeBuilder.hxx>
#include <simgear/scene/util/SGReaderWriterOptions.hxx>
#include <simgear/scene/util/OptionsReadFileCallback.hxx>
#include <simgear/scene/util/SGNodeMasks.hxx>

#include "SGNodeTriangles.hxx"
#include "GroundLightManager.hxx"
#include "SGLightBin.hxx"
#include "SGDirectionalLightBin.hxx"
#include "SGModelBin.hxx"
#include "SGBuildingBin.hxx"
#include "TreeBin.hxx"

#include "pt_lights.hxx"


typedef std::list<SGLightBin> SGLightListBin;
typedef std::list<SGDirectionalLightBin> SGDirectionalLightListBin;

#define SG_SIMPLIFIER_RATIO         (0.001)
#define SG_SIMPLIFIER_MAX_LENGTH    (1000.0)
#define SG_SIMPLIFIER_MAX_ERROR     (2000.0)
#define SG_OBJECT_RANGE             (9000.0)
#define SG_TILE_RADIUS              (14000.0)
#define SG_TILE_MIN_EXPIRY          (180.0)

using namespace simgear;

// QuadTreeBuilder is used by Random Objects Generator
typedef std::pair<osg::Node*, int> ModelLOD;
struct MakeQuadLeaf {
    osg::LOD* operator() () const { return new osg::LOD; }
};
struct AddModelLOD {
    void operator() (osg::LOD* leaf, ModelLOD& mlod) const
    {
        leaf->addChild(mlod.first, 0, mlod.second);
    }
};
struct GetModelLODCoord {
    GetModelLODCoord() {}
    GetModelLODCoord(const GetModelLODCoord& rhs)
    {}
    osg::Vec3 operator() (const ModelLOD& mlod) const
    {
        return mlod.first->getBound().center();
    }
};
typedef QuadTreeBuilder<osg::LOD*, ModelLOD, MakeQuadLeaf, AddModelLOD,
                        GetModelLODCoord>  RandomObjectsQuadtree;


// needs constructor
static unsigned int num_tdcb = 0;
class SGTileDetailsCallback : public OptionsReadFileCallback {
public:
    SGTileDetailsCallback() 
    {
        num_tdcb++;
    }

    virtual ~SGTileDetailsCallback() 
    {
        num_tdcb--;
        SG_LOG( SG_GENERAL, SG_INFO, "SGTileDetailsCallback::~SGTileDetailsCallback() num cbs left " << num_tdcb  );
    }
    
    virtual osgDB::ReaderWriter::ReadResult readNode(
        const std::string&, const osgDB::Options*)
    {
        SGMaterialLibPtr matlib;
        osg::ref_ptr<SGMaterialCache> matcache; 
        
        osg::ref_ptr<osg::Group> group = new osg::Group;
        group->setDataVariance(osg::Object::STATIC);

        // generate textured triangle list
        std::vector<SGTriangleInfo> matTris;
        GetNodeTriangles nodeTris(_gbs_center, &matTris);
        _rootNode->accept( nodeTris );

        // build matcache
        matlib = _options->getMaterialLib();
        if (matlib) {
            SGGeod geodPos = SGGeod::fromCart(_gbs_center);            
            matcache = matlib->generateMatCache(geodPos);
        }
        
#if 0
        // TEST : See if we can regenerate landclass shapes from node
        for ( unsigned int i=0; i<matTris.size(); i++ ) {
            matTris[i].dumpBorder(_gbs_center);
        }
#endif

        osg::Node* node = loadTerrain();
        if (node) {
            group->addChild(node);
        }

        osg::LOD* lightLOD = generateLightingTileObjects(matTris, matcache);
        if (lightLOD) {
            group->addChild(lightLOD);
        }

        osg::LOD* objectLOD = generateRandomTileObjects(matTris, matcache);
        if (objectLOD) {
            group->addChild(objectLOD);
        }
        
        return group.release();
    }

    static SGVec4f getMaterialLightColor(const SGMaterial* material)
    {
        if (!material) {
            return SGVec4f(1, 1, 1, 0.8);
        }
        
        return material->get_light_color();
    }
    
    static void
    addPointGeometry(SGLightBin& lights,
                     const std::vector<SGVec3d>& vertices,
                     const SGVec4f& color,
                     const int_list& pts_v)
    {
        for (unsigned i = 0; i < pts_v.size(); ++i)
            lights.insert(toVec3f(vertices[pts_v[i]]), color);
    }
    
    static void
    addPointGeometry(SGDirectionalLightBin& lights,
                     const std::vector<SGVec3d>& vertices,
                     const std::vector<SGVec3f>& normals,
                     const SGVec4f& color,
                     const int_list& pts_v,
                     const int_list& pts_n)
    {
        // If the normal indices match the vertex indices, use seperate
        // normal indices. Else reuse the vertex indices for the normals.
        if (pts_v.size() == pts_n.size()) {
            for (unsigned i = 0; i < pts_v.size(); ++i)
                lights.insert(toVec3f(vertices[pts_v[i]]), normals[pts_n[i]], color);
        } else {
            for (unsigned i = 0; i < pts_v.size(); ++i)
                lights.insert(toVec3f(vertices[pts_v[i]]), normals[pts_v[i]], color);
        }
    }
    
    bool insertPtGeometry(const SGBinObject& obj, SGMaterialCache* matcache)
    {
        if (obj.get_pts_v().size() != obj.get_pts_n().size()) {
            SG_LOG(SG_TERRAIN, SG_ALERT,
                   "Group list sizes for points do not match!");
            return false;
        }
        
        for (unsigned grp = 0; grp < obj.get_pts_v().size(); ++grp) {
            std::string materialName = obj.get_pt_materials()[grp];
            SGMaterial* material = matcache->find(materialName);
            SGVec4f color = getMaterialLightColor(material);
            
            if (3 <= materialName.size() && materialName.substr(0, 3) != "RWY") {
                // Just plain lights. Not something for the runway.
                addPointGeometry(tileLights, obj.get_wgs84_nodes(), color,
                                 obj.get_pts_v()[grp]);
            } else if (materialName == "RWY_BLUE_TAXIWAY_LIGHTS"
                || materialName == "RWY_GREEN_TAXIWAY_LIGHTS") {
                addPointGeometry(taxiLights, obj.get_wgs84_nodes(), obj.get_normals(),
                                 color, obj.get_pts_v()[grp], obj.get_pts_n()[grp]);
                } else if (materialName == "RWY_VASI_LIGHTS") {
                    vasiLights.push_back(SGDirectionalLightBin());
                    addPointGeometry(vasiLights.back(), obj.get_wgs84_nodes(),
                                     obj.get_normals(), color, obj.get_pts_v()[grp],
                                     obj.get_pts_n()[grp]);
                } else if (materialName == "RWY_SEQUENCED_LIGHTS") {
                    rabitLights.push_back(SGDirectionalLightBin());
                    addPointGeometry(rabitLights.back(), obj.get_wgs84_nodes(),
                                     obj.get_normals(), color, obj.get_pts_v()[grp],
                                     obj.get_pts_n()[grp]);
                } else if (materialName == "RWY_ODALS_LIGHTS") {
                    odalLights.push_back(SGLightBin());
                    addPointGeometry(odalLights.back(), obj.get_wgs84_nodes(),
                                     color, obj.get_pts_v()[grp]);
                } else if (materialName == "RWY_YELLOW_PULSE_LIGHTS") {
                    holdshortLights.push_back(SGDirectionalLightBin());
                    addPointGeometry(holdshortLights.back(), obj.get_wgs84_nodes(),
                                     obj.get_normals(), color, obj.get_pts_v()[grp],
                                     obj.get_pts_n()[grp]);
                } else if (materialName == "RWY_GUARD_LIGHTS") {
                    guardLights.push_back(SGDirectionalLightBin());
                    addPointGeometry(guardLights.back(), obj.get_wgs84_nodes(),
                                     obj.get_normals(), color, obj.get_pts_v()[grp],
                                     obj.get_pts_n()[grp]);
                } else if (materialName == "RWY_REIL_LIGHTS") {
                    reilLights.push_back(SGDirectionalLightBin());
                    addPointGeometry(reilLights.back(), obj.get_wgs84_nodes(),
                                     obj.get_normals(), color, obj.get_pts_v()[grp],
                                     obj.get_pts_n()[grp]);
                } else {
                    // what is left must be runway lights
                    addPointGeometry(runwayLights, obj.get_wgs84_nodes(),
                                     obj.get_normals(), color, obj.get_pts_v()[grp],
                                     obj.get_pts_n()[grp]);
                }
        }
        
        return true;
    }
    
    
    
    // Load terrain if required
    // todo - this is the same code as when we load a btg from the .STG - can we combine?
    osg::Node* loadTerrain()
    {
      if (! _loadterrain)
        return NULL;

      SGBinObject tile;
      if (!tile.read_bin(_path))
        return NULL;

      SGMaterialLibPtr matlib;
      SGMaterialCache* matcache = 0;
      bool useVBOs = false;
      bool simplifyNear    = false;
      double ratio       = SG_SIMPLIFIER_RATIO;
      double maxLength   = SG_SIMPLIFIER_MAX_LENGTH;
      double maxError    = SG_SIMPLIFIER_MAX_ERROR;

      if (_options) {
        matlib = _options->getMaterialLib();
        useVBOs = (_options->getPluginStringData("SimGear::USE_VBOS") == "ON");
        SGPropertyNode* propertyNode = _options->getPropertyNode().get();
        simplifyNear = propertyNode->getBoolValue("/sim/rendering/terrain/simplifier/enabled-near", simplifyNear);
        ratio = propertyNode->getDoubleValue("/sim/rendering/terrain/simplifier/ratio", ratio);
        maxLength = propertyNode->getDoubleValue("/sim/rendering/terrain/simplifier/max-length", maxLength);
        maxError = propertyNode->getDoubleValue("/sim/rendering/terrain/simplifier/max-error", maxError);
      }

      // PSADRO TODO : we can do this in terragear 
      // - why not add a bitmask of flags to the btg so we can precompute this?
      // and only do it if it hasn't been done already
      SGVec3d center = tile.get_gbs_center();
      SGGeod geodPos = SGGeod::fromCart(center);
      SGQuatd hlOr = SGQuatd::fromLonLat(geodPos)*SGQuatd::fromEulerDeg(0, 0, 180);

      // Generate a materials cache
      if (matlib) {
          matcache = matlib->generateMatCache(geodPos);
      }
      
      // rotate the tiles so that the bounding boxes get nearly axis aligned.
      // this will help the collision tree's bounding boxes a bit ...
      std::vector<SGVec3d> nodes = tile.get_wgs84_nodes();
      for (unsigned i = 0; i < nodes.size(); ++i) {
        nodes[i] = hlOr.transform(nodes[i]);
      }
      tile.set_wgs84_nodes(nodes);

      SGQuatf hlOrf(hlOr[0], hlOr[1], hlOr[2], hlOr[3]);
      std::vector<SGVec3f> normals = tile.get_normals();
      for (unsigned i = 0; i < normals.size(); ++i) {
        normals[i] = hlOrf.transform(normals[i]);
      }
      tile.set_normals(normals);

      osg::ref_ptr<SGTileGeometryBin> tileGeometryBin = new SGTileGeometryBin;

      if (!tileGeometryBin->insertSurfaceGeometry(tile, matcache)) {
        return NULL;
      }
      
      osg::Node* node = tileGeometryBin->getSurfaceGeometry(matcache, useVBOs);
      if (node && simplifyNear) {
        osgUtil::Simplifier simplifier(ratio, maxError, maxLength);
        node->accept(simplifier);
      }

      return node;
    }

    float min_dist_to_seg_squared( const SGVec3f p, const SGVec3d& a, const SGVec3d& b )
    {
        const float l2 = distSqr(a, b);
        SGVec3d pd = toVec3d( p );
        if (l2 == 0.0) {
            return distSqr(pd, a); // if a == b, just return distance to A
        }
        
        // Consider the line extending the segment, parameterized as a + t (b - a).
        // We find projection of pt onto the line. 
        // It falls where t = [(p-a) . (b-a)] / |b-a|^2
        const float t = dot(pd-a, b-a) / l2;
        
        if (t < 0.0) {
            return distSqr(pd, a);
        } else if (t > 1.0) {
            return distSqr(pd, b);
        } else {
            const SGVec3d proj = a + t * (b-a);
            return distSqr(pd, proj);
        }
    }
    
    float min_dist_from_borders( SGVec3f p, const std::vector<SGBorderContour>& bsegs )
    {
        // calc min dist to each line 
        // calc distance squared to keep this as fast as we can
        // first, we must be able to project the point onto the segment
        std::vector<float> distances;
        for ( unsigned int b=0; b<bsegs.size(); b++ )
        {
            distances.push_back( min_dist_to_seg_squared( p, bsegs[b].start, bsegs[b].end ) );
        }
        
        float min_dist_sq = *std::min_element( distances.begin(), distances.end() );        
        return sqrt( min_dist_sq );
    }
    
    // let's break random objects from randomBuildings
    void computeRandomObjectsAndBuildings(
        std::vector<SGTriangleInfo>& matTris, 
        float building_density,
        bool use_random_objects,
        bool use_random_buildings,
        bool useVBOs,
        SGMatModelBin&     randomModels,
        SGBuildingBinList& randomBuildings )
    {
        unsigned int m;
        
        // Only compute the random objects if we haven't already done so
        if (_tileRandomObjectsComputed) {
            return;
        }
        _tileRandomObjectsComputed = true;
        
        // generate a repeatable random seed
        mt seed;
        mt_init(&seed, unsigned(123));
        
        for ( m=0; m<matTris.size(); m++ ) {
            SGMaterial *mat = matTris[m].getMaterial();
            if (!mat)
                continue;
                        
            osg::Texture2D* object_mask  = mat->get_one_object_mask(matTris[m].getTextureIndex());
            
            int   group_count            = mat->get_object_group_count();
            float building_coverage      = mat->get_building_coverage();
            float cos_zero_density_angle = mat->get_cos_object_zero_density_slope_angle();
            float cos_max_density_angle  = mat->get_cos_object_max_density_slope_angle();
            
            if (building_coverage == 0)
                continue;
            
            SGBuildingBin* bin = NULL;
            
            if (building_coverage > 0) {
                bin = new SGBuildingBin(mat, useVBOs);                
                randomBuildings.push_back(bin);
            }
            
            unsigned num = matTris[m].getNumTriangles();
            int random_dropped = 0;
            int mask_dropped = 0;
            int building_dropped = 0;
            int triangle_dropped = 0;
            
            // get the polygon border segments
//            std::vector<SGBorderContour> borderSegs;
//            matTris[m].getBorderContours( borderSegs );
            
            for (unsigned i = 0; i < num; ++i) {
                std::vector<SGVec3f> triVerts;
                std::vector<SGVec2f> triTCs;
                matTris[m].getTriangle(i, triVerts, triTCs);
                
                SGVec3f vorigin = triVerts[0];
                SGVec3f v0 = triVerts[1] - vorigin;
                SGVec3f v1 = triVerts[2] - vorigin;
                SGVec2f torigin = triTCs[0];
                SGVec2f t0 = triTCs[1] - torigin;
                SGVec2f t1 = triTCs[2] - torigin;
                SGVec3f normal = cross(v0, v1);
                
                // Ensure the slope isn't too steep by checking the
                // cos of the angle between the slope normal and the
                // vertical (conveniently the z-component of the normalized
                // normal) and values passed in.
                float cos = normalize(normal).z();
                float slope_density = 1.0;
                if (cos < cos_zero_density_angle) continue; // Too steep for any objects
                if (cos < cos_max_density_angle) {
                    slope_density =
                    (cos - cos_zero_density_angle) /
                    (cos_max_density_angle - cos_zero_density_angle);
                }
                
                // Containers to hold the random buildings and objects generated
                // for this triangle for collision detection purposes.
                std::vector< std::pair< SGVec3f, float> > triangleObjectsList;
                std::vector< std::pair< SGVec3f, float> > triangleBuildingList;
                
                // Compute the area : todo - we only want to stop if the area of the POLY
                // is too small
                // so we need to know area of each poly....
                float area = 0.5f*length(normal);
                if (area <= SGLimitsf::min())
                    continue;
                
                // Generate any random objects
                if (use_random_objects && (group_count > 0))
                {
                    for (int j = 0; j < group_count; j++)
                    {
                        SGMatModelGroup *object_group =  mat->get_object_group(j);
                        int nObjects = object_group->get_object_count();
                        
                        if (nObjects == 0) continue;
                        
                        // For each of the random models in the group, determine an appropriate
                        // number of random placements and insert them.
                        for (int k = 0; k < nObjects; k++) {
                            SGMatModel * object = object_group->get_object(k);
                            
                            // Determine the number of objecst to place, taking into account
                            // the slope density factor.
                            double n = slope_density * area / object->get_coverage_m2();
                            
                            // Use the zombie door method to determine fractional object placement.
                            n = n + mt_rand(&seed);
                            
                            // place an object each unit of area
                            while ( n > 1.0 ) {
                                n -= 1.0;
                                
                                float a = mt_rand(&seed);
                                float b = mt_rand(&seed);
                                if ( a + b > 1 ) {
                                    a = 1 - a;
                                    b = 1 - b;
                                }
                                
                                SGVec3f randomPoint = vorigin + a*v0 + b*v1;
                                float rotation = static_cast<float>(mt_rand(&seed));
                                
                                // Check that the point is sufficiently far from
                                // the edge of the triangle by measuring the distance
                                // from the three lines that make up the triangle.
                                float spacing = object->get_spacing_m();
                                
                                SGVec3f p = randomPoint - vorigin;
#if 1
                                float edges[] = { 
                                    length(cross(p     , p - v0)) / length(v0),
                                    length(cross(p - v0, p - v1)) / length(v1 - v0),
                                    length(cross(p - v1, p     )) / length(v1)      };
                                    float edge_dist = *std::min_element(edges, edges + 3);
#else
                                    float edge_dist = min_dist_from_borders( randomPoint, borderSegs );
#endif
                                    if (edge_dist < spacing) {
                                        continue;
                                    }
                                    
                                    if (object_mask != NULL) {
                                        SGVec2f texCoord = torigin + a*t0 + b*t1;
                                        
                                        // Check this random point against the object mask
                                        // blue (for buildings) channel.
                                        osg::Image* img = object_mask->getImage();
                                        unsigned int x = (int) (img->s() * texCoord.x()) % img->s();
                                        unsigned int y = (int) (img->t() * texCoord.y()) % img->t();
                                        
                                        if (mt_rand(&seed) > img->getColor(x, y).b()) {
                                            // Failed object mask check
                                            continue;
                                        }
                                        
                                        rotation = img->getColor(x,y).r();
                                    }
                                    
                                    bool close = false;
                                    
                                    // Check it isn't too close to any other random objects in the triangle
                                    std::vector<std::pair<SGVec3f, float> >::iterator l;
                                    for (l = triangleObjectsList.begin(); l != triangleObjectsList.end(); ++l) {
                                        float min_dist2 = (l->second + object->get_spacing_m()) *
                                        (l->second + object->get_spacing_m());
                                        
                                        if (distSqr(l->first, randomPoint) < min_dist2) {
                                            close = true;
                                            continue;
                                        }
                                    }
                                    
                                    if (!close) {
                                        triangleObjectsList.push_back(std::make_pair(randomPoint, object->get_spacing_m()));
                                        randomModels.insert(randomPoint,
                                                            object,
                                                            (int)object->get_randomized_range_m(&seed),
                                                            rotation);
                                    }
                            }
                        }
                    }
                }
                
                // Random objects now generated.  Now generate the random buildings (if any);
                if (use_random_buildings && (building_coverage > 0) && (building_density > 0)) {
                    
                    // Calculate the number of buildings, taking into account building density (which is linear)
                    // and the slope density factor.
                    double num = building_density * building_density * slope_density * area / building_coverage;
                    
                    // For partial units of area, use a zombie door method to
                    // create the proper random chance of an object being created
                    // for this triangle.
                    num = num + mt_rand(&seed);
                    
                    if (num < 1.0f) {
                        continue;
                    }
                    
                    // Cosine of the angle between the two vectors.
                    float cosine = (dot(v0, v1) / (length(v0) * length(v1)));
                    
                    // Determine a grid spacing in each vector such that the correct
                    // coverage will result.
                    float stepv0 = (sqrtf(building_coverage) / building_density) / length(v0) / sqrtf(1 - cosine * cosine);
                    float stepv1 = (sqrtf(building_coverage) / building_density) / length(v1);
                    
                    stepv0 = std::min(stepv0, 1.0f);
                    stepv1 = std::min(stepv1, 1.0f);
                    
                    // Start at a random point. a will be immediately incremented below.
                    float a = -mt_rand(&seed) * stepv0;
                    float b = mt_rand(&seed) * stepv1;
                    
                    // Place an object each unit of area
                    while (num > 1.0) {
                        num -= 1.0;
                        
                        // Set the next location to place a building
                        a += stepv0;
                        
                        if ((a + b) > 1.0f) {
                            // Reached the end of the scan-line on v0. Reset and increment
                            // scan-line on v1
                            a = mt_rand(&seed) * stepv0;
                            b += stepv1;
                        }
                        
                        if (b > 1.0f) {
                            // In a degenerate case of a single point, we might be outside the
                            // scanline.  Note that we need to still ensure that a+b < 1.
                            b = mt_rand(&seed) * stepv1 * (1.0f - a);
                        }
                        
                        if ((a + b) > 1.0f ) {
                            // Truly degenerate case - simply choose a random point guaranteed
                            // to fulfil the constraing of a+b < 1.
                            a = mt_rand(&seed);
                            b = mt_rand(&seed) * (1.0f - a);
                        }
                        
                        SGVec3f randomPoint = vorigin + a*v0 + b*v1;
                        float rotation = mt_rand(&seed);
                        
                        if (object_mask != NULL) {
                            SGVec2f texCoord = torigin + a*t0 + b*t1;
                            osg::Image* img = object_mask->getImage();
                            int x = (int) (img->s() * texCoord.x()) % img->s();
                            int y = (int) (img->t() * texCoord.y()) % img->t();
                            
                            // In some degenerate cases x or y can be < 1, in which case the mod operand fails
                            while (x < 0) x += img->s();
                            while (y < 0) y += img->t();
                            
                            if (mt_rand(&seed) < img->getColor(x, y).b()) {
                                // Object passes mask. Rotation is taken from the red channel
                                rotation = img->getColor(x,y).r();
                            } else {
                                // Fails mask test - try again.
                                mask_dropped++;
                                continue;
                            }
                        }
                        
                        // Check building isn't too close to the triangle edge.
                        float type_roll = mt_rand(&seed);
                        SGBuildingBin::BuildingType buildingtype = bin->getBuildingType(type_roll);
                        float radius = bin->getBuildingMaxRadius(buildingtype);
                        
                        // Determine the actual center of the building, by shifting from the
                        // center of the front face to the true center.
                        osg::Matrix rotationMat = osg::Matrix::rotate(- rotation * M_PI * 2,
                                                                      osg::Vec3f(0.0, 0.0, 1.0));
                        SGVec3f buildingCenter = randomPoint + toSG(osg::Vec3f(-0.5 * bin->getBuildingMaxDepth(buildingtype), 0.0, 0.0) * rotationMat);
                        
                        SGVec3f p = buildingCenter - vorigin;
#if 1
                        float edges[] = { length(cross(p     , p - v0)) / length(v0),
                            length(cross(p - v0, p - v1)) / length(v1 - v0),
                            length(cross(p - v1, p     )) / length(v1)      };
                            float edge_dist = *std::min_element(edges, edges + 3);
#else
                            float edge_dist = min_dist_from_borders(randomPoint, borderSegs);
#endif
                            if (edge_dist < radius) {
                                triangle_dropped++;
                                continue;
                            }
                            
                            // Check building isn't too close to random objects and other buildings.
                            bool close = false;
                            std::vector<std::pair<SGVec3f, float> >::iterator iter;
                            
                            for (iter = triangleBuildingList.begin(); iter != triangleBuildingList.end(); ++iter) {
                                float min_dist = iter->second + radius;
                                if (distSqr(iter->first, buildingCenter) < min_dist * min_dist) {
                                    close = true;
                                    continue;
                                }
                            }
                            
                            if (close) {
                                building_dropped++;
                                continue;
                            }
                            
                            for (iter = triangleObjectsList.begin(); iter != triangleObjectsList.end(); ++iter) {
                                float min_dist = iter->second + radius;
                                if (distSqr(iter->first, buildingCenter) < min_dist * min_dist) {
                                    close = true;
                                    continue;
                                }
                            }
                            
                            if (close) {
                                random_dropped++;
                                continue;
                            }
                            
                            std::pair<SGVec3f, float> pt = std::make_pair(buildingCenter, radius);
                            triangleBuildingList.push_back(pt);
                            bin->insert(randomPoint, rotation, buildingtype);
                    }
                }
                
                triangleObjectsList.clear();
                triangleBuildingList.clear();
            }
            
            SG_LOG(SG_TERRAIN, SG_DEBUG, "Random Buildings: " << ((bin) ? bin->getNumBuildings() : 0));
            SG_LOG(SG_TERRAIN, SG_DEBUG, "  Dropped due to mask: " << mask_dropped);
            SG_LOG(SG_TERRAIN, SG_DEBUG, "  Dropped due to random object: " << random_dropped);
            SG_LOG(SG_TERRAIN, SG_DEBUG, "  Dropped due to other buildings: " << building_dropped);
        }
    }
    
    void computeRandomForest(std::vector<SGTriangleInfo>& matTris, float vegetation_density, SGTreeBinList& randomForest)
    {        
        unsigned int i;
        
        // generate a repeatable random seed
        mt seed;
        mt_init(&seed, unsigned(586));
        
        for ( i=0; i<matTris.size(); i++ ) {
            SGMaterial *mat = matTris[i].getMaterial();
            if (!mat)
                continue;
            
            float wood_coverage = mat->get_wood_coverage();
            if ((wood_coverage <= 0) || (vegetation_density <= 0))
                continue;
            
            // Attributes that don't vary by tree but do vary by material
            bool found = false;
            TreeBin* bin = NULL;
            
            BOOST_FOREACH(bin, randomForest)
            {
                if ((bin->texture           == mat->get_tree_texture()  ) &&
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
                SG_LOG(SG_INPUT, SG_DEBUG, "Tree texture " << bin->texture);
                bin->range   = mat->get_tree_range();
                bin->width   = mat->get_tree_width();
                bin->height  = mat->get_tree_height();
                bin->texture_varieties = mat->get_tree_varieties();
                randomForest.push_back(bin);
            }
            
            std::vector<SGVec3f> randomPoints;
            matTris[i].addRandomTreePoints(wood_coverage,
                                           mat->get_one_object_mask(matTris[i].getTextureIndex()),
                                           vegetation_density,
                                           mat->get_cos_tree_max_density_slope_angle(),
                                           mat->get_cos_tree_zero_density_slope_angle(),
                                           randomPoints);
            
            std::vector<SGVec3f>::iterator k;
            for (k = randomPoints.begin(); k != randomPoints.end(); ++k) {
                bin->insert(*k);
            }
        }
    }
    
    void computeRandomSurfaceLights(std::vector<SGTriangleInfo>& matTris, SGLightBin& randomTileLights )
    {
        unsigned int i;
        
        // Only compute the lights if we haven't already done so.
        // For example, the light data will still exist if the
        // PagedLOD expires.
        if ( _randomSurfaceLightsComputed ) 
        {
            return;
        } 
        _randomSurfaceLightsComputed = true;
        
        // generate a repeatable random seed
        mt seed;
        mt_init(&seed, unsigned(123));

        for ( i=0; i<matTris.size(); i++ ) {
            SGMaterial *mat = matTris[i].getMaterial();
            if (!mat)
                continue;
            
            float coverage = mat->get_light_coverage();
            if (coverage <= 0)
                continue;
            if (coverage < 10000.0) {
                SG_LOG(SG_INPUT, SG_ALERT, "Light coverage is "
                << coverage << ", pushing up to 10000");
                coverage = 10000;
            }
                        
            int texIndex = matTris[i].getTextureIndex();
            
            std::vector<SGVec3f> randomPoints;
            matTris[i].addRandomSurfacePoints(coverage, 3, mat->get_one_object_mask(texIndex), randomPoints);
            std::vector<SGVec3f>::iterator j;
            for (j = randomPoints.begin(); j != randomPoints.end(); ++j) {
                float zombie = mt_rand(&seed);
                // factor = sg_random() ^ 2, range = 0 .. 1 concentrated towards 0
                float factor = mt_rand(&seed);
                factor *= factor;
                
                float bright = 1;
                SGVec4f color;
                if ( zombie > 0.5 ) {
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
                randomTileLights.insert(*j, color);
            }
        }
    }
    
    // Generate all the lighting objects for the tile.
    osg::LOD* generateLightingTileObjects(std::vector<SGTriangleInfo>& matTris, const SGMaterialCache* matcache)
    {
      SGLightBin randomTileLights;
      computeRandomSurfaceLights(matTris, randomTileLights);
      
      GroundLightManager* lightManager = GroundLightManager::instance();
      osg::ref_ptr<osg::Group> lightGroup = new SGOffsetTransform(0.94);
      SGVec3f up(0, 0, 1);

      if (tileLights.getNumLights() > 0 || randomTileLights.getNumLights() > 0) {
        osg::ref_ptr<osg::Group> groundLights0 = new osg::Group;

        groundLights0->setStateSet(lightManager->getGroundLightStateSet());
        groundLights0->setNodeMask(GROUNDLIGHTS0_BIT);

        osg::ref_ptr<EffectGeode> geode = new EffectGeode;        
        osg::ref_ptr<Effect> lightEffect = getLightEffect(24, osg::Vec3(1, 0.001, 0.00001), 1, 8, false, _options);
        
        geode->setEffect(lightEffect);                
        geode->addDrawable(SGLightFactory::getLights(tileLights));
        geode->addDrawable(SGLightFactory::getLights(randomTileLights, 4, -0.3f));
        groundLights0->addChild(geode);
        lightGroup->addChild(groundLights0);
      }

      if (randomTileLights.getNumLights() > 0) {
        osg::ref_ptr<osg::Group> groundLights1 = new osg::Group;
        groundLights1->setStateSet(lightManager->getGroundLightStateSet());
        groundLights1->setNodeMask(GROUNDLIGHTS1_BIT);
        
        osg::ref_ptr<osg::Group> groundLights2 = new osg::Group;
        groundLights2->setStateSet(lightManager->getGroundLightStateSet());
        groundLights2->setNodeMask(GROUNDLIGHTS2_BIT);

        osg::ref_ptr<EffectGeode> geode1 = new EffectGeode;
        
        osg::ref_ptr<Effect> lightEffect = getLightEffect(24, osg::Vec3(1, 0.001, 0.00001), 1, 8, false, _options);        
        geode1->setEffect(lightEffect);        
        geode1->addDrawable(SGLightFactory::getLights(randomTileLights, 2, -0.15f));
        groundLights1->addChild(geode1);
        lightGroup->addChild(groundLights1);
        
        osg::ref_ptr<EffectGeode> geode2 = new EffectGeode;
        
        geode2->setEffect(lightEffect);
        geode2->addDrawable(SGLightFactory::getLights(randomTileLights));
        groundLights2->addChild(geode2);
        lightGroup->addChild(groundLights2);
      }

      if (vasiLights.empty()) {
        EffectGeode* vasiGeode = new EffectGeode;        
        Effect* vasiEffect = getLightEffect(24, osg::Vec3(1, 0.0001, 0.000001), 1, 24, true, _options);
        vasiGeode->setEffect(vasiEffect);
        SGVec4f red(1, 0, 0, 1);
        SGMaterial* mat = 0;
        if (matcache)
          mat = matcache->find("RWY_RED_LIGHTS");
        if (mat) {
          red = mat->get_light_color();
        }
        
        SGVec4f white(1, 1, 1, 1);
        mat = 0;
        if (matcache)
          mat = matcache->find("RWY_WHITE_LIGHTS");
        if (mat) {
          white = mat->get_light_color();
        }
        SGDirectionalLightListBin::const_iterator i;
        for (i = vasiLights.begin();
             i != vasiLights.end(); ++i) {
            osg::Drawable* vasiDraw = SGLightFactory::getVasi(up, *i, red, white);
            vasiGeode->addDrawable( vasiDraw );
        }
        osg::StateSet* ss = lightManager->getRunwayLightStateSet();
        vasiGeode->setStateSet( ss );
        lightGroup->addChild(vasiGeode);
      }

      Effect* runwayEffect = 0;
      if (runwayLights.getNumLights() > 0
          || !rabitLights.empty()
          || !reilLights.empty()
          || !odalLights.empty()
          || taxiLights.getNumLights() > 0) {
          
          runwayEffect = getLightEffect(16, osg::Vec3(1, 0.001, 0.0002), 1, 16, true, _options);
      }
      
      if (runwayLights.getNumLights() > 0
          || !rabitLights.empty()
          || !reilLights.empty()
          || !odalLights.empty()
          || !holdshortLights.empty()
          || !guardLights.empty()) {
        osg::Group* rwyLights = new osg::Group;

        osg::StateSet* ss = lightManager->getRunwayLightStateSet();      
        rwyLights->setStateSet(ss);      
        rwyLights->setNodeMask(RUNWAYLIGHTS_BIT);
        
        if (runwayLights.getNumLights() != 0) {
          EffectGeode* geode = new EffectGeode;
          geode->setEffect(runwayEffect);
          
          osg::Drawable* rldraw = SGLightFactory::getLights(runwayLights);
          geode->addDrawable( rldraw );
          
          rwyLights->addChild(geode);
        }
        SGDirectionalLightListBin::const_iterator i;
        for (i = rabitLights.begin();
             i != rabitLights.end(); ++i) {
            osg::Node* seqNode = SGLightFactory::getSequenced(*i, _options);
            rwyLights->addChild( seqNode );
        }
        for (i = reilLights.begin();
             i != reilLights.end(); ++i) {
            osg::Node* seqNode = SGLightFactory::getSequenced(*i, _options);
            rwyLights->addChild(seqNode);
        }
        for (i = holdshortLights.begin();
             i != holdshortLights.end(); ++i) {
            osg::Node* seqNode = SGLightFactory::getHoldShort(*i, _options);
            rwyLights->addChild(seqNode);
        }
        for (i = guardLights.begin();
             i != guardLights.end(); ++i) {
            osg::Node* seqNode = SGLightFactory::getGuard(*i, _options);
            rwyLights->addChild(seqNode);
        }
        SGLightListBin::const_iterator j;
        for (j = odalLights.begin();
             j != odalLights.end(); ++j) {
            osg::Node* seqNode = SGLightFactory::getOdal(*j, _options);
            rwyLights->addChild(seqNode);
        }
        lightGroup->addChild(rwyLights);
      }

      if (taxiLights.getNumLights() > 0) {
        osg::Group* taxiLightsGroup = new osg::Group;
        taxiLightsGroup->setStateSet(lightManager->getTaxiLightStateSet());
        taxiLightsGroup->setNodeMask(RUNWAYLIGHTS_BIT);
        EffectGeode* geode = new EffectGeode;
        geode->setEffect(runwayEffect);
        geode->addDrawable(SGLightFactory::getLights(taxiLights));
        taxiLightsGroup->addChild(geode);
        lightGroup->addChild(taxiLightsGroup);
      }

      osg::LOD* lightLOD = NULL;

      if (lightGroup->getNumChildren() > 0) {
        lightLOD = new osg::LOD;
        lightLOD->addChild(lightGroup.get(), 0, 60000);
        // VASI is always on, so doesn't use light bits.
        lightLOD->setNodeMask(LIGHTS_BITS | MODEL_BIT | PERMANENTLIGHT_BIT);
      }

      return lightLOD;
    }

    // Generate all the random forest, objects and buildings for the tile
    osg::LOD* generateRandomTileObjects(std::vector<SGTriangleInfo>& matTris, const SGMaterialCache* matcache)
    {
      SGMaterialLibPtr matlib;
      bool use_random_objects = false;
      bool use_random_vegetation = false;
      bool use_random_buildings = false;
      float vegetation_density = 1.0f;
      float building_density = 1.0f;
      bool useVBOs = false;
      
      osg::ref_ptr<osg::Group> randomObjects;
      osg::ref_ptr<osg::Group> forestNode;
      osg::ref_ptr<osg::Group> buildingNode;

      if (_options) {
        matlib = _options->getMaterialLib();
        SGPropertyNode* propertyNode = _options->getPropertyNode().get();
        if (propertyNode) {
            use_random_objects
                = propertyNode->getBoolValue("/sim/rendering/random-objects",
                                             use_random_objects);
            use_random_vegetation
                = propertyNode->getBoolValue("/sim/rendering/random-vegetation",
                                             use_random_vegetation);
            vegetation_density
                = propertyNode->getFloatValue("/sim/rendering/vegetation-density",
                                              vegetation_density);
            use_random_buildings
                = propertyNode->getBoolValue("/sim/rendering/random-buildings",
                                             use_random_buildings);
            building_density
                = propertyNode->getFloatValue("/sim/rendering/building-density",
                                              building_density);
        }
        
        useVBOs = (_options->getPluginStringData("SimGear::USE_VBOS") == "ON");
      }

      SGMatModelBin     randomModels;
      
      SGBuildingBinList randomBuildings;
      
      if (matlib && (use_random_objects || use_random_buildings)) {
          computeRandomObjectsAndBuildings( matTris, 
                                            building_density,
                                            use_random_objects,
                                            use_random_buildings,
                                            useVBOs,
                                            randomModels,
                                            randomBuildings
                                          );
      }

      if (randomModels.getNumModels() > 0) {
        // Generate a repeatable random seed
        mt seed;
        mt_init(&seed, unsigned(123));

        std::vector<ModelLOD> models;
        for (unsigned int i = 0; i < randomModels.getNumModels(); i++) {
          SGMatModelBin::MatModel obj = randomModels.getMatModel(i);

          SGPropertyNode* root = _options->getPropertyNode()->getRootNode();
          osg::Node* node = obj.model->get_random_model(root, &seed);

          // Create a matrix to place the object in the correct
          // location, and then apply the rotation matrix created
          // above, with an additional random (or taken from
          // the object mask) heading rotation if appropriate.
          osg::Matrix transformMat;
          transformMat = osg::Matrix::translate(toOsg(obj.position));
          if (obj.model->get_heading_type() == SGMatModel::HEADING_RANDOM) {
            // Rotate the object around the z axis.
            double hdg = mt_rand(&seed) * M_PI * 2;
            transformMat.preMult(osg::Matrix::rotate(hdg,
                                                     osg::Vec3d(0.0, 0.0, 1.0)));
          }

          if (obj.model->get_heading_type() == SGMatModel::HEADING_MASK) {
            // Rotate the object around the z axis.
            double hdg =  - obj.rotation * M_PI * 2;
            transformMat.preMult(osg::Matrix::rotate(hdg,
                                                     osg::Vec3d(0.0, 0.0, 1.0)));
          }

          osg::MatrixTransform* position =
            new osg::MatrixTransform(transformMat);
          position->setName("positionRandomModel");
          position->addChild(node);
          models.push_back(ModelLOD(position, obj.lod));
        }
        RandomObjectsQuadtree quadtree((GetModelLODCoord()), (AddModelLOD()));
        quadtree.buildQuadTree(models.begin(), models.end());
        randomObjects = quadtree.getRoot();
        randomObjects->setName("Random objects");
      }

      if (!randomBuildings.empty()) {
        buildingNode = createRandomBuildings(randomBuildings, osg::Matrix::identity(), _options);
        buildingNode->setName("Random buildings");
        randomBuildings.clear();
      }

      if (use_random_vegetation && matlib) {
        // Now add some random forest.
        SGTreeBinList randomForest;
        computeRandomForest(matTris, vegetation_density, randomForest);

        if (!randomForest.empty()) {
          forestNode = createForest(randomForest, osg::Matrix::identity(),_options);
          forestNode->setName("Random trees");
        }
      }

      osg::LOD* objectLOD = NULL;

      if (randomObjects.valid() ||  forestNode.valid() || buildingNode.valid()) {
        objectLOD = new osg::LOD;

        if (randomObjects.valid()) objectLOD->addChild(randomObjects.get(), 0, 20000);
        if (forestNode.valid())  objectLOD->addChild(forestNode.get(), 0, 20000);
        if (buildingNode.valid()) objectLOD->addChild(buildingNode.get(), 0, 20000);

        unsigned nodeMask = SG_NODEMASK_CASTSHADOW_BIT | SG_NODEMASK_RECEIVESHADOW_BIT | SG_NODEMASK_TERRAIN_BIT;
        objectLOD->setNodeMask(nodeMask);
      }

      return objectLOD;
    }

    /// The original options to use for this bunch of models
    osg::ref_ptr<SGReaderWriterOptions>     _options;
    string                                  _path;
    bool                                    _loadterrain;
    osg::ref_ptr<osg::Node>                 _rootNode;
    SGVec3d                                 _gbs_center;
    bool                                    _randomSurfaceLightsComputed;
    bool                                    _tileRandomObjectsComputed;
    
    // most of these are just point and color arrays - extracted from the 
    // .BTG PointGeometry at tile load time.
    // It shouldn't be too much to keep this in memory even if we don't use it.
    SGLightBin                              tileLights;
    SGDirectionalLightBin                   runwayLights;
    SGDirectionalLightBin                   taxiLights;
    SGDirectionalLightListBin               vasiLights;
    SGDirectionalLightListBin               rabitLights;
    SGLightListBin                          odalLights;
    SGDirectionalLightListBin               holdshortLights;
    SGDirectionalLightListBin               guardLights;
    SGDirectionalLightListBin               reilLights;    
};