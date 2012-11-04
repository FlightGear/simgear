// Parse CSS colors
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

#include "parse_color.hxx"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/tokenizer.hpp>

namespace simgear
{

  //----------------------------------------------------------------------------
  bool parseColor(std::string str, osg::Vec4& result)
  {
    boost::trim(str);
    osg::Vec4 color(0,0,0,1);

    if( str.empty() )
      return false;

    // #rrggbb
    if( str[0] == '#' )
    {
      const int offsets[] = {2,2,2};
      const boost::offset_separator hex_separator( boost::begin(offsets),
                                                   boost::end(offsets) );
      typedef boost::tokenizer<boost::offset_separator> offset_tokenizer;
      offset_tokenizer tokens(str.begin() + 1, str.end(), hex_separator);

      int comp = 0;
      for( offset_tokenizer::const_iterator tok = tokens.begin();
           tok != tokens.end() && comp < 4;
           ++tok, ++comp )
      {
        color._v[comp] = strtol(std::string(*tok).c_str(), 0, 16) / 255.f;
      }
    }
    // rgb(r,g,b)
    // rgba(r,g,b,a)
    else if( boost::ends_with(str, ")") )
    {
      const std::string RGB = "rgb(",
                        RGBA = "rgba(";
      size_t pos;
      if( boost::starts_with(str, RGB) )
        pos = RGB.length();
      else if( boost::starts_with(str, RGBA) )
        pos = RGBA.length();
      else
        return false;

      typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
      const boost::char_separator<char> del(", \t\n");

      tokenizer tokens(str.begin() + pos, str.end() - 1, del);
      int comp = 0;
      for( tokenizer::const_iterator tok = tokens.begin();
           tok != tokens.end() && comp < 4;
           ++tok, ++comp )
      {
        color._v[comp] = boost::lexical_cast<float>(*tok)
                       // rgb = [0,255], a = [0,1]
                       / (comp < 3 ? 255 : 1);
      }
    }
    else
      return false;

    result = color;
    return true;
  }

} // namespace simgear
