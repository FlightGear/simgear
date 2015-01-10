
#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include "obj.hxx"

#include <simgear/scene/material/EffectGeode.hxx>
#include <simgear/scene/material/matlib.hxx>
#include <simgear/scene/material/mat.hxx>

#include "SGTexturedTriangleBin.hxx"

using namespace simgear;

typedef std::map<std::string,SGTexturedTriangleBin> SGMaterialTriangleMap;

// Class handling the initial BTG loading : should probably be in its own file
// it is very closely coupled with SGTexturedTriangleBin.hxx
// it was used to load fans, strips, and triangles.
// WS2.0 no longer uses fans or strips, but people still use ws1.0, so we need 
// to keep this functionality.
class SGTileGeometryBin : public osg::Referenced {
public:
  SGMaterialTriangleMap materialTriangleMap;

  SGTileGeometryBin() {}

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

  SGVec2f getTexCoordScale(const std::string& name, SGMaterialCache* matcache)
  {
    if (!matcache)
      return SGVec2f(1, 1);
    SGMaterial* material = matcache->find(name);
    if (!material)
      return SGVec2f(1, 1);

    return material->get_tex_coord_scale();
  }
  
  static void
  addTriangleGeometry(SGTexturedTriangleBin& triangles,
                      const SGBinObject& obj, unsigned grp,
                      const SGVec2f& tc0Scale, 
                      const SGVec2f& tc1Scale)
  {
    const std::vector<SGVec3d>& vertices(obj.get_wgs84_nodes());
    const std::vector<SGVec3f>& normals(obj.get_normals());
    const std::vector<SGVec2f>& texCoords(obj.get_texcoords());
    const int_list& tris_v(obj.get_tris_v()[grp]);
    const int_list& tris_n(obj.get_tris_n()[grp]);
    const tci_list& tris_tc(obj.get_tris_tcs()[grp]);
    bool  num_norms_is_num_verts = true;  
    
    if (tris_v.size() != tris_n.size()) {
        // If the normal indices do not match, they should be inmplicitly
        // the same than the vertex indices. 
        num_norms_is_num_verts = false;
    }

    if ( !tris_tc[1].empty() ) {
        triangles.hasSecondaryTexCoord(true);
    }
    
    for (unsigned i = 2; i < tris_v.size(); i += 3) {
        SGVertNormTex v0;
        v0.SetVertex( toVec3f(vertices[tris_v[i-2]]) );
        v0.SetNormal( num_norms_is_num_verts ? normals[tris_n[i-2]] : 
                                               normals[tris_v[i-2]] );
        v0.SetTexCoord( 0, getTexCoord(texCoords, tris_tc[0], tc0Scale, i-2) );
        if (!tris_tc[1].empty()) {
            v0.SetTexCoord( 1, getTexCoord(texCoords, tris_tc[1], tc1Scale, i-2) );
        }
        SGVertNormTex v1;
        v1.SetVertex( toVec3f(vertices[tris_v[i-1]]) );
        v1.SetNormal( num_norms_is_num_verts ? normals[tris_n[i-1]] : 
                                               normals[tris_v[i-1]] );
        v1.SetTexCoord( 0, getTexCoord(texCoords, tris_tc[0], tc0Scale, i-1) );
        if (!tris_tc[1].empty()) {
            v1.SetTexCoord( 1, getTexCoord(texCoords, tris_tc[1], tc1Scale, i-1) );
        }
        SGVertNormTex v2;
        v2.SetVertex( toVec3f(vertices[tris_v[i]]) );
        v2.SetNormal( num_norms_is_num_verts ? normals[tris_n[i]] : 
                                               normals[tris_v[i]] );
        v2.SetTexCoord( 0, getTexCoord(texCoords, tris_tc[0], tc0Scale, i) );
        if (!tris_tc[1].empty()) {
            v2.SetTexCoord( 1, getTexCoord(texCoords, tris_tc[1], tc1Scale, i) );
        }
        
        triangles.insert(v0, v1, v2);
    }
  }

  static void
  addStripGeometry(SGTexturedTriangleBin& triangles,
                   const SGBinObject& obj, unsigned grp,
                   const SGVec2f& tc0Scale, 
                   const SGVec2f& tc1Scale)
  {
      const std::vector<SGVec3d>& vertices(obj.get_wgs84_nodes());
      const std::vector<SGVec3f>& normals(obj.get_normals());
      const std::vector<SGVec2f>& texCoords(obj.get_texcoords());
      const int_list& strips_v(obj.get_strips_v()[grp]);
      const int_list& strips_n(obj.get_strips_n()[grp]);
      const tci_list& strips_tc(obj.get_strips_tcs()[grp]);
      bool  num_norms_is_num_verts = true;  
      
      if (strips_v.size() != strips_n.size()) {
          // If the normal indices do not match, they should be inmplicitly
          // the same than the vertex indices. 
          num_norms_is_num_verts = false;
      }
      
      if ( !strips_tc[1].empty() ) {
          triangles.hasSecondaryTexCoord(true);
      }
      
    for (unsigned i = 2; i < strips_v.size(); ++i) {
      SGVertNormTex v0;
      v0.SetVertex( toVec3f(vertices[strips_v[i-2]]) );
      v0.SetNormal( num_norms_is_num_verts ? normals[strips_n[i-2]] : 
                                             normals[strips_v[i-2]] );
      v0.SetTexCoord( 0, getTexCoord(texCoords, strips_tc[0], tc0Scale, i-2) );
      if (!strips_tc[1].empty()) {
          v0.SetTexCoord( 1, getTexCoord(texCoords, strips_tc[1], tc1Scale, i-2) );
      }
      SGVertNormTex v1;
      v1.SetVertex( toVec3f(vertices[strips_v[i-1]]) );
      v1.SetNormal( num_norms_is_num_verts ? normals[strips_n[i-1]] : 
                                             normals[strips_v[i-1]] );
      v1.SetTexCoord( 0, getTexCoord(texCoords, strips_tc[1], tc0Scale, i-1) );
      if (!strips_tc[1].empty()) {
          v1.SetTexCoord( 1, getTexCoord(texCoords, strips_tc[1], tc1Scale, i-1) );
      }
      SGVertNormTex v2;
      v2.SetVertex( toVec3f(vertices[strips_v[i]]) );
      v2.SetNormal( num_norms_is_num_verts ? normals[strips_n[i]] : 
                                             normals[strips_v[i]] );
      v2.SetTexCoord( 0, getTexCoord(texCoords, strips_tc[0], tc0Scale, i) );
      if (!strips_tc[1].empty()) {
          v2.SetTexCoord( 1, getTexCoord(texCoords, strips_tc[1], tc1Scale, i) );
      }
      if (i%2)
        triangles.insert(v1, v0, v2);
      else
        triangles.insert(v0, v1, v2);
    }
  }

  static void
  addFanGeometry(SGTexturedTriangleBin& triangles,
                 const SGBinObject& obj, unsigned grp,
                 const SGVec2f& tc0Scale, 
                 const SGVec2f& tc1Scale)
  {
      const std::vector<SGVec3d>& vertices(obj.get_wgs84_nodes());
      const std::vector<SGVec3f>& normals(obj.get_normals());
      const std::vector<SGVec2f>& texCoords(obj.get_texcoords());
      const int_list& fans_v(obj.get_fans_v()[grp]);
      const int_list& fans_n(obj.get_fans_n()[grp]);
      const tci_list& fans_tc(obj.get_fans_tcs()[grp]);
      bool  num_norms_is_num_verts = true;  
      
      if (fans_v.size() != fans_n.size()) {
          // If the normal indices do not match, they should be inmplicitly
          // the same than the vertex indices. 
          num_norms_is_num_verts = false;
      }
      
      if ( !fans_tc[1].empty() ) {
          triangles.hasSecondaryTexCoord(true);
      }
      
    SGVertNormTex v0;
    v0.SetVertex( toVec3f(vertices[fans_v[0]]) );
    v0.SetNormal( num_norms_is_num_verts ? normals[fans_n[0]] : 
                                           normals[fans_v[0]] );
    v0.SetTexCoord( 0, getTexCoord(texCoords, fans_tc[0], tc0Scale, 0) );
    if (!fans_tc[1].empty()) {
        v0.SetTexCoord( 1, getTexCoord(texCoords, fans_tc[1], tc1Scale, 0) );
    }
    SGVertNormTex v1;
    v1.SetVertex( toVec3f(vertices[fans_v[1]]) );
    v1.SetNormal( num_norms_is_num_verts ? normals[fans_n[1]] : 
                                           normals[fans_v[1]] );
    v1.SetTexCoord( 0, getTexCoord(texCoords, fans_tc[0], tc0Scale, 1) );
    if (!fans_tc[1].empty()) {
        v1.SetTexCoord( 1, getTexCoord(texCoords, fans_tc[1], tc1Scale, 1) );
    }
    for (unsigned i = 2; i < fans_v.size(); ++i) {
      SGVertNormTex v2;
      v2.SetVertex( toVec3f(vertices[fans_v[i]]) );
      v2.SetNormal( num_norms_is_num_verts ? normals[fans_n[i]] : 
                                             normals[fans_v[i]] );
      v2.SetTexCoord( 0, getTexCoord(texCoords, fans_tc[0], tc0Scale, i) );
      if (!fans_tc[1].empty()) {
          v2.SetTexCoord( 1, getTexCoord(texCoords, fans_tc[1], tc1Scale, i) );
      }
      triangles.insert(v0, v1, v2);
      v1 = v2;
    }
  }

  bool
  insertSurfaceGeometry(const SGBinObject& obj, SGMaterialCache* matcache)
  {
    if (obj.get_tris_n().size() < obj.get_tris_v().size() ||
        obj.get_tris_tcs().size() < obj.get_tris_v().size()) {
      SG_LOG(SG_TERRAIN, SG_ALERT,
             "Group list sizes for triangles do not match!");
      return false;
    }

    for (unsigned grp = 0; grp < obj.get_tris_v().size(); ++grp) {
      std::string materialName = obj.get_tri_materials()[grp];
      SGVec2f tc0Scale = getTexCoordScale(materialName, matcache);
      SGVec2f tc1Scale(1.0, 1.0);
      addTriangleGeometry(materialTriangleMap[materialName],
                          obj, grp, tc0Scale, tc1Scale );
    }

    if (obj.get_strips_n().size() < obj.get_strips_v().size() ||
        obj.get_strips_tcs().size() < obj.get_strips_v().size()) {
      SG_LOG(SG_TERRAIN, SG_ALERT,
             "Group list sizes for strips do not match!");
      return false;
    }
    for (unsigned grp = 0; grp < obj.get_strips_v().size(); ++grp) {
      std::string materialName = obj.get_strip_materials()[grp];
      SGVec2f tc0Scale = getTexCoordScale(materialName, matcache);
      SGVec2f tc1Scale(1.0, 1.0);
      addStripGeometry(materialTriangleMap[materialName],
                          obj, grp, tc0Scale, tc1Scale);
    }

    if (obj.get_fans_n().size() < obj.get_fans_v().size() ||
        obj.get_fans_tcs().size() < obj.get_fans_v().size()) {
      SG_LOG(SG_TERRAIN, SG_ALERT,
             "Group list sizes for fans do not match!");
      return false;
    }
    for (unsigned grp = 0; grp < obj.get_fans_v().size(); ++grp) {
      std::string materialName = obj.get_fan_materials()[grp];
      SGVec2f tc0Scale = getTexCoordScale(materialName, matcache);
      SGVec2f tc1Scale(1.0, 1.0);
      addFanGeometry(materialTriangleMap[materialName],
                       obj, grp, tc0Scale, tc1Scale );
    }
    return true;
  }

  osg::Node* getSurfaceGeometry(SGMaterialCache* matcache, bool useVBOs) const
  {
    if (materialTriangleMap.empty())
      return 0;

    EffectGeode* eg = NULL;
    osg::Group* group = (materialTriangleMap.size() > 1 ? new osg::Group : NULL);
    if (group) {
        group->setName("surfaceGeometryGroup");
    }
    
    //osg::Geode* geode = new osg::Geode;
    SGMaterialTriangleMap::const_iterator i;
    for (i = materialTriangleMap.begin(); i != materialTriangleMap.end(); ++i) {
      osg::Geometry* geometry = i->second.buildGeometry(useVBOs);
      SGMaterial *mat = NULL;
      if (matcache) {
        mat = matcache->find(i->first);
      }
      eg = new EffectGeode;
      eg->setName("EffectGeode");
      if (mat) {
        eg->setMaterial(mat);
        eg->setEffect(mat->get_one_effect(i->second.getTextureIndex()));
      } else {
        eg->setMaterial(NULL);
      }
      eg->addDrawable(geometry);
      eg->runGenerators(geometry);  // Generate extra data needed by effect
      if (group) {
        group->addChild(eg);
      }
    }
    
    if (group) {
        return group;
    } else {
        return eg;
    }
  }
};
