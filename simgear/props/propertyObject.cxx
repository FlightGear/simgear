// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2010 James Turner <james@flightgear.com>

#ifdef HAVE_CONFIG_H
#  include "simgear_config.h"
#endif

#include "propertyObject.hxx"

#include <simgear/structure/exception.hxx>

namespace simgear
{

SGPropertyNode* static_defaultRoot = NULL;

void PropertyObjectBase::setDefaultRoot(SGPropertyNode* aRoot)
{
  static_defaultRoot = aRoot;
}
  
PropertyObjectBase::PropertyObjectBase() :
  _path(NULL),
  _prop(NULL)
{
    
}

PropertyObjectBase::PropertyObjectBase(const PropertyObjectBase& aOther) :
  _path(aOther._path),
  _prop(aOther._prop)
{

}

PropertyObjectBase::PropertyObjectBase(const char* aChild) :
  _path(aChild),
  _prop(NULL)
{
}
  
PropertyObjectBase::PropertyObjectBase(SGPropertyNode* aNode, const char* aChild) :
  _path(aChild),
  _prop(aNode)
{

}
  
SGPropertyNode* PropertyObjectBase::node(bool aCreate) const
{
  if (_path == NULL) { // already resolved
    return _prop;
  }
  
  SGPropertyNode *r = _prop ? _prop : static_defaultRoot,
                 *prop = r->getNode(_path, aCreate);
  
  if( prop )
  {
    // resolve worked, we will cache from now on, so clear _path and cache prop
    _path = NULL;
    _prop = prop;
  }

  return prop;
}

SGPropertyNode* PropertyObjectBase::getOrThrow() const
{
  SGPropertyNode* n = node(false);
  if (!n) {
    std::string path;
    if (_prop) {
      path = _prop->getPath();
      if (_path) {
        path += '/';      
      }
    }

    if (_path) {
      path += _path;
    }

    throw sg_exception("Unknown property:" + path);
  }

  return n;
}

} // of namespace simgear

