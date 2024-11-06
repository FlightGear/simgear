/// @brief Object exposed to Nasal including a Nasal hash for data storage.
//
// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2014  Thomas Geymayer <tomgey@gmail.com>


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
