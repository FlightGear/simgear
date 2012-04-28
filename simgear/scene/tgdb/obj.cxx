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

#include "obj.hxx"

#include <simgear/compiler.h>

#include <osg/Fog>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Group>
#include <osg/LOD>
#include <osg/MatrixTransform>
#include <osg/Point>
#include <osg/StateSet>
#include <osg/Switch>

#include <boost/foreach.hpp>

#include <algorithm>

#include <simgear/debug/logstream.hxx>
#include <simgear/io/sg_binobj.hxx>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/math/sg_random.h>
#include <simgear/math/SGMisc.hxx>
#include <simgear/scene/material/Effect.hxx>
#include <simgear/scene/material/EffectGeode.hxx>
#include <simgear/scene/material/mat.hxx>
#include <simgear/scene/material/matlib.hxx>
#include <simgear/scene/model/SGOffsetTransform.hxx>
#include <simgear/scene/util/SGUpdateVisitor.hxx>
#include <simgear/scene/util/SGNodeMasks.hxx>
#include <simgear/scene/util/QuadTreeBuilder.hxx>

#include "SGTexturedTriangleBin.hxx"
#include "SGLightBin.hxx"
#include "SGModelBin.hxx"
#include "SGBuildingBin.hxx"
#include "TreeBin.hxx"
#include "SGDirectionalLightBin.hxx"
#include "GroundLightManager.hxx"


#include "pt_lights.hxx"

using namespace simgear;

typedef std::map<std::string,SGTexturedTriangleBin> SGMaterialTriangleMap;
typedef std::list<SGLightBin> SGLightListBin;
typedef std::list<SGDirectionalLightBin> SGDirectionalLightListBin;

struct SGTileGeometryBin {
  SGMaterialTriangleMap materialTriangleMap;
  SGLightBin tileLights;
  SGLightBin randomTileLights;
  SGTreeBinList randomForest;
  SGDirectionalLightBin runwayLights;
  SGDirectionalLightBin taxiLights;
  SGDirectionalLightListBin vasiLights;
  SGDirectionalLightListBin rabitLights;
  SGLightListBin odalLights;
  SGDirectionalLightListBin holdshortLights;
  SGDirectionalLightListBin reilLights;
  SGMatModelBin randomModels;
  SGBuildingBinList randomBuildings;

  static SGVec4f
  getMaterialLightColor(const SGMaterial* material)
  {
    if (!material)
      return SGVec4f(1, 1, 1, 0.8);
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

  bool
  insertPtGeometry(const SGBinObject& obj, SGMaterialLib* matlib)
  {
    if (obj.get_pts_v().size() != obj.get_pts_n().size()) {
      SG_LOG(SG_TERRAIN, SG_ALERT,
             "Group list sizes for points do not match!");
      return false;
    }

    for (unsigned grp = 0; grp < obj.get_pts_v().size(); ++grp) {
      std::string materialName = obj.get_pt_materials()[grp];
      SGMaterial* material = 0;
      if (matlib)
          material = matlib->find(materialName);
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


  static SGVec2f
  getTexCoord(const std::vector<SGVec2f>& texCoords, const int_list& tc,
              const SGVec2f& tcScale, unsigned i)
  {
    if (tc.empty())
      return tcScale;
    else if (tc.size() == 1)
      return mult(texCoords[tc[0]], tcScale);
    else
      return mult(texCoords[tc[i]], tcScale);
  }

  static void
  addTriangleGeometry(SGTexturedTriangleBin& triangles,
                      const std::vector<SGVec3d>& vertices,
                      const std::vector<SGVec3f>& normals,
                      const std::vector<SGVec2f>& texCoords,
                      const int_list& tris_v,
                      const int_list& tris_n,
                      const int_list& tris_tc,
                      const SGVec2f& tcScale)
  {
    if (tris_v.size() != tris_n.size()) {
      // If the normal indices do not match, they should be inmplicitly
      // the same than the vertex indices. So just call ourselves again
      // with the matching index vector.
      addTriangleGeometry(triangles, vertices, normals, texCoords,
                          tris_v, tris_v, tris_tc, tcScale);
      return;
    }

    for (unsigned i = 2; i < tris_v.size(); i += 3) {
      SGVertNormTex v0;
      v0.vertex = toVec3f(vertices[tris_v[i-2]]);
      v0.normal = normals[tris_n[i-2]];
      v0.texCoord = getTexCoord(texCoords, tris_tc, tcScale, i-2);
      SGVertNormTex v1;
      v1.vertex = toVec3f(vertices[tris_v[i-1]]);
      v1.normal = normals[tris_n[i-1]];
      v1.texCoord = getTexCoord(texCoords, tris_tc, tcScale, i-1);
      SGVertNormTex v2;
      v2.vertex = toVec3f(vertices[tris_v[i]]);
      v2.normal = normals[tris_n[i]];
      v2.texCoord = getTexCoord(texCoords, tris_tc, tcScale, i);
      triangles.insert(v0, v1, v2);
    }
  }

  static void
  addStripGeometry(SGTexturedTriangleBin& triangles,
                   const std::vector<SGVec3d>& vertices,
                   const std::vector<SGVec3f>& normals,
                   const std::vector<SGVec2f>& texCoords,
                   const int_list& strips_v,
                   const int_list& strips_n,
                   const int_list& strips_tc,
                   const SGVec2f& tcScale)
  {
    if (strips_v.size() != strips_n.size()) {
      // If the normal indices do not match, they should be inmplicitly
      // the same than the vertex indices. So just call ourselves again
      // with the matching index vector.
      addStripGeometry(triangles, vertices, normals, texCoords,
                       strips_v, strips_v, strips_tc, tcScale);
      return;
    }

    for (unsigned i = 2; i < strips_v.size(); ++i) {
      SGVertNormTex v0;
      v0.vertex = toVec3f(vertices[strips_v[i-2]]);
      v0.normal = normals[strips_n[i-2]];
      v0.texCoord = getTexCoord(texCoords, strips_tc, tcScale, i-2);
      SGVertNormTex v1;
      v1.vertex = toVec3f(vertices[strips_v[i-1]]);
      v1.normal = normals[strips_n[i-1]];
      v1.texCoord = getTexCoord(texCoords, strips_tc, tcScale, i-1);
      SGVertNormTex v2;
      v2.vertex = toVec3f(vertices[strips_v[i]]);
      v2.normal = normals[strips_n[i]];
      v2.texCoord = getTexCoord(texCoords, strips_tc, tcScale, i);
      if (i%2)
        triangles.insert(v1, v0, v2);
      else
        triangles.insert(v0, v1, v2);
    }
  }
  
  static void
  addFanGeometry(SGTexturedTriangleBin& triangles,
                 const std::vector<SGVec3d>& vertices,
                 const std::vector<SGVec3f>& normals,
                 const std::vector<SGVec2f>& texCoords,
                 const int_list& fans_v,
                 const int_list& fans_n,
                 const int_list& fans_tc,
                 const SGVec2f& tcScale)
  {
    if (fans_v.size() != fans_n.size()) {
      // If the normal indices do not match, they should be implicitly
      // the same than the vertex indices. So just call ourselves again
      // with the matching index vector.
      addFanGeometry(triangles, vertices, normals, texCoords,
                     fans_v, fans_v, fans_tc, tcScale);
      return;
    }

    SGVertNormTex v0;
    v0.vertex = toVec3f(vertices[fans_v[0]]);
    v0.normal = normals[fans_n[0]];
    v0.texCoord = getTexCoord(texCoords, fans_tc, tcScale, 0);
    SGVertNormTex v1;
    v1.vertex = toVec3f(vertices[fans_v[1]]);
    v1.normal = normals[fans_n[1]];
    v1.texCoord = getTexCoord(texCoords, fans_tc, tcScale, 1);
    for (unsigned i = 2; i < fans_v.size(); ++i) {
      SGVertNormTex v2;
      v2.vertex = toVec3f(vertices[fans_v[i]]);
      v2.normal = normals[fans_n[i]];
      v2.texCoord = getTexCoord(texCoords, fans_tc, tcScale, i);
      triangles.insert(v0, v1, v2);
      v1 = v2;
    }
  }

  SGVec2f getTexCoordScale(const std::string& name, SGMaterialLib* matlib)
  {
    if (!matlib)
      return SGVec2f(1, 1);
    SGMaterial* material = matlib->find(name);
    if (!material)
      return SGVec2f(1, 1);

    return material->get_tex_coord_scale();
  }

  bool
  insertSurfaceGeometry(const SGBinObject& obj, SGMaterialLib* matlib)
  {
    if (obj.get_tris_n().size() < obj.get_tris_v().size() ||
        obj.get_tris_tc().size() < obj.get_tris_v().size()) {
      SG_LOG(SG_TERRAIN, SG_ALERT,
             "Group list sizes for triangles do not match!");
      return false;
    }

    for (unsigned grp = 0; grp < obj.get_tris_v().size(); ++grp) {
      std::string materialName = obj.get_tri_materials()[grp];
      SGVec2f tcScale = getTexCoordScale(materialName, matlib);
      addTriangleGeometry(materialTriangleMap[materialName],
                          obj.get_wgs84_nodes(), obj.get_normals(),
                          obj.get_texcoords(), obj.get_tris_v()[grp],
                          obj.get_tris_n()[grp], obj.get_tris_tc()[grp],
                          tcScale);
    }

    if (obj.get_strips_n().size() < obj.get_strips_v().size() ||
        obj.get_strips_tc().size() < obj.get_strips_v().size()) {
      SG_LOG(SG_TERRAIN, SG_ALERT,
             "Group list sizes for strips do not match!");
      return false;
    }
    for (unsigned grp = 0; grp < obj.get_strips_v().size(); ++grp) {
      std::string materialName = obj.get_strip_materials()[grp];
      SGVec2f tcScale = getTexCoordScale(materialName, matlib);
      addStripGeometry(materialTriangleMap[materialName],
                       obj.get_wgs84_nodes(), obj.get_normals(),
                       obj.get_texcoords(), obj.get_strips_v()[grp],
                       obj.get_strips_n()[grp], obj.get_strips_tc()[grp],
                       tcScale);
    }

    if (obj.get_fans_n().size() < obj.get_fans_v().size() ||
        obj.get_fans_tc().size() < obj.get_fans_v().size()) {
      SG_LOG(SG_TERRAIN, SG_ALERT,
             "Group list sizes for fans do not match!");
      return false;
    }
    for (unsigned grp = 0; grp < obj.get_fans_v().size(); ++grp) {
      std::string materialName = obj.get_fan_materials()[grp];
      SGVec2f tcScale = getTexCoordScale(materialName, matlib);
      addFanGeometry(materialTriangleMap[materialName],
                     obj.get_wgs84_nodes(), obj.get_normals(),
                     obj.get_texcoords(), obj.get_fans_v()[grp],
                     obj.get_fans_n()[grp], obj.get_fans_tc()[grp],
                     tcScale);
    }
    return true;
  }

  osg::Node* getSurfaceGeometry(SGMaterialLib* matlib) const
  {
    if (materialTriangleMap.empty())
      return 0;

    EffectGeode* eg = 0;
    osg::Group* group = (materialTriangleMap.size() > 1 ? new osg::Group : 0);
    //osg::Geode* geode = new osg::Geode;
    SGMaterialTriangleMap::const_iterator i;
    for (i = materialTriangleMap.begin(); i != materialTriangleMap.end(); ++i) {
      osg::Geometry* geometry = i->second.buildGeometry();
      SGMaterial *mat = 0;
      if (matlib)
        mat = matlib->find(i->first);
      eg = new EffectGeode;
      if (mat)
        eg->setEffect(mat->get_effect(i->second));
      eg->addDrawable(geometry);
      eg->runGenerators(geometry);  // Generate extra data needed by effect
      if (group)
        group->addChild(eg);
    }
    if (group)
        return group;
    else
        return eg;
  }

  void computeRandomSurfaceLights(SGMaterialLib* matlib)
  {
    SGMaterialTriangleMap::iterator i;
        
    // generate a repeatable random seed
    mt seed;
    mt_init(&seed, unsigned(123));
    
    for (i = materialTriangleMap.begin(); i != materialTriangleMap.end(); ++i) {
      SGMaterial *mat = matlib->find(i->first);
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
      
      std::vector<SGVec3f> randomPoints;
      i->second.addRandomSurfacePoints(coverage, 3, mat->get_object_mask(i->second), randomPoints);
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
  
  void computeRandomBuildings(SGMaterialLib* matlib, float building_density)
  {
    SGMaterialTriangleMap::iterator i;
        
    // generate a repeatable random seed
    mt seed;
    mt_init(&seed, unsigned(123));
    
    for (i = materialTriangleMap.begin(); i != materialTriangleMap.end(); ++i) {
      SGMaterial *mat = matlib->find(i->first);
      SGTexturedTriangleBin triangleBin = i->second;
      
      if (!mat)
        continue;

      osg::Texture2D* object_mask = mat->get_object_mask(triangleBin);

      float coverage = mat->get_building_coverage();
      
      // Minimum spacing needs to include the maximum footprint of a building.
      // As the 0,0,0 point is the center of the front of the building, we need
      // to consider the full depth, but only half the possible width.          
      float min_spacing = mat->get_building_spacing();

      if (coverage <= 0)
        continue;        
        
      bool found = false;
      SGBuildingBin* bin = NULL;
      
      BOOST_FOREACH(bin, randomBuildings)
      {
        if (bin->texture == mat->get_building_texture()) {
            found = true;
            break;
        }
      }
      
      if (!found) {
        bin = new SGBuildingBin();
        bin->texture = mat->get_building_texture();
        SG_LOG(SG_INPUT, SG_DEBUG, "Building texture " << bin->texture);
        randomBuildings.push_back(bin);
      }       
      
      std::vector<std::pair<SGVec3f, float> > randomPoints;
      
      unsigned num = i->second.getNumTriangles();
      int triangle_dropped = 0;
      int building_dropped = 0;
      int random_dropped = 0;
      int mask_dropped = 0;

      for (unsigned i = 0; i < num; ++i) {
        SGBuildingBin::BuildingList triangle_buildings;
        SGTexturedTriangleBin::triangle_ref triangleRef = triangleBin.getTriangleRef(i);
        
        SGVec3f vorigin = triangleBin.getVertex(triangleRef[0]).vertex;
        SGVec3f v0 = triangleBin.getVertex(triangleRef[1]).vertex - vorigin;
        SGVec3f v1 = triangleBin.getVertex(triangleRef[2]).vertex - vorigin;
        SGVec2f torigin = triangleBin.getVertex(triangleRef[0]).texCoord;
        SGVec2f t0 = triangleBin.getVertex(triangleRef[1]).texCoord - torigin;
        SGVec2f t1 = triangleBin.getVertex(triangleRef[2]).texCoord - torigin;
        SGVec3f normal = cross(v0, v1);
        
        // Compute the area
        float area = 0.5f*length(normal);
        if (area <= SGLimitsf::min())
          continue;

        // for partial units of area, use a zombie door method to
        // create the proper random chance of an object being created
        // for this triangle.
        double num = area / coverage + mt_rand(&seed);
        if (num < 1.0f) {
          continue;          
        }
                
        // Apply density, which is linear, while we're dealing in areas
        num = num * building_density * building_density;
        
        // Cosine of the angle between the two vectors.
        float cosine = (dot(v0, v1) / (length(v0) * length(v1)));

        // Determine a grid spacing in each vector such that the correct
        // coverage will result.
        float stepv0 = (sqrtf(coverage) / building_density) / length(v0) / sqrtf(1 - cosine * cosine);
        float stepv1 = (sqrtf(coverage) / building_density) / length(v1);
        
        stepv0 = std::min(stepv0, 1.0f);
        stepv1 = std::min(stepv1, 1.0f);
        
        // Start at a random point. a will be immediately incremented below.
        float a = -mt_rand(&seed) * stepv0;  
        float b = mt_rand(&seed) * stepv1;

        // Place an object each unit of area
        while ( num > 1.0 ) {

          // Set the next location to place a building          
          a += stepv0;
          
          if ( a + b > 1.0f ) {
            // Reached the end of the scan-line on v0. Reset and increment
            // scan-line on v1
            a = mt_rand(&seed) * stepv0;
            b += stepv1;
          }
          
          if (b > 1.0f) {
            // In a degenerate case of a single point, we might be outside the 
            // scanline.
            b = mt_rand(&seed) * stepv1;
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
              num -= 1.0;
              continue;
            }  
          }
          
          // Now create the building, so we have an idea of its footprint
          // and therefore appropriate spacing.
          SGBuildingBin::BuildingType buildingtype;
          float width;
          float depth;
          int floors;
          float height;
          bool pitched;
                                  
          // Determine the building type, and hence dimensions.
          float type = mt_rand(&seed);
          
          if (type < mat->get_building_small_fraction()) {
            // Small building
            buildingtype = SGBuildingBin::SMALL;
            width = mat->get_building_small_min_width() + mt_rand(&seed) * mt_rand(&seed) * (mat->get_building_small_max_width() - mat->get_building_small_min_width());
            depth = mat->get_building_small_min_depth() + mt_rand(&seed) * mt_rand(&seed) * (mat->get_building_small_max_depth() - mat->get_building_small_min_depth());
            floors = SGMisc<double>::round(mat->get_building_small_min_floors() + mt_rand(&seed) * (mat->get_building_small_max_floors() - mat->get_building_small_min_floors()));
            height = floors * (2.8 + mt_rand(&seed));
            
            if (depth > width) { depth = width; }
            
            pitched = (mt_rand(&seed) < mat->get_building_small_pitch());
          } else if (type < (mat->get_building_small_fraction() + mat->get_building_medium_fraction())) {
            buildingtype = SGBuildingBin::MEDIUM;
            width = mat->get_building_medium_min_width() + mt_rand(&seed) * mt_rand(&seed) * (mat->get_building_medium_max_width() - mat->get_building_medium_min_width());
            depth = mat->get_building_medium_min_depth() + mt_rand(&seed) * mt_rand(&seed) * (mat->get_building_medium_max_depth() - mat->get_building_medium_min_depth());
            floors = SGMisc<double>::round(mat->get_building_medium_min_floors() + mt_rand(&seed) * (mat->get_building_medium_max_floors() - mat->get_building_medium_min_floors()));
            height = floors * (2.8 + mt_rand(&seed));
            pitched = (mt_rand(&seed) < mat->get_building_medium_pitch());         
          } else {
            buildingtype = SGBuildingBin::LARGE;
            width = mat->get_building_large_min_width() + mt_rand(&seed) * (mat->get_building_large_max_width() - mat->get_building_large_min_width());
            depth = mat->get_building_large_min_depth() + mt_rand(&seed) * (mat->get_building_large_max_depth() - mat->get_building_large_min_depth());
            floors = SGMisc<double>::round(mat->get_building_large_min_floors() + mt_rand(&seed) * (mat->get_building_large_max_floors() - mat->get_building_large_min_floors())); 
            height = floors * (2.8 + mt_rand(&seed));
            pitched = (mt_rand(&seed) < mat->get_building_large_pitch());                   
          }
          
          // Determine an appropriate minimum spacing for the object.  Note that the
          // origin of the building model is the center of the front face, hence we
          // consider the full depth.  We choose _not_ to use the diagonal distance
          // to one of the rear corners, as we assume that terrain masking will
          // make the buildings place in some sort of grid.
          float radius = std::max(depth, 0.5f*width);

          // Check that the point is sufficiently far from
          // the edge of the triangle by measuring the distance
          // from the three lines that make up the triangle.        
          SGVec3f p = randomPoint - vorigin;
          
          if (((length(cross(p     , p - v0)) / length(v0))      < radius) ||
              ((length(cross(p - v0, p - v1)) / length(v1 - v0)) < radius) ||
              ((length(cross(p - v1, p     )) / length(v1))      < radius)   )
          {
            triangle_dropped++;
            num -= 1.0;
            continue;
          }

          // Check against the generic random objects.  TODO - make this more efficient by
          // masking ahead of time objects outside of the triangle.
          bool too_close = false;
          for (unsigned int i = 0; i < randomModels.getNumModels(); ++i) {
            float min_dist = randomModels.getMatModel(i).model->get_spacing_m() + radius + min_spacing;
            min_dist = min_dist * min_dist;
            
            if (distSqr(randomModels.getMatModel(i).position, randomPoint) < min_dist) {
              too_close = true;
              random_dropped++;
              continue;
            }          
          }
          
          if (too_close) {
            // Too close to a random model - drop and try again
            num -= 1.0;
            continue;            
          }
          
          SGBuildingBin::BuildingList::iterator l;       
          
          // Check that the building is sufficiently far from any other building within the triangle.
          for (l = triangle_buildings.begin(); l != triangle_buildings.end(); ++l) {
            
            float min_dist = l->radius + radius + min_spacing;
            min_dist = min_dist * min_dist;
            
            if (distSqr(randomPoint, l->position) < min_dist) {
              building_dropped++;
              too_close = true;
              continue;
            }
          }
          
          if (too_close) {
            // Too close to another building - drop and try again
            num -= 1.0;
            continue;            
          }
          
          // If we've passed all of the above tests we have a valid
          // building, so create it!          
          SGBuildingBin::Building building = 
                    SGBuildingBin::Building(buildingtype, 
                                            randomPoint, 
                                            width, 
                                            depth, 
                                            height, 
                                            floors,
                                            rotation,
                                            pitched);                                                            
          triangle_buildings.push_back(building);

          num -= 1.0;
        }        
        
        // Add the buildings from this triangle to the overall list.
        SGBuildingBin::BuildingList::iterator l;  

        for (l = triangle_buildings.begin(); l != triangle_buildings.end(); ++l) {
          bin->insert(*l);
        }
      }
      
      SG_LOG(SG_INPUT, SG_DEBUG, "Random Buildings: " << bin->getNumBuildings());
      SG_LOG(SG_INPUT, SG_DEBUG, "  Dropped due to mask: " << mask_dropped);
      SG_LOG(SG_INPUT, SG_DEBUG, "  Dropped due to triangle edge: " << triangle_dropped);
      SG_LOG(SG_INPUT, SG_DEBUG, "  Dropped due to random object: " << random_dropped);
      SG_LOG(SG_INPUT, SG_DEBUG, "  Dropped due to other building: " << building_dropped);
    }
  }
  
  void computeRandomForest(SGMaterialLib* matlib, float vegetation_density)
  {
    SGMaterialTriangleMap::iterator i;

    // generate a repeatable random seed
    mt seed;

    mt_init(&seed, unsigned(586));

    for (i = materialTriangleMap.begin(); i != materialTriangleMap.end(); ++i) {
      SGMaterial *mat = matlib->find(i->first);
      if (!mat)
        continue;

      float wood_coverage = mat->get_wood_coverage();
      if (wood_coverage <= 0)
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
      i->second.addRandomTreePoints(wood_coverage,
                                    mat->get_object_mask(i->second),
                                    vegetation_density,
                                    randomPoints);
      
      std::vector<SGVec3f>::iterator k;
      for (k = randomPoints.begin(); k != randomPoints.end(); ++k) {
        bin->insert(*k);
      }
    }
  }

  void computeRandomObjects(SGMaterialLib* matlib)
  {
    SGMaterialTriangleMap::iterator i;

    // generate a repeatable random seed
    mt seed;
    mt_init(&seed, unsigned(123));

    for (i = materialTriangleMap.begin(); i != materialTriangleMap.end(); ++i) {
      SGMaterial *mat = matlib->find(i->first);
      if (!mat)
        continue;

      int group_count = mat->get_object_group_count();

      if (group_count > 0)
      {
        for (int j = 0; j < group_count; j++)
        {
          SGMatModelGroup *object_group =  mat->get_object_group(j);
          int nObjects = object_group->get_object_count();

          if (nObjects > 0)
          {
            // For each of the random models in the group, determine an appropriate
            // number of random placements and insert them.
            for (int k = 0; k < nObjects; k++) {
              SGMatModel * object = object_group->get_object(k);

              std::vector<std::pair<SGVec3f, float> > randomPoints;

              i->second.addRandomPoints(object->get_coverage_m2(), 
                                        object->get_spacing_m(),
                                        mat->get_object_mask(i->second), 
                                        randomPoints);
                                        
              std::vector<std::pair<SGVec3f, float> >::iterator l;
              for (l = randomPoints.begin(); l != randomPoints.end(); ++l) {
                // Only add the model if it is sufficiently far from the
                // other models
                bool close = false;                
                
                for (unsigned i = 0; i < randomModels.getNumModels(); i++) {
                  float spacing = randomModels.getMatModel(i).model->get_spacing_m() + object->get_spacing_m();
                  spacing = spacing * spacing;
                  
                  if (distSqr(randomModels.getMatModel(i).position, l->first) < spacing) {
                    close = true;                
                    continue;
                  }              
                }            
                
                if (!close) { 
                  randomModels.insert(l->first, object, (int)object->get_randomized_range_m(&seed), l->second);
                }
              }
            }
          }
        }
      }
    }
  }

  bool insertBinObj(const SGBinObject& obj, SGMaterialLib* matlib)
  {
    if (!insertPtGeometry(obj, matlib))
      return false;
    if (!insertSurfaceGeometry(obj, matlib))
      return false;
    return true;
  }
};

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

osg::Node*
SGLoadBTG(const std::string& path, const simgear::SGReaderWriterOptions* options)
{
  SGBinObject tile;
  if (!tile.read_bin(path))
    return NULL;

  SGMaterialLib* matlib = 0;
  bool use_random_objects = false;
  bool use_random_vegetation = false;
  bool use_random_buildings = false;
  float vegetation_density = 1.0f;  
  float building_density = 1.0f;
  if (options) {
    matlib = options->getMaterialLib();
    SGPropertyNode* propertyNode = options->getPropertyNode().get();
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
  }

  SGVec3d center = tile.get_gbs_center();
  SGGeod geodPos = SGGeod::fromCart(center);
  SGQuatd hlOr = SGQuatd::fromLonLat(geodPos)*SGQuatd::fromEulerDeg(0, 0, 180);

  // rotate the tiles so that the bounding boxes get nearly axis aligned.
  // this will help the collision tree's bounding boxes a bit ...
  std::vector<SGVec3d> nodes = tile.get_wgs84_nodes();
  for (unsigned i = 0; i < nodes.size(); ++i)
    nodes[i] = hlOr.transform(nodes[i]);
  tile.set_wgs84_nodes(nodes);

  SGQuatf hlOrf(hlOr[0], hlOr[1], hlOr[2], hlOr[3]);
  std::vector<SGVec3f> normals = tile.get_normals();
  for (unsigned i = 0; i < normals.size(); ++i)
    normals[i] = hlOrf.transform(normals[i]);
  tile.set_normals(normals);

  SGTileGeometryBin tileGeometryBin;
  if (!tileGeometryBin.insertBinObj(tile, matlib))
    return NULL;

  SGVec3f up(0, 0, 1);
  GroundLightManager* lightManager = GroundLightManager::instance();

  osg::ref_ptr<osg::Group> lightGroup = new SGOffsetTransform(0.94);
  osg::ref_ptr<osg::Group> randomObjects;
  osg::ref_ptr<osg::Group> forestNode;
  osg::ref_ptr<osg::Group> buildingNode;  
  osg::Group* terrainGroup = new osg::Group;

  osg::Node* node = tileGeometryBin.getSurfaceGeometry(matlib);
  if (node)
    terrainGroup->addChild(node);

  if (use_random_objects && matlib) {
    tileGeometryBin.computeRandomObjects(matlib);

    if (tileGeometryBin.randomModels.getNumModels() > 0) {
      // Generate a repeatable random seed
      mt seed;
      mt_init(&seed, unsigned(123));

      std::vector<ModelLOD> models;
      for (unsigned int i = 0;
           i < tileGeometryBin.randomModels.getNumModels(); i++) {
        SGMatModelBin::MatModel obj
          = tileGeometryBin.randomModels.getMatModel(i);          

        SGPropertyNode* root = options->getPropertyNode()->getRootNode();
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
        position->addChild(node);
        models.push_back(ModelLOD(position, obj.lod));
      }
      RandomObjectsQuadtree quadtree((GetModelLODCoord()), (AddModelLOD()));
      quadtree.buildQuadTree(models.begin(), models.end());
      randomObjects = quadtree.getRoot();
      randomObjects->setName("random objects");
    }
  }

  if (use_random_vegetation && matlib) {
    // Now add some random forest.
    tileGeometryBin.computeRandomForest(matlib, vegetation_density);
    
    if (tileGeometryBin.randomForest.size() > 0) {
      forestNode = createForest(tileGeometryBin.randomForest, osg::Matrix::identity(),
                                options);
      forestNode->setName("Random trees");
    }
  } 

  if (use_random_buildings && matlib) {
    tileGeometryBin.computeRandomBuildings(matlib, building_density);
    if (tileGeometryBin.randomBuildings.size() > 0) {
      buildingNode = createRandomBuildings(tileGeometryBin.randomBuildings, osg::Matrix::identity(),
                                  options);                                
      buildingNode->setName("Random buildings");
    }
  }  

  // FIXME: ugly, has a side effect
  if (matlib)
    tileGeometryBin.computeRandomSurfaceLights(matlib);

  if (tileGeometryBin.tileLights.getNumLights() > 0
      || tileGeometryBin.randomTileLights.getNumLights() > 0) {
    osg::Group* groundLights0 = new osg::Group;
    groundLights0->setStateSet(lightManager->getGroundLightStateSet());
    groundLights0->setNodeMask(GROUNDLIGHTS0_BIT);
    osg::Geode* geode = new osg::Geode;
    geode->addDrawable(SGLightFactory::getLights(tileGeometryBin.tileLights));
    geode->addDrawable(SGLightFactory::getLights(tileGeometryBin.randomTileLights, 4, -0.3f));
    groundLights0->addChild(geode);
    lightGroup->addChild(groundLights0);
  }
  
  if (tileGeometryBin.randomTileLights.getNumLights() > 0) {
    osg::Group* groundLights1 = new osg::Group;
    groundLights1->setStateSet(lightManager->getGroundLightStateSet());
    groundLights1->setNodeMask(GROUNDLIGHTS1_BIT);
    osg::Group* groundLights2 = new osg::Group;
    groundLights2->setStateSet(lightManager->getGroundLightStateSet());
    groundLights2->setNodeMask(GROUNDLIGHTS2_BIT);
    osg::Geode* geode = new osg::Geode;
    geode->addDrawable(SGLightFactory::getLights(tileGeometryBin.randomTileLights, 2, -0.15f));
    groundLights1->addChild(geode);
    lightGroup->addChild(groundLights1);
    geode = new osg::Geode;
    geode->addDrawable(SGLightFactory::getLights(tileGeometryBin.randomTileLights));
    groundLights2->addChild(geode);
    lightGroup->addChild(groundLights2);
  }

  if (!tileGeometryBin.vasiLights.empty()) {
    EffectGeode* vasiGeode = new EffectGeode;
    Effect* vasiEffect
        = getLightEffect(24, osg::Vec3(1, 0.0001, 0.000001), 1, 24, true);
    vasiGeode->setEffect(vasiEffect);
    SGVec4f red(1, 0, 0, 1);
    SGMaterial* mat = 0;
    if (matlib)
      mat = matlib->find("RWY_RED_LIGHTS");
    if (mat)
      red = mat->get_light_color();
    SGVec4f white(1, 1, 1, 1);
    mat = 0;
    if (matlib)
      mat = matlib->find("RWY_WHITE_LIGHTS");
    if (mat)
      white = mat->get_light_color();
    SGDirectionalLightListBin::const_iterator i;
    for (i = tileGeometryBin.vasiLights.begin();
         i != tileGeometryBin.vasiLights.end(); ++i) {
      vasiGeode->addDrawable(SGLightFactory::getVasi(up, *i, red, white));
    }
    vasiGeode->setStateSet(lightManager->getRunwayLightStateSet());
    lightGroup->addChild(vasiGeode);
  }
  
  Effect* runwayEffect = 0;
  if (tileGeometryBin.runwayLights.getNumLights() > 0
      || !tileGeometryBin.rabitLights.empty()
      || !tileGeometryBin.reilLights.empty()
      || !tileGeometryBin.odalLights.empty()
      || tileGeometryBin.taxiLights.getNumLights() > 0)
      runwayEffect = getLightEffect(16, osg::Vec3(1, 0.001, 0.0002), 1, 16, true);
  if (tileGeometryBin.runwayLights.getNumLights() > 0
      || !tileGeometryBin.rabitLights.empty()
      || !tileGeometryBin.reilLights.empty()
      || !tileGeometryBin.odalLights.empty()
      || !tileGeometryBin.holdshortLights.empty()) {
    osg::Group* rwyLights = new osg::Group;
    rwyLights->setStateSet(lightManager->getRunwayLightStateSet());
    rwyLights->setNodeMask(RUNWAYLIGHTS_BIT);
    if (tileGeometryBin.runwayLights.getNumLights() != 0) {
      EffectGeode* geode = new EffectGeode;
      geode->setEffect(runwayEffect);
      geode->addDrawable(SGLightFactory::getLights(tileGeometryBin
                                                   .runwayLights));
      rwyLights->addChild(geode);
    }
    SGDirectionalLightListBin::const_iterator i;
    for (i = tileGeometryBin.rabitLights.begin();
         i != tileGeometryBin.rabitLights.end(); ++i) {
      rwyLights->addChild(SGLightFactory::getSequenced(*i));
    }
    for (i = tileGeometryBin.reilLights.begin();
         i != tileGeometryBin.reilLights.end(); ++i) {
      rwyLights->addChild(SGLightFactory::getSequenced(*i));
    }
    for (i = tileGeometryBin.holdshortLights.begin();
         i != tileGeometryBin.holdshortLights.end(); ++i) {
      rwyLights->addChild(SGLightFactory::getHoldShort(*i));
    }
    SGLightListBin::const_iterator j;
    for (j = tileGeometryBin.odalLights.begin();
         j != tileGeometryBin.odalLights.end(); ++j) {
      rwyLights->addChild(SGLightFactory::getOdal(*j));
    }
    lightGroup->addChild(rwyLights);
  }

  if (tileGeometryBin.taxiLights.getNumLights() > 0) {
    osg::Group* taxiLights = new osg::Group;
    taxiLights->setStateSet(lightManager->getTaxiLightStateSet());
    taxiLights->setNodeMask(RUNWAYLIGHTS_BIT);
    EffectGeode* geode = new EffectGeode;
    geode->setEffect(runwayEffect);
    geode->addDrawable(SGLightFactory::getLights(tileGeometryBin.taxiLights));
    taxiLights->addChild(geode);
    lightGroup->addChild(taxiLights);
  }

  // The toplevel transform for that tile.
  osg::MatrixTransform* transform = new osg::MatrixTransform;
  transform->setName(path);
  transform->setMatrix(osg::Matrix::rotate(toOsg(hlOr))*
                       osg::Matrix::translate(toOsg(center)));
  transform->addChild(terrainGroup);
  if (lightGroup->getNumChildren() > 0) {
    osg::LOD* lightLOD = new osg::LOD;
    lightLOD->addChild(lightGroup.get(), 0, 30000);
    // VASI is always on, so doesn't use light bits.
    lightLOD->setNodeMask(LIGHTS_BITS | MODEL_BIT); 
    transform->addChild(lightLOD);
  }
  
  if (randomObjects.valid() || forestNode.valid() || buildingNode.valid()) {
  
    // Add a LoD node, so we don't try to display anything when the tile center
    // is more than 20km away.
    osg::LOD* objectLOD = new osg::LOD;
    
    if (randomObjects.valid()) objectLOD->addChild(randomObjects.get(), 0, 20000);
    if (forestNode.valid())  objectLOD->addChild(forestNode.get(), 0, 20000);
    if (buildingNode.valid()) objectLOD->addChild(buildingNode.get(), 0, 20000);
    
    unsigned nodeMask = SG_NODEMASK_CASTSHADOW_BIT | SG_NODEMASK_RECIEVESHADOW_BIT | SG_NODEMASK_TERRAIN_BIT;
    objectLOD->setNodeMask(nodeMask);
    transform->addChild(objectLOD);
  }
  
  return transform;
}
