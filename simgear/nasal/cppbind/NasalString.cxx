// Wrapper class for Nasal strings
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

#include "NasalString.hxx"
#include "to_nasal.hxx"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <functional>
#include <stdexcept>

namespace nasal
{

//template<typename Iterator>
//Iterator
//rfind_first_of( Iterator rbegin_src, Iterator rend_src,
//                Iterator begin_find, Iterator end_find )
//{
//  for(; rbegin_src != rend_src; --rbegin_src)
//  {
//    for(Iterator chr = begin_find; chr != end_find; ++chr)
//    {
//      if( *rbegin_src == *chr )
//        return rbegin_src;
//    }
//  }
//  return rend_src;
//}


  const size_t String::npos = static_cast<size_t>(-1);

  //----------------------------------------------------------------------------
  String::String(naContext c, const char* str):
    _str( to_nasal(c, str) )
  {
    assert( naIsString(_str) );
  }

  //----------------------------------------------------------------------------
  String::String(naRef str):
    _str(str)
  {
    assert( naIsString(_str) );
  }

  //----------------------------------------------------------------------------
  const char* String::c_str() const
  {
    return naStr_data(_str);
  }

  //----------------------------------------------------------------------------
  const char* String::begin() const
  {
    return c_str();
  }

  //----------------------------------------------------------------------------
  const char* String::end() const
  {
    return c_str() + size();
  }

  //----------------------------------------------------------------------------
  size_t String::size() const
  {
    return naStr_len(_str);
  }

  //----------------------------------------------------------------------------
  size_t String::length() const
  {
    return size();
  }

  //----------------------------------------------------------------------------
  bool String::empty() const
  {
    return size() == 0;
  }

  //----------------------------------------------------------------------------
  int String::compare(size_t pos, size_t len, const String& rhs) const
  {
    if( pos >= size() )
      throw std::out_of_range("nasal::String::compare: pos");

    return memcmp( begin() + pos,
                   rhs.begin(),
                   std::min(rhs.size(), std::min(size() - pos, len)) );
  }

  //----------------------------------------------------------------------------
  bool String::starts_with(const String& rhs) const
  {
    return rhs.size() <= size() && compare(0, npos, rhs) == 0;
  }

  //----------------------------------------------------------------------------
  bool String::ends_with(const String& rhs) const
  {
    return rhs.size() <= size() && compare(size() - rhs.size(), npos, rhs) == 0;
  }

  //----------------------------------------------------------------------------
  size_t String::find(const char c, size_t pos) const
  {
    if( pos >= size() )
      return npos;

    const char* result = std::find(begin() + pos, end(), c);

    return result != end() ? result - begin() : npos;
  }

  //----------------------------------------------------------------------------
  size_t String::find_first_of(const String& chr, size_t pos) const
  {
    if( pos >= size() )
      return npos;

    const char* result = std::find_first_of( begin() + pos, end(),
                                             chr.begin(), chr.end() );

    return result != end() ? result - begin() : npos;
  }

  //----------------------------------------------------------------------------
  size_t String::find_first_not_of(const String& chr, size_t pos) const
  {
    if( pos >= size() )
      return npos;

    const char* result = std::find_if(begin() + pos, end(), [&chr](const char c) {
        return std::find(chr.begin(), chr.end(), c) == chr.end();
    });

    return result != end() ? result - begin() : npos;
  }

  //----------------------------------------------------------------------------
  const naRef String::get_naRef() const
  {
    return _str;
  }

} // namespace nasal
