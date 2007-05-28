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

#include STL_STRING      // Standard C++ string library
#include <vector>
#include <map>

#include <simgear/math/SGMath.hxx>

#include <osg/ref_ptr>
#include <osg/StateSet>

#include <simgear/props/props.hxx>
#include <simgear/structure/SGSharedPtr.hxx>

#include "matmodel.hxx"

SG_USING_STD(string);
SG_USING_STD(vector);
SG_USING_STD(map);


class SGMaterialGlyph;

class SGTextureFilterListener  {
private:
  static int filter;

  SGTextureFilterListener() {
  }
  
public:
  static int getFilter();
  static void setFilter(int filt);
 };

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
  SGMaterial( const string &fg_root, const SGPropertyNode *props, const char *season );


  /**
   * Construct a material from an absolute texture path.
   *
   * @param texture_path A string containing an absolute path
   * to a texture file (usually RGB).
   */
  SGMaterial( const string &texpath );


  /**
   * Construct a material around an existing state.
   *
   * This constructor allows the application to create a custom,
   * low-level state for the scene graph and wrap a material around
   * it.  Note: the pointer ownership is transferred to the material.
   *
   * @param s The state for this material.
   */
  SGMaterial( osg::StateSet *s );

  /**
   * Destructor.
   */
  ~SGMaterial( void );



  ////////////////////////////////////////////////////////////////////
  // Public methods.
  ////////////////////////////////////////////////////////////////////

  /**
   * Force the texture to load if it hasn't already.
   *
   * @return true if the texture loaded, false if it was loaded
   * already.
   */
  bool load_texture (int n = -1);

  /**
   * Get the textured state.
   */
  osg::StateSet *get_state (int n = -1) const;


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
   * @return The area (m^2?) covered by each light.
   */
  inline double get_light_coverage () const { return light_coverage; }

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
  const vector<string>& get_names() const { return _names; }

  /**
   * add the given name to the list of names this material is known
   */
  void add_name(const string& name) { _names.push_back(name); }

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
  SGMaterialGlyph * get_glyph (const string& name) const;

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
      _internal_state( osg::StateSet *s, const string &t, bool l )
                  : state(s), texture_path(t), texture_loaded(l) {}
      osg::ref_ptr<osg::StateSet> state;
      string texture_path;
      bool texture_loaded;
  };

private:


  ////////////////////////////////////////////////////////////////////
  // Internal state.
  ////////////////////////////////////////////////////////////////////

  // texture status
  vector<_internal_state> _status;

  // Round-robin counter
  mutable unsigned int _current_ptr;

  // texture size
  double xsize, ysize;

  // wrap texture?
  bool wrapu, wrapv;

  // use mipmapping?
  int mipmap;

  // coverage of night lighting.
  double light_coverage;

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

  // the list of names for this material. May be empty.
  vector<string> _names;

  vector<SGSharedPtr<SGMatModelGroup> > object_groups;

  // taxiway-/runway-sign texture elements
  map<string, SGSharedPtr<SGMaterialGlyph> > glyphs;


  ////////////////////////////////////////////////////////////////////
  // Internal constructors and methods.
  ////////////////////////////////////////////////////////////////////

  SGMaterial( const string &fg_root, const SGMaterial &mat ); // unimplemented

  void read_properties( const string &fg_root, const SGPropertyNode *props, const char *season );
  void build_state( bool defer_tex_load );
  void set_state( osg::StateSet *s );

  void assignTexture( osg::StateSet *state, const std::string &fname, int _wrapu = TRUE, int _wrapv = TRUE, int _mipmap = TRUE );

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
  SGSharedPtr<const SGMaterial> mMaterial;
};

#endif // _SG_MAT_HXX 
