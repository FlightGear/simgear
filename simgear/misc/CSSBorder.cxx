// Parse and represent CSS border definitions (eg. margin, border-image-width)
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

#include "CSSBorder.hxx"

#include <boost/tokenizer.hpp>

namespace simgear
{

  //----------------------------------------------------------------------------
  bool CSSBorder::isValid() const
  {
    return valid;
  }

  //----------------------------------------------------------------------------
  bool CSSBorder::isNone() const
  {
    return !valid
        || (  offsets.t == 0
           && offsets.r == 0
           && offsets.b == 0
           && offsets.l == 0 );
  }

  //----------------------------------------------------------------------------
  const std::string& CSSBorder::getKeyword() const
  {
    return keyword;
  }

  //----------------------------------------------------------------------------
  CSSBorder::Offsets CSSBorder::getRelOffsets(const SGRect<int>& dim) const
  {
    Offsets ret = {{0}};
    if( !valid )
      return ret;

    for(int i = 0; i < 4; ++i)
    {
      ret.val[i] = offsets.val[i];
      if( !types.rel[i] )
        ret.val[i] /= (i & 1) ? dim.width() : dim.height();
    }

    return ret;
  }

  //----------------------------------------------------------------------------
  CSSBorder::Offsets CSSBorder::getAbsOffsets(const SGRect<int>& dim) const
  {
    Offsets ret = {{0}};
    if( !valid )
      return ret;

    for(int i = 0; i < 4; ++i)
    {
      ret.val[i] = offsets.val[i];
      if( types.rel[i] )
        ret.val[i] *= (i & 1) ? dim.width() : dim.height();
    }

    return ret;
  }

  //----------------------------------------------------------------------------
  CSSBorder CSSBorder::parse(const std::string& str)
  {
    if( str.empty() )
      return CSSBorder();

    // [<number>'%'?]{1,4} (top[,right[,bottom[,left]]])
    //
    // Percentages are relative to the size of the image: the width of the
    // image for the horizontal offsets, the height for vertical offsets.
    // Numbers represent pixels in the image.
    int c = 0;
    CSSBorder ret;

    typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
    const boost::char_separator<char> del(" \t\n");

    tokenizer tokens(str.begin(), str.end(), del);
    for( tokenizer::const_iterator tok = tokens.begin();
         tok != tokens.end() && c < 4;
         ++tok )
    {
      if( isalpha(*tok->begin()) )
        ret.keyword = *tok;
      else
      {
        bool rel = ret.types.rel[c] = (*tok->rbegin() == '%');
        ret.offsets.val[c] =
          // Negative values are not allowed and values bigger than the size of
          // the image are interpreted as ‘100%’. TODO check max
          std::max
          (
            0.f,
            std::stof
            (
              rel ? std::string(tok->begin(), tok->end() - 1)
                  : *tok
            )
            /
            (rel ? 100 : 1)
          );
        ++c;
      }
    }

    // When four values are specified, they set the offsets on the top, right,
    // bottom and left sides in that order.

#define CSS_COPY_VAL(dest, src)\
  {\
    ret.offsets.val[dest] = ret.offsets.val[src];\
    ret.types.rel[dest] = ret.types.rel[src];\
  }

    if( c < 4 )
    {
      if( c < 3 )
      {
        if( c < 2 )
          // if the right is missing, it is the same as the top.
          CSS_COPY_VAL(1, 0);

        // if the bottom is missing, it is the same as the top
        CSS_COPY_VAL(2, 0);
      }

      // If the left is missing, it is the same as the right
      CSS_COPY_VAL(3, 1);
    }

#undef CSS_COPY_VAL

    if( ret.keyword == "none" )
    {
      memset(&ret.offsets, 0, sizeof(Offsets));
      ret.keyword.clear();
    }

    ret.valid = true;
    return ret;
  }

} // namespace simgear
