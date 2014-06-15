// Object exposed to Nasal including a Nasal hash for data storage.
//
// Copyright (C) 2014  Thomas Geymayer <tomgey@gmail.com>
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
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA

#include "NasalObject.hxx"
#include <simgear/nasal/cppbind/NasalHash.hxx>

namespace nasal
{
  //----------------------------------------------------------------------------
  Object::Object(naRef impl):
    _nasal_impl(impl)
  {

  }


  //----------------------------------------------------------------------------
  void Object::setImpl(naRef obj)
  {
    _nasal_impl.reset(obj);
  }

  //----------------------------------------------------------------------------
  naRef Object::getImpl() const
  {
    return _nasal_impl.get_naRef();
  }

  //----------------------------------------------------------------------------
  bool Object::valid() const
  {
    return _nasal_impl.valid();
  }

  //----------------------------------------------------------------------------
  bool Object::_set(naContext c, const std::string& key, naRef val)
  {
    if( !_nasal_impl.valid() )
      _nasal_impl.reset(naNewHash(c));

    nasal::Hash(_nasal_impl.get_naRef(), c).set(key, val);
    return true;
  }

  //----------------------------------------------------------------------------
  bool Object::_get(naContext c, const std::string& key, naRef& out)
  {
    if( !_nasal_impl.valid() )
      return false;

    return naHash_get(_nasal_impl.get_naRef(), to_nasal(c, key), &out);
  }

  //----------------------------------------------------------------------------
  void Object::setupGhost()
  {
    NasalObject::init("Object")
      ._set(&Object::_set)
      ._get(&Object::_get)
      .member("_impl", &Object::getImpl, &Object::setImpl);
  }

} // namespace nasal
