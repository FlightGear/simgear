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

#include <simgear/debug/logstream.hxx>
#include <simgear/io/sg_binobj.hxx>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/math/sg_random.h>
#include <simgear/scene/material/mat.hxx>
#include <simgear/scene/material/matlib.hxx>
#include <simgear/scene/model/SGOffsetTransform.hxx>
#include <simgear/scene/util/SGUpdateVisitor.hxx>
#include <simgear/scene/util/SGNodeMasks.hxx>
#include <simgear/threads/SGThread.hxx>
#include <simgear/threads/SGGuard.hxx>

#include "SGTexturedTriangleBin.hxx"
#include "SGLightBin.hxx"
#include "SGDirectionalLightBin.hxx"

#include "pt_lights.hxx"

typedef std::map<std::string,SGTexturedTriangleBin> SGMaterialTriangleMap;
typedef std::list<SGLightBin> SGLightListBin;
typedef std::list<SGDirectionalLightBin> SGDirectionalLightListBin;

struct SGTileGeometryBin {
  SGMaterialTriangleMap materialTriangleMap;
  SGLightBin tileLights;
  SGLightBin randomTileLights;
  SGDirectionalLightBin runwayLights;
  SGDirectionalLightBin taxiLights;
  SGDirectionalLightListBin vasiLights;
  SGDirectionalLightListBin rabitLights;
  SGLightListBin odalLights;
  SGDirectionalLightListBin reilLights;

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
      SGMaterial* material = matlib->find(materialName);
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

    osg::Geode* geode = new osg::Geode;
    SGMaterialTriangleMap::const_iterator i;
    for (i = materialTriangleMap.begin(); i != materialTriangleMap.end(); ++i) {
      // CHUNCKED (sic) here splits up unconnected triangles parts of
      // the mesh into different Geometry sets, presumably for better
      // culling. I (timoore) believe it is more performant to build
      // the biggest indexed sets possible at the expense of tight
      // culling.
//#define CHUNCKED
#ifdef CHUNCKED
      SGMaterial *mat = matlib->find(i->first);

      std::list<SGTexturedTriangleBin::TriangleVector> connectSets;
      i->second.getConnectedSets(connectSets);

      std::list<SGTexturedTriangleBin::TriangleVector>::iterator j;
      for (j = connectSets.begin(); j != connectSets.end(); ++j) {
        osg::Geometry* geometry = i->second.buildGeometry(*j);
        if (mat)
          geometry->setStateSet(mat->get_state());
        geode->addDrawable(geometry);
      }
#else
      osg::Geometry* geometry = i->second.buildGeometry();
      SGMaterial *mat = matlib->find(i->first);
      if (mat)
        geometry->setStateSet(mat->get_state());
      geode->addDrawable(geometry);
#endif
    }
    return geode;
  }

  void computeRandomSurfaceLights(SGMaterialLib* matlib)
  {
    SGMaterialTriangleMap::const_iterator i;
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
      
      // generate a repeatable random seed
      sg_srandom(unsigned(coverage));

      std::vector<SGVec3f> randomPoints;
      i->second.addRandomSurfacePoints(coverage, 3, randomPoints);
      std::vector<SGVec3f>::iterator j;
      for (j = randomPoints.begin(); j != randomPoints.end(); ++j) {
        float zombie = sg_random();
        // factor = sg_random() ^ 2, range = 0 .. 1 concentrated towards 0
        float factor = sg_random();
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

  bool insertBinObj(const SGBinObject& obj, SGMaterialLib* matlib)
  {
    if (!insertPtGeometry(obj, matlib))
      return false;
    if (!insertSurfaceGeometry(obj, matlib))
      return false;
    return true;
  }
};


class SGTileUpdateCallback : public osg::NodeCallback {
public:
  virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
  {
    assert(dynamic_cast<osg::Switch*>(node));
    assert(dynamic_cast<SGUpdateVisitor*>(nv));

    osg::Switch* lightSwitch = static_cast<osg::Switch*>(node);
    SGUpdateVisitor* updateVisitor = static_cast<SGUpdateVisitor*>(nv);

    // The current sun angle in degree
    float sun_angle = updateVisitor->getSunAngleDeg();

    // vasi is always on
    lightSwitch->setValue(0, true);
    if (sun_angle > 85 || updateVisitor->getVisibility() < 5000) {
      // runway and taxi
      lightSwitch->setValue(1, true);
      lightSwitch->setValue(2, true);
    } else {
      // runway and taxi
      lightSwitch->setValue(1, false);
      lightSwitch->setValue(2, false);
    }
    
    // ground lights
    if ( sun_angle > 95 )
      lightSwitch->setValue(5, true);
    else
      lightSwitch->setValue(5, false);
    if ( sun_angle > 92 )
      lightSwitch->setValue(4, true);
    else
      lightSwitch->setValue(4, false);
    if ( sun_angle > 89 )
      lightSwitch->setValue(3, true);
    else
      lightSwitch->setValue(3, false);

    traverse(node, nv);
  }
};

class SGRunwayLightFogUpdateCallback : public osg::StateAttribute::Callback {
public:
  virtual void operator () (osg::StateAttribute* sa, osg::NodeVisitor* nv)
  {
    assert(dynamic_cast<SGUpdateVisitor*>(nv));
    assert(dynamic_cast<osg::Fog*>(sa));
    SGUpdateVisitor* updateVisitor = static_cast<SGUpdateVisitor*>(nv);
    osg::Fog* fog = static_cast<osg::Fog*>(sa);
    fog->setMode(osg::Fog::EXP2);
    fog->setColor(updateVisitor->getFogColor().osg());
    fog->setDensity(updateVisitor->getRunwayFogExp2Density());
  }
};

class SGTaxiLightFogUpdateCallback : public osg::StateAttribute::Callback {
public:
  virtual void operator () (osg::StateAttribute* sa, osg::NodeVisitor* nv)
  {
    assert(dynamic_cast<SGUpdateVisitor*>(nv));
    assert(dynamic_cast<osg::Fog*>(sa));
    SGUpdateVisitor* updateVisitor = static_cast<SGUpdateVisitor*>(nv);
    osg::Fog* fog = static_cast<osg::Fog*>(sa);
    fog->setMode(osg::Fog::EXP2);
    fog->setColor(updateVisitor->getFogColor().osg());
    fog->setDensity(updateVisitor->getTaxiFogExp2Density());
  }
};

class SGGroundLightFogUpdateCallback : public osg::StateAttribute::Callback {
public:
  virtual void operator () (osg::StateAttribute* sa, osg::NodeVisitor* nv)
  {
    assert(dynamic_cast<SGUpdateVisitor*>(nv));
    assert(dynamic_cast<osg::Fog*>(sa));
    SGUpdateVisitor* updateVisitor = static_cast<SGUpdateVisitor*>(nv);
    osg::Fog* fog = static_cast<osg::Fog*>(sa);
    fog->setMode(osg::Fog::EXP2);
    fog->setColor(updateVisitor->getFogColor().osg());
    fog->setDensity(updateVisitor->getGroundLightsFogExp2Density());
  }
};

osg::Node*
SGLoadBTG(const std::string& path, SGMaterialLib *matlib, bool calc_lights, bool use_random_objects)
{
  SGBinObject obj;
  if (!obj.read_bin(path))
    return false;

  SGTileGeometryBin tileGeometryBin;
  if (!tileGeometryBin.insertBinObj(obj, matlib))
    return false;

  SGVec3d center = obj.get_gbs_center2();
  SGGeod geodPos = SGGeod::fromCart(center);
  SGQuatd hlOr = SGQuatd::fromLonLat(geodPos);
  SGVec3f up = toVec3f(hlOr.backTransform(SGVec3d(0, 0, -1)));

  osg::Group* vasiLights = new osg::Group;
  vasiLights->setCullCallback(new SGPointSpriteLightCullCallback(osg::Vec3(1, 0.0001, 0.000001), 6));
  osg::StateSet* stateSet = vasiLights->getOrCreateStateSet();
  osg::Fog* fog = new osg::Fog;
  fog->setUpdateCallback(new SGRunwayLightFogUpdateCallback);
  stateSet->setAttribute(fog);

  osg::Group* rwyLights = new osg::Group;
  rwyLights->setCullCallback(new SGPointSpriteLightCullCallback);
  stateSet = rwyLights->getOrCreateStateSet();
  fog = new osg::Fog;
  fog->setUpdateCallback(new SGRunwayLightFogUpdateCallback);
  stateSet->setAttribute(fog);

  osg::Group* taxiLights = new osg::Group;
  taxiLights->setCullCallback(new SGPointSpriteLightCullCallback);
  stateSet = taxiLights->getOrCreateStateSet();
  fog = new osg::Fog;
  fog->setUpdateCallback(new SGTaxiLightFogUpdateCallback);
  stateSet->setAttribute(fog);

  osg::Group* groundLights0 = new osg::Group;
  stateSet = groundLights0->getOrCreateStateSet();
  fog = new osg::Fog;
  fog->setUpdateCallback(new SGGroundLightFogUpdateCallback);
  stateSet->setAttribute(fog);

  osg::Group* groundLights1 = new osg::Group;
  stateSet = groundLights1->getOrCreateStateSet();
  fog = new osg::Fog;
  fog->setUpdateCallback(new SGGroundLightFogUpdateCallback);
  stateSet->setAttribute(fog);

  osg::Group* groundLights2 = new osg::Group;
  stateSet = groundLights2->getOrCreateStateSet();
  fog = new osg::Fog;
  fog->setUpdateCallback(new SGGroundLightFogUpdateCallback);
  stateSet->setAttribute(fog);

  osg::Switch* lightSwitch = new osg::Switch;
  lightSwitch->setUpdateCallback(new SGTileUpdateCallback);
  lightSwitch->addChild(vasiLights, true);
  lightSwitch->addChild(rwyLights, true);
  lightSwitch->addChild(taxiLights, true);
  lightSwitch->addChild(groundLights0, true);
  lightSwitch->addChild(groundLights1, true);
  lightSwitch->addChild(groundLights2, true);

  osg::Group* lightGroup = new SGOffsetTransform(0.94);
  lightGroup->addChild(lightSwitch);

  osg::LOD* lightLOD = new osg::LOD;
  lightLOD->addChild(lightGroup, 0, 30000);
  unsigned nodeMask = ~0u;
  nodeMask &= ~SG_NODEMASK_CASTSHADOW_BIT;
  nodeMask &= ~SG_NODEMASK_RECIEVESHADOW_BIT;
  nodeMask &= ~SG_NODEMASK_PICK_BIT;
  nodeMask &= ~SG_NODEMASK_TERRAIN_BIT;
  lightLOD->setNodeMask(nodeMask);

  osg::Group* terrainGroup = new osg::Group;
  osg::Node* node = tileGeometryBin.getSurfaceGeometry(matlib);
  if (node)
    terrainGroup->addChild(node);

  if (calc_lights) {
    // FIXME: ugly, has a side effect
    tileGeometryBin.computeRandomSurfaceLights(matlib);

    osg::Geode* geode = new osg::Geode;
    groundLights0->addChild(geode);
    geode->addDrawable(SGLightFactory::getLights(tileGeometryBin.tileLights));

    groundLights0->addChild(geode);
    geode->addDrawable(SGLightFactory::getLights(tileGeometryBin.randomTileLights, 4, -0.3f));

    geode = new osg::Geode;
    groundLights1->addChild(geode);
    geode->addDrawable(SGLightFactory::getLights(tileGeometryBin.randomTileLights, 2, -0.15f));

    geode = new osg::Geode;
    groundLights2->addChild(geode);
    geode->addDrawable(SGLightFactory::getLights(tileGeometryBin.randomTileLights));
  }

  {
    SGVec4f red(1, 0, 0, 1);
    SGMaterial* mat = matlib->find("RWY_RED_LIGHTS");
    if (mat)
      red = mat->get_light_color();
    SGVec4f white(1, 1, 1, 1);
    mat = matlib->find("RWY_WHITE_LIGHTS");
    if (mat)
      white = mat->get_light_color();
    osg::Geode* geode;
    geode = new osg::Geode;
    vasiLights->addChild(geode);
    SGDirectionalLightListBin::const_iterator i;
    for (i = tileGeometryBin.vasiLights.begin();
         i != tileGeometryBin.vasiLights.end(); ++i) {
      geode->addDrawable(SGLightFactory::getVasi(up, *i, red, white));
    }
    
    geode = new osg::Geode;
    rwyLights->addChild(geode);
    geode->addDrawable(SGLightFactory::getLights(tileGeometryBin.runwayLights));
    for (i = tileGeometryBin.rabitLights.begin();
         i != tileGeometryBin.rabitLights.end(); ++i) {
      rwyLights->addChild(SGLightFactory::getSequenced(*i));
    }
    for (i = tileGeometryBin.reilLights.begin();
         i != tileGeometryBin.reilLights.end(); ++i) {
      rwyLights->addChild(SGLightFactory::getSequenced(*i));
    }

    SGLightListBin::const_iterator j;
    for (j = tileGeometryBin.odalLights.begin();
         j != tileGeometryBin.odalLights.end(); ++j) {
      rwyLights->addChild(SGLightFactory::getOdal(*j));
    }

    geode = new osg::Geode;
    taxiLights->addChild(geode);
    geode->addDrawable(SGLightFactory::getLights(tileGeometryBin.taxiLights));
  }

  // The toplevel transform for that tile.
  osg::MatrixTransform* transform = new osg::MatrixTransform;
  transform->setName(path);
  transform->setMatrix(osg::Matrix::translate(center.osg()));
  transform->addChild(terrainGroup);
  transform->addChild(lightLOD);

  return transform;
}
