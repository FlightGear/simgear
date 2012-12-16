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

#include "to_nasal.hxx"
#include "NasalHash.hxx"

namespace nasal
{
  //----------------------------------------------------------------------------
  naRef to_nasal(naContext c, const std::string& str)
  {
    naRef ret = naNewString(c);
    naStr_fromdata(ret, str.c_str(), str.size());
    return ret;
  }

  //----------------------------------------------------------------------------
  naRef to_nasal(naContext c, const char* str)
  {
    return to_nasal(c, std::string(str));
  }

  //----------------------------------------------------------------------------
  naRef to_nasal(naContext c, naCFunction func)
  {
    return naNewFunc(c, naNewCCode(c, func));
  }

  //----------------------------------------------------------------------------
  naRef to_nasal(naContext c, const Hash& hash)
  {
    return hash.get_naRef();
  }

  //----------------------------------------------------------------------------
  naRef to_nasal(naContext c, naRef ref)
  {
    return ref;
  }

} // namespace nasal
