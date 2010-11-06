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

#ifdef HAVE_CONFIG_H
#  include "simgear_config.h"
#endif

#include "propertyObject.hxx"

namespace simgear
{

SGPropertyNode* static_defaultRoot = NULL;

void PropertyObjectBase::setDefaultRoot(SGPropertyNode* aRoot)
{
  static_defaultRoot = aRoot;
}

PropertyObjectBase::PropertyObjectBase(const char* aChild) :
  _base(NULL),
  _path(aChild),
  _prop(NULL)
{
}
  
PropertyObjectBase::PropertyObjectBase(SGPropertyNode* aNode, const char* aChild) :
  _base(aNode),
  _path(aChild),
  _prop(NULL)
{

}
  
SGPropertyNode* PropertyObjectBase::node(bool aCreate) const
{
  if (_prop) {
    return _prop;
  }
  
  _prop = _base ? _base : static_defaultRoot;
  if (_path) {
    _prop = _prop->getNode(_path, aCreate);
  }
  
  return _prop;
}

} // of namespace simgear

void test()
{
  SGPropObjDouble foo("/bar/foo");

  SGPropertyNode* zoob;

  SGPropObjDouble foo2 = SGPropObjDouble::create(zoob, "foo2", 42.0);
  
  foo = 1123.0;
  foo2 =  43;
  
  std::string s("lalala");
  
  foo = "lalal";
  
  
  SGPropObjString sp(zoob);
  sp = "fooo";
  s =  sp;
  

  SGPropObjBool bp("/some nice big path");
  bp = false;
  
  bp = 456;
  int i5 = bp;
}
