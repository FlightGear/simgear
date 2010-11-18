// Copyright (C) 2010  James Turner - zakalawe@mac.com
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//

#ifndef SG_PROPERTY_OBJECT
#define SG_PROPERTY_OBJECT

#include <simgear/props/props.hxx>
#include <simgear/structure/exception.hxx>

namespace simgear
{

class PropertyObjectBase
{
public:
  static void setDefaultRoot(SGPropertyNode* aRoot);
  
  PropertyObjectBase(const char* aChild);
  
  PropertyObjectBase(SGPropertyNode* aNode, const char* aChild = NULL);
  
  SGPropertyNode* node(bool aCreate) const;

protected:
  SGPropertyNode* _base;
  const char* _path;
  mutable SGPropertyNode* _prop;
};

template <typename T>
class PropertyObject : PropertyObjectBase
{
public:
  /**
   * Create from path relative to the default root, and option default value
   */
  PropertyObject(const char* aChild) :
    PropertyObjectBase(aChild)
  { }
  
  /**
   * Create from a node, with optional relative path
   */
  PropertyObject(SGPropertyNode* aNode, const char* aChild = NULL) :
    PropertyObjectBase(aNode, aChild)
  {
  
  }
  
// create() form creates the property immediately
  static PropertyObject<T> create(const char* aPath, T aValue)
  {
    PropertyObject<T> p(aPath);
    p = aValue;
    return p;
  }
  
  static PropertyObject<T> create(SGPropertyNode* aNode, T aValue)
  {
    PropertyObject<T> p(aNode);
    p = aValue;
    return p;
  }

  static PropertyObject<T> create(SGPropertyNode* aNode, const char* aChild, T aValue)
  {
    PropertyObject<T> p(aNode, aChild);
    p = aValue;
    return p;
  }
  
// conversion operators
  operator T () const
  {
    SGPropertyNode* n = node(false);
    if (!n) {
      throw sg_exception("read of undefined property:", _path);
    }
    
    return n->getValue<T>();
  }

  T operator=(const T& aValue)
  {
    SGPropertyNode* n = node(true);
    if (!n) {
      return aValue;
    }
    
    n->setValue<T>(aValue);
    return aValue;
  }

}; // of template PropertyObject


// specialization for const char* / std::string

template <>
class PropertyObject<std::string> : PropertyObjectBase
{
public:
  PropertyObject(const char* aChild) :
    PropertyObjectBase(aChild)
  { }
  

  
  PropertyObject(SGPropertyNode* aNode, const char* aChild = NULL) :
    PropertyObjectBase(aNode, aChild)
  {
  
  }
  

  
  operator std::string () const
  {
    SGPropertyNode* n = node(false);
    if (!n) {
      throw sg_exception("read of undefined property:", _path);
    }
    
    return n->getStringValue();
  }
  
  const char* operator=(const char* aValue)
  {
    SGPropertyNode* n = node(true);
    if (!n) {
      return aValue;
    }
    
    n->setStringValue(aValue);
    return aValue;
  }
  
  std::string operator=(const std::string& aValue)
  {
    SGPropertyNode* n = node(true);
    if (!n) {
      return aValue;
    }
    
    n->setStringValue(aValue);
    return aValue;
  }
  
private:
};

} // of namespace simgear

typedef simgear::PropertyObject<double> SGPropObjDouble;
typedef simgear::PropertyObject<bool> SGPropObjBool;
typedef simgear::PropertyObject<std::string> SGPropObjString;
typedef simgear::PropertyObject<long> SGPropObjInt;

#endif
