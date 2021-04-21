// Conversion functions to convert C++ types to Nasal types
//
// Copyright (C) 2012  Thomas Geymayer <tomgey@gmail.com>
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

#include "to_nasal_helper.hxx"

#include <memory>

#include <simgear/nasal/cppbind/NasalHash.hxx>
#include <simgear/nasal/cppbind/Ghost.hxx>

#include <simgear/structure/exception.hxx>
#include <simgear/math/SGMath.hxx>
#include <simgear/misc/sg_path.hxx>

namespace nasal
{


  //----------------------------------------------------------------------------
  naRef to_nasal_helper(naContext c, const std::string& str)
  {
    naRef ret = naNewString(c);
    naStr_fromdata(ret, str.c_str(), static_cast<int>(str.size()));
    return ret;
  }

  //----------------------------------------------------------------------------
  naRef to_nasal_helper(naContext c, const char* str)
  {
    return to_nasal_helper(c, std::string(str));
  }

  //----------------------------------------------------------------------------
  naRef to_nasal_helper(naContext c, const Hash& hash)
  {
    return hash.get_naRef();
  }

  //----------------------------------------------------------------------------
  naRef to_nasal_helper(naContext c, const naRef& ref)
  {
    return ref;
  }

  //------------------------------------------------------------------------------
  naRef to_nasal_helper(naContext c, const SGGeod& pos)
  {
    nasal::Hash hash(c);
    hash.set("lat", pos.getLatitudeDeg());
    hash.set("lon", pos.getLongitudeDeg());
    hash.set("elevation", pos.getElevationM());
    return hash.get_naRef();
  }

  //----------------------------------------------------------------------------
  naRef to_nasal_helper(naContext c, const SGPath& path)
  {
    return to_nasal_helper(c, path.utf8Str());
  }

  //----------------------------------------------------------------------------
  naRef to_nasal_helper(naContext c, naCFunction func)
  {
    return naNewFunc(c, naNewCCode(c, func));
  }

  //----------------------------------------------------------------------------
  static naRef free_function_invoker( naContext c,
                                      naRef me,
                                      int argc,
                                      naRef* args,
                                      void* user_data )
  {
    free_function_t* func = static_cast<free_function_t*>(user_data);
    assert(func);

    try
    {
      return (*func)(nasal::CallContext(c, me, argc, args));
    }
    // CppUnit::Exception inherits std::exception, so restrict ourselves to
    // sg_exception
    catch (const sg_exception& e)
    {
       naRuntimeError(c, "Fatal error in Nasal call: %s", e.what());
    }

    return naNil();
  }

  //----------------------------------------------------------------------------
  static void free_function_destroy(void* func)
  {
    delete static_cast<free_function_t*>(func);
  }

  //----------------------------------------------------------------------------
  naRef to_nasal_helper(naContext c, const free_function_t& func)
  {
    return naNewFunc
    (
      c,
      naNewCCodeUD
      (
        c,
        &free_function_invoker,
        new free_function_t(func),
        &free_function_destroy
      )
    );
  }

} // namespace nasal
