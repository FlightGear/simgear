// mat.hxx -- a material in the scene graph.
// TODO: this class needs to be renamed.
//
// Written by Curtis Olson, started May 1998.
// Overhauled by David Megginson, December 2001
//
// Copyright (C) 1998 - 2000  Curtis L. Olson  - curt@flightgear.org
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
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
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

#include <plib/sg.h>
#include <plib/ssg.h>

#include <simgear/props/props.hxx>

#include "matmodel.hxx"

SG_USING_STD(string);
SG_USING_STD(vector);


/**
 * A material in the scene graph.
 *
 * A material represents information about a single surface type
 * in the 3D scene graph, including texture, colour, lighting,
 * tiling, and so on; most of the materials in FlightGear are
 * defined in the $FG_ROOT/materials.xml file, and can be changed
 * at runtime.
 */
class SGMaterial {

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
  SGMaterial( const string &fg_root, const SGPropertyNode *props );


  /**
   * Construct a material from an absolute texture path.
   *
   * @param texture_path A string containing an absolute path
   * to a texture file (usually RGB).
   */
  SGMaterial( const string &texpath );


  /**
   * Construct a material around an existing SSG state.
   *
   * This constructor allows the application to create a custom,
   * low-level state for the scene graph and wrap a material around
   * it.  Note: the pointer ownership is transferred to the material.
   *
   * @param s The SSG state for this material.
   */
  SGMaterial( ssgSimpleState *s );

  /**
   * Destructor.
   */
  virtual ~SGMaterial( void );



  ////////////////////////////////////////////////////////////////////
  // Public methods.
  ////////////////////////////////////////////////////////////////////

  /**
   * Force the texture to load if it hasn't already.
   *
   * @return true if the texture loaded, false if it was loaded
   * already.
   */
  virtual bool load_texture (int n = -1);


  /**
   * Get the textured state.
   */
  virtual ssgSimpleState *get_state (int n = -1) const;


  /**
   * Get the number of textures assigned to this material.
   */
  virtual inline int get_num() const { return _status.size(); }


  /**
   * Get the xsize of the texture, in meters.
   */
  virtual inline double get_xsize() const { return xsize; }


  /**
   * Get the ysize of the texture, in meters.
   */
  virtual inline double get_ysize() const { return ysize; }


  /**
   * Get the light coverage.
   *
   * A smaller number means more generated night lighting.
   *
   * @return The area (m^2?) covered by each light.
   */
  virtual inline double get_light_coverage () const { return light_coverage; }


  /**
   * Get the number of randomly-placed objects defined for this material.
   */
  virtual int get_object_group_count () const { return object_groups.size(); }


  /**
   * Get a randomly-placed object for this material.
   */
  virtual SGMatModelGroup * get_object_group (int index) const {
    return object_groups[index];
  }


  /**
   * Increment the reference count for this material.
   *
   * A material with 0 references may be deleted by the
   * material library.
   */
  virtual inline void ref () { refcount++; }


  /**
   * Decrement the reference count for this material.
   */
  virtual inline void deRef () { refcount--; }


  /**
   * Get the reference count for this material.
   *
   * @return The number of references (0 if none).
   */
  virtual inline int getRef () const { return refcount; }

protected:


  ////////////////////////////////////////////////////////////////////
  // Protected methods.
  ////////////////////////////////////////////////////////////////////

  /**
   * Initialization method, invoked by all public constructors.
   */
  virtual void init();

protected:

  struct _internal_state {
     _internal_state( ssgSimpleState *s, const string &t, bool l )
                  : state(s), texture_path(t), texture_loaded(l) {}
      ssgSimpleState *state;
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
  unsigned int _current_ptr;

  // texture size
  double xsize, ysize;

  // wrap texture?
  bool wrapu, wrapv;

  // use mipmapping?
  int mipmap;

  // coverage of night lighting.
  double light_coverage;

  // material properties
  sgVec4 ambient, diffuse, specular, emission;
  double shininess;

  vector<SGMatModelGroup *> object_groups;

  // ref count so we can properly delete if we have multiple
  // pointers to this record
  int refcount;



  ////////////////////////////////////////////////////////////////////
  // Internal constructors and methods.
  ////////////////////////////////////////////////////////////////////

  SGMaterial( const string &fg_root, const SGMaterial &mat ); // unimplemented

  void read_properties( const string &fg_root, const SGPropertyNode *props );
  void build_ssg_state( bool defer_tex_load );
  void set_ssg_state( ssgSimpleState *s );


};

#endif // _SG_MAT_HXX 
