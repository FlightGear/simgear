// mat.hxx -- a material in the scene graph.
// TODO: this class needs to be renamed.
//
// Written by Curtis Olson, started May 1998.
// Overhauled by David Megginson, December 2001
//
// Copyright (C) 1998 - 2000  Curtis L. Olson  - http://www.flightgear.org/~curt
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


#ifndef _SG_MAT_HXX
#define _SG_MAT_HXX

#include <simgear/compiler.h>

#include <string>      // Standard C++ string library
#include <vector>
#include <map>

#include "Effect.hxx"

#include <osg/ref_ptr>
#include <osg/Texture2D>

namespace osg
{
class StateSet;
}

#include <simgear/structure/SGSharedPtr.hxx>
#include <simgear/threads/SGThread.hxx> // for SGMutex
#include <simgear/math/SGLimits.hxx>
#include <simgear/math/SGMisc.hxx>
#include <simgear/math/SGMath.hxx>
#include <simgear/math/SGVec2.hxx>
#include <simgear/math/SGRect.hxx>
#include <simgear/bvh/BVHMaterial.hxx>

typedef osg::ref_ptr<osg::Texture2D> Texture2DRef;
typedef std::vector<SGRect <float> > AreaList;

namespace simgear
{
class Effect;
void reload_shaders();
class SGReaderWriterOptions;
}

class SGMatModelGroup;
class SGCondition;
class SGPropertyNode;
class SGMaterialGlyph;
class SGTexturedTriangleBin;

/**
 * A material in the scene graph.
 *
 * A material represents information about a single surface type
 * in the 3D scene graph, including texture, colour, lighting,
 * tiling, and so on; most of the materials in FlightGear are
 * defined in the $FG_ROOT/materials.xml file, and can be changed
 * at runtime.
 */
class SGMaterial : public simgear::BVHMaterial {

public:


  ////////////////////////////////////////////////////////////////////
  // Public Constructors.
  ////////////////////////////////////////////////////////////////////

  /**
   * Construct a material from a set of properties.
   *
   * @param props A property node containing subnodes with the
   * state information for the material.  This node is usually
   * loaded from the $FG_ROOT/materials.xml file.
   */
  SGMaterial(const osgDB::Options*,
             const SGPropertyNode *props,
             SGPropertyNode *prop_root,
             AreaList *a,
			 SGSharedPtr<const SGCondition> c);


  SGMaterial(const simgear::SGReaderWriterOptions*,
             const SGPropertyNode *props,
             SGPropertyNode *prop_root,
             AreaList *a,
			 SGSharedPtr<const SGCondition> c);

  /**
   * Destructor.
   */
  ~SGMaterial( void );



  ////////////////////////////////////////////////////////////////////
  // Public methods.
  ////////////////////////////////////////////////////////////////////

  /**
   * Get the textured state.
   */
  simgear::Effect* get_one_effect(int texIndex);
  simgear::Effect* get_effect();

  /**
   * Get the textured state.
   */
  osg::Texture2D* get_one_object_mask(int texIndex);


  /**
   * Get the number of textures assigned to this material.
   */
  inline int get_num() const { return _status.size(); }


  /**
   * Get the xsize of the texture, in meters.
   */
  inline double get_xsize() const { return xsize; }


  /**
   * Get the ysize of the texture, in meters.
   */
  inline double get_ysize() const { return ysize; }


  /**
   * Get the light coverage.
   *
   * A smaller number means more generated night lighting.
   *
   * @return The area (m^2) covered by each light.
   */
  inline double get_light_coverage () const { return light_coverage; }
  
  /**
   * Get the building coverage.
   *
   * A smaller number means more generated buildings.
   *
   * @return The area (m^2) covered by each light.
   */
  inline double get_building_coverage () const { return building_coverage; }

  /**
   * Get the building spacing.
   *
   * This is the minimum spacing between buildings
   *
   * @return The minimum distance between buildings
   */
  inline double get_building_spacing () const { return building_spacing; }

  /**
   * Get the building texture.
   *
   * This is the texture used for auto-generated buildings.
   *
   * @return The texture for auto-generated buildings.
   */
  inline std::string get_building_texture () const { return building_texture; }

  /**
   * Get the building lightmap.
   *
   * This is the lightmap used for auto-generated buildings.
   *
   * @return The lightmap for auto-generated buildings.
   */
  inline std::string get_building_lightmap () const { return building_lightmap; }
  
  // Ratio of the 3 random building sizes
  inline double get_building_small_fraction () const { return building_small_ratio / (building_small_ratio + building_medium_ratio + building_large_ratio); }
  inline double get_building_medium_fraction () const { return building_medium_ratio / (building_small_ratio + building_medium_ratio + building_large_ratio); }
  inline double get_building_large_fraction () const { return building_large_ratio / (building_small_ratio + building_medium_ratio + building_large_ratio); }
  
  // Proportion of buildings with pitched roofs
  inline double get_building_small_pitch () const { return building_small_pitch; }
  inline double get_building_medium_pitch () const { return building_medium_pitch; }
  inline double get_building_large_pitch () const { return building_large_pitch; }

  // Min/Max number of floors for each size
  inline int get_building_small_min_floors () const { return  building_small_min_floors; }
  inline int get_building_small_max_floors () const { return  building_small_max_floors; }
  inline int get_building_medium_min_floors () const { return building_medium_min_floors; }
  inline int get_building_medium_max_floors () const { return building_medium_max_floors; }
  inline int get_building_large_min_floors () const { return building_large_min_floors; }
  inline int get_building_large_max_floors () const { return building_large_max_floors; }
  
  // Minimum width and depth for each size
  inline double get_building_small_min_width () const { return building_small_min_width; }
  inline double get_building_small_max_width () const { return building_small_max_width; }
  inline double get_building_small_min_depth () const { return building_small_min_depth; }
  inline double get_building_small_max_depth () const { return building_small_max_depth; }
  
  inline double get_building_medium_min_width () const { return building_medium_min_width; }
  inline double get_building_medium_max_width () const { return building_medium_max_width; }
  inline double get_building_medium_min_depth () const { return building_medium_min_depth; }
  inline double get_building_medium_max_depth () const { return building_medium_max_depth; }
  
  inline double get_building_large_min_width () const { return building_large_min_width; }
  inline double get_building_large_max_width () const { return building_large_max_width; }
  inline double get_building_large_min_depth () const { return building_large_min_depth; }
  inline double get_building_large_max_depth () const { return building_large_max_depth; }
  
  inline double get_building_range () const { return building_range; }
  
  inline double get_cos_object_max_density_slope_angle () const { return cos_object_max_density_slope_angle; }
  inline double get_cos_object_zero_density_slope_angle () const { return cos_object_zero_density_slope_angle; }

  /**
   * Get the wood coverage.
   *
   * A smaller number means more generated woods within the forest.
   *
   * @return The area (m^2) covered by each wood.
   */
  inline double get_wood_coverage () const { return wood_coverage; }
  
  /**
   * Get the tree height.
   *
   * @return The average height of the trees.
   */
  inline double get_tree_height () const { return tree_height; }

  /**
   * Get the tree width.
   *
   * @return The average width of the trees.
   */
  inline double get_tree_width () const { return tree_width; }

  /**
   * Get the forest LoD range.
   *
   * @return The LoD range for the trees.
   */
  inline double get_tree_range () const { return tree_range; }
  
  /**
   * Get the number of tree varieties available
   *
   * @return the number of different trees defined in the texture strip
   */
  inline int get_tree_varieties () const { return tree_varieties; }
  
  /**
   * Get the texture strip to use for trees
   *
   * @return the texture to use for trees.
   */
  inline std::string get_tree_texture () const { return  tree_texture; }
  
  /**
   * Get the cosine of the maximum tree density slope angle. We
   * use the cosine as it can be compared directly to the z component
   * of a triangle normal.
   * 
   * @return the cosine of the maximum tree density slope angle.
   */
  inline double get_cos_tree_max_density_slope_angle () const { return cos_tree_max_density_slope_angle; }
  
  /**
   * Get the cosine of the maximum tree density slope angle. We
   * use the cosine as it can be compared directly to the z component
   * of a triangle normal.
   * 
   * @return the cosine of the maximum tree density slope angle.
   */
  inline double get_cos_tree_zero_density_slope_angle () const { return cos_tree_zero_density_slope_angle; }
  
  /**
   * Get the list of names for this material
   */
  const std::vector<std::string>& get_names() const { return _names; }

  /**
   * add the given name to the list of names this material is known
   */
  void add_name(const std::string& name) { _names.push_back(name); }

  /**
   * Get the number of randomly-placed objects defined for this material.
   */
  int get_object_group_count () const { return object_groups.size(); }

  /**
   * Get a randomly-placed object for this material.
   */
  SGMatModelGroup * get_object_group (int index) const {
    return object_groups[index];
  }
  
  /**
   * Evaluate whether this material is valid given the current global
   * property state and the tile location.
   */
     bool valid(SGVec2f loc) const;

  /**
   * Return pointer to glyph class, or 0 if it doesn't exist.
   */
  SGMaterialGlyph * get_glyph (const std::string& name) const;

  void set_light_color(const SGVec4f& color)
  { emission = color; }
  const SGVec4f& get_light_color() const
  { return emission; }

  SGVec2f get_tex_coord_scale() const
  {
    float tex_width = get_xsize();
    float tex_height = get_ysize();

    return SGVec2f((0 < tex_width) ? 1000.0f/tex_width : 1.0f,
                   (0 < tex_height) ? 1000.0f/tex_height : 1.0f);
  }
  
protected:


  ////////////////////////////////////////////////////////////////////
  // Protected methods.
  ////////////////////////////////////////////////////////////////////

  /**
   * Initialization method, invoked by all public constructors.
   */
  void init();

protected:

  struct _internal_state {
      _internal_state(simgear::Effect *e, bool l,
                      const simgear::SGReaderWriterOptions *o);
      _internal_state(simgear::Effect *e, const std::string &t, bool l,
                      const simgear::SGReaderWriterOptions *o);
      void add_texture(const std::string &t, int i);
      osg::ref_ptr<simgear::Effect> effect;
      std::vector<std::pair<std::string,int> > texture_paths;
      bool effect_realized;
      osg::ref_ptr<const simgear::SGReaderWriterOptions> options;
  };

private:


  ////////////////////////////////////////////////////////////////////
  // Internal state.
  ////////////////////////////////////////////////////////////////////

  // texture status
  std::vector<_internal_state> _status;

  // texture size
  double xsize, ysize;

  // wrap texture?
  bool wrapu, wrapv;

  // use mipmapping?
  bool mipmap;

  // coverage of night lighting.
  double light_coverage;
  
  // coverage of buildings
  double building_coverage;
  
  // building spacing
  double building_spacing;
  
  // building texture & lightmap
  std::string building_texture;
  std::string building_lightmap;

  // Ratio of the 3 random building sizes
  double building_small_ratio;
  double building_medium_ratio;
  double building_large_ratio;
  
  // Proportion of buildings with pitched roofs
  double building_small_pitch;
  double building_medium_pitch;
  double building_large_pitch;

  // Min/Max number of floors for each size
  int building_small_min_floors; 
  int building_small_max_floors;
  int building_medium_min_floors;
  int building_medium_max_floors;
  int building_large_min_floors;
  int building_large_max_floors;
  
  // Minimum width and depth for each size
  double building_small_min_width;
  double building_small_max_width;
  double building_small_min_depth;
  double building_small_max_depth;
  
  double building_medium_min_width;
  double building_medium_max_width;
  double building_medium_min_depth;
  double building_medium_max_depth;
  
  double building_large_min_width;
  double building_large_max_width;
  double building_large_min_depth;
  double building_large_max_depth;
  
  double building_range;
  
  // Cosine of the angle of maximum and zero density, 
  // used to stop buildings and random objects from being 
  // created on too steep a slope.
  double cos_object_max_density_slope_angle;
  double cos_object_zero_density_slope_angle;
  
  // coverage of woods
  double wood_coverage;

  // Range at which trees become visible
  double tree_range;

  // Height of the tree
  double tree_height;

  // Width of the tree
  double tree_width;

  // Number of varieties of tree texture
  int tree_varieties;
  
  // cosine of the tile angle of maximum and zero density,
  // used to stop trees from being created on too steep a slope.
  double cos_tree_max_density_slope_angle;
  double cos_tree_zero_density_slope_angle;

  // material properties
  SGVec4f ambient, diffuse, specular, emission;
  double shininess;

  // effect for this material
  std::string effect;

  // the list of names for this material. May be empty.
  std::vector<std::string> _names;

  std::vector<SGSharedPtr<SGMatModelGroup> > object_groups;

  // taxiway-/runway-sign texture elements
  std::map<std::string, SGSharedPtr<SGMaterialGlyph> > glyphs;
  
  // Tree texture, typically a strip of applicable tree textures
  std::string tree_texture;
  
  // Object mask, a simple RGB texture used as a mask when placing
  // random vegetation, objects and buildings
  std::vector<Texture2DRef> _masks;
  
  // Condition, indicating when this material is active
  SGSharedPtr<const SGCondition> condition;
  
  // List of geographical rectangles for this material
  AreaList* areas;

  // Parameters from the materials file
  const SGPropertyNode* parameters;

  // per-material lock for entrypoints called from multiple threads
  SGMutex _lock;

  ////////////////////////////////////////////////////////////////////
  // Internal constructors and methods.
  ////////////////////////////////////////////////////////////////////

  void read_properties(const simgear::SGReaderWriterOptions* options,
                        const SGPropertyNode *props,
                        SGPropertyNode *prop_root);
  void buildEffectProperties(const simgear::SGReaderWriterOptions* options);
  simgear::Effect* get_effect(int i);
};


class SGMaterialGlyph : public SGReferenced {
public:
  SGMaterialGlyph(SGPropertyNode *);
  inline double get_left() const { return _left; }
  inline double get_right() const { return _right; }
  inline double get_width() const { return _right - _left; }

protected:
  double _left;
  double _right;
};

class SGMaterialUserData : public osg::Referenced {
public:
  SGMaterialUserData(const SGMaterial* material) :
    mMaterial(material)
  {}
  const SGMaterial* getMaterial() const
  { return mMaterial; }
private:
  // this cannot be an SGSharedPtr since that would create a cicrular reference
  // making it impossible to ever free the space needed by SGMaterial
  const SGMaterial* mMaterial;
};

void
SGSetTextureFilter( int max);

int
SGGetTextureFilter();

#endif // _SG_MAT_HXX 
