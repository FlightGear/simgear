// Conversion functions to convert Nasal types to C++ types
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

#include "from_nasal_helper.hxx"
#include <simgear/nasal/cppbind/NasalHash.hxx>
#include <simgear/nasal/cppbind/NasalString.hxx>

#include <simgear/misc/sg_path.hxx>

namespace nasal
{
  //----------------------------------------------------------------------------
  bad_nasal_cast::bad_nasal_cast():
    sg_exception("conversion failed", "bad_nasal_cast")
  {

  }

  //----------------------------------------------------------------------------
  bad_nasal_cast::bad_nasal_cast(const std::string& msg):
    sg_exception(msg, "bad_nasal_cast")
  {

  }

  //----------------------------------------------------------------------------
  bad_nasal_cast::~bad_nasal_cast()
  {

  }

  //----------------------------------------------------------------------------
  std::string from_nasal_helper(naContext c, naRef ref, const std::string*)
  {
    naRef na_str = naStringValue(c, ref);

    if( naIsNil(na_str) )
      return std::string();
    else if( !naIsString(na_str) )
      throw bad_nasal_cast("Not convertible to string");

    return std::string(naStr_data(na_str), naStr_len(na_str));
  }

  //----------------------------------------------------------------------------
  SGPath from_nasal_helper(naContext c, naRef ref, const SGPath*)
  {
      naRef na_str = naStringValue(c, ref);
      return SGPath(std::string(naStr_data(na_str), naStr_len(na_str)));
  }

  //----------------------------------------------------------------------------
  Hash from_nasal_helper(naContext c, naRef ref, const Hash*)
  {
    if( !naIsHash(ref) )
      throw bad_nasal_cast("Not a hash");

    return Hash(ref, c);
  }

  //----------------------------------------------------------------------------
  String from_nasal_helper(naContext c, naRef ref, const String*)
  {
    if( !naIsString(ref) )
      throw bad_nasal_cast("Not a string");

    return String(ref);
  }

  //----------------------------------------------------------------------------
  bool from_nasal_helper(naContext c, naRef ref, const bool*)
  {
    return naTrue(ref) == 1;
  }

} // namespace nasal
