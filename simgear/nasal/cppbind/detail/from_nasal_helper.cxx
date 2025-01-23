///@brief Conversion functions to convert Nasal types to C++ types
// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2018 Thomas Geymayer <tomgey@gmail.com>

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
      return SGPath(std::string(naStr_data(na_str), naStr_len(na_str)),
                    &SGPath::NasalIORulesChecker);
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
