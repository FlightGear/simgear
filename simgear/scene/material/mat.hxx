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

#ifndef __cplusplus
# error This library requires C++
#endif

#include <simgear/compiler.h>

#include <string>      // Standard C++ string library
#include <vector>
#include <map>

#include <simgear/math/SGMath.hxx>

#include <osg/ref_ptr>
#include <osgDB/ReaderWriter>

namespace osg
{
class StateSet;
}

#include <simgear/props/props.hxx>
#include <simgear/structure/SGSharedPtr.hxx>
#include <simgear/scene/util/SGSceneFeatures.hxx>

#include "matmodel.hxx"

namespace simgear
{
class Effect;
}

class SGMaterialGlyph;

/**
 * A material in the scene graph.
 *
 * A material represents information about a single surface type
 * in the 3D scene graph, including texture, colour, lighting,
 * tiling, and so on; most of the materials in FlightGear are
 * defined in the $FG_ROOT/materials.xml file, and can be changed
 * at runtime.
 */
class SGMaterial : public SGReferenced {

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
  SGMaterial( const osgDB::ReaderWriter::Options*, const SGPropertyNode *props);

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
  simgear::Effect *get_effect(int n = -1);

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
   * Get the forest coverage.
   *
   * A smaller number means more generated forest canopy.
   *
   * @return The area (m^2) covered by each canopy.
   */
  inline double get_tree_coverage () const { return tree_coverage; }

  /**
   * Get the forest height.
   *
   * @return The average height of the trees.
   */
  inline double get_tree_height () const { return tree_height; }

  /**
   * Get the forest width.
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
   * Return if the surface material is solid, if it is not solid, a fluid
   * can be assumed, that is usually water.
   */
  bool get_solid () const { return solid; }

  /**
   * Get the friction factor for that material
   */
  double get_friction_factor () const { return friction_factor; }

  /**
   * Get the rolling friction for that material
   */
  double get_rolling_friction () const { return rolling_friction; }

  /**
   * Get the bumpines for that material
   */
  double get_bumpiness () const { return bumpiness; }

  /**
   * Get the load resistance
   */
  double get_load_resistance () const { return load_resistance; }

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
      _internal_state(simgear::Effect *e, const std::string &t, bool l,
                      const osgDB::ReaderWriter::Options *o);
      osg::ref_ptr<simgear::Effect> effect;
      std::string texture_path;
      bool effect_realized;
      osg::ref_ptr<const osgDB::ReaderWriter::Options> options;
  };

private:


  ////////////////////////////////////////////////////////////////////
  // Internal state.
  ////////////////////////////////////////////////////////////////////

  // texture status
  std::vector<_internal_state> _status;

  // Round-robin counter
  mutable unsigned int _current_ptr;

  // texture size
  double xsize, ysize;

  // wrap texture?
  bool wrapu, wrapv;

  // use mipmapping?
  bool mipmap;

  // coverage of night lighting.
  double light_coverage;
  
  // coverage of trees
  double tree_coverage;
  
  // Range at which trees become visible
  double tree_range;

  // Height of the tree
  double tree_height;

  // Width of the tree
  double tree_width;

  // Number of varieties of tree texture
  int tree_varieties;

  // True if the material is solid, false if it is a fluid
  bool solid;

  // the friction factor of that surface material
  double friction_factor;

  // the rolling friction of that surface material
  double rolling_friction;

  // the bumpiness of that surface material
  double bumpiness;

  // the load resistance of that surface material
  double load_resistance;

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

  ////////////////////////////////////////////////////////////////////
  // Internal constructors and methods.
  ////////////////////////////////////////////////////////////////////

  void read_properties(const osgDB::ReaderWriter::Options* options,
                        const SGPropertyNode *props);
  void buildEffectProperties(const osgDB::ReaderWriter::Options* options);
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
