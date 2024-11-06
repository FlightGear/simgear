/// @brief Manage lifetime and encapsulate a Nasal context
//
// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2014  Thomas Geymayer <tomgey@gmail.com>


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
