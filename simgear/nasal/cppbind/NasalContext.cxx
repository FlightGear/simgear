// Manage lifetime and encapsulate a Nasal context
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

#include "NasalContext.hxx"
#include "NasalHash.hxx"
#include "NasalString.hxx"

#include <cassert>
#include <stdexcept> // for std::runtime_error

namespace nasal
{

  //----------------------------------------------------------------------------
  ContextWrapper::ContextWrapper(naContext ctx):
    _ctx(ctx)
  {
    assert(_ctx);
  }

  //----------------------------------------------------------------------------
  ContextWrapper::operator naContext()
  {
    return _ctx;
  }

  //----------------------------------------------------------------------------
  naContext ContextWrapper::c_ctx() const
  {
    return const_cast<naContext>(_ctx);
  }

  //----------------------------------------------------------------------------
  Hash ContextWrapper::newHash()
  {
    return Hash(_ctx);
  }

  //----------------------------------------------------------------------------
  String ContextWrapper::newString(const char* str)
  {
    return String(_ctx, str);
  }

  //----------------------------------------------------------------------------
  naRef ContextWrapper::callMethod( Me me,
                                    naRef code,
                                    std::initializer_list<naRef> args )
  {
    naRef ret = naCallMethodCtx(
      _ctx,
      code,
      me,
      args.size(),
      const_cast<naRef*>(args.begin()),
      naNil() // locals
    );

    if( const char* error = naGetError(_ctx) )
      throw std::runtime_error(error);

    return ret;
  }

  //----------------------------------------------------------------------------
  naRef ContextWrapper::newVector(std::initializer_list<naRef> vals)
  {
    naRef vec = naNewVector(_ctx);
    naVec_setsize(_ctx, vec, vals.size());
    int i = 0;
    for(naRef val: vals)
      naVec_set(vec, i++, val);
    return vec;
  }

  //----------------------------------------------------------------------------
  Context::Context():
    ContextWrapper(naNewContext())
  {

  }

  //----------------------------------------------------------------------------
  Context::~Context()
  {
    naFreeContext(_ctx);
    _ctx = nullptr;
  }

} // namespace nasal
