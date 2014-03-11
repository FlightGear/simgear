// Wrapper class for keeping Nasal objects save from the garbage collector
//
// Copyright (C) 2013  Thomas Geymayer <tomgey@gmail.com>
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

#include "NasalObjectHolder.hxx"
#include <simgear/nasal/nasal.h>

namespace nasal
{

  //----------------------------------------------------------------------------
  ObjectHolder::~ObjectHolder()
  {
    if( !naIsNil(_ref) )
      naGCRelease(_gc_key);
  }

  //----------------------------------------------------------------------------
  naRef ObjectHolder::get_naRef() const
  {
    return _ref;
  }

  //----------------------------------------------------------------------------
  void ObjectHolder::reset()
  {
    if( !naIsNil(_ref) )
      naGCRelease(_gc_key);

    _ref = naNil();
    _gc_key = 0;
  }

  //----------------------------------------------------------------------------
  void ObjectHolder::reset(naRef obj)
  {
    if( !naIsNil(_ref) )
      naGCRelease(_gc_key);

    _ref = obj;
    _gc_key = naGCSave(obj);
  }

  //----------------------------------------------------------------------------
  bool ObjectHolder::valid() const
  {
    return !naIsNil(_ref);
  }

  //----------------------------------------------------------------------------
  ObjectHolderRef ObjectHolder::makeShared(naRef obj)
  {
    return new ObjectHolder(obj);
  }

  //----------------------------------------------------------------------------
  ObjectHolder::ObjectHolder(naRef obj):
    _ref(obj),
    _gc_key(0)
  {
    if( !naIsNil(obj) )
      naGCSave(obj);
  }

  //----------------------------------------------------------------------------
  ObjectHolder::ObjectHolder():
    _ref(naNil()),
    _gc_key(0)
  {

  }

} // namespace nasal
