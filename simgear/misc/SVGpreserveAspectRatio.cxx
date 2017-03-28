// Parse and represent SVG preserveAspectRatio attribute
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

#include <simgear_config.h>

#include "SVGpreserveAspectRatio.hxx"

#include <simgear/debug/logstream.hxx>
#include <simgear/misc/strutils.hxx>

#include <boost/tokenizer.hpp>

namespace simgear
{

  //----------------------------------------------------------------------------
  SVGpreserveAspectRatio::SVGpreserveAspectRatio():
    _align_x(ALIGN_NONE),
    _align_y(ALIGN_NONE),
    _meet(true)
  {

  }

  //----------------------------------------------------------------------------
  SVGpreserveAspectRatio::Align SVGpreserveAspectRatio::alignX() const
  {
    return _align_x;
  }

  //----------------------------------------------------------------------------
  SVGpreserveAspectRatio::Align SVGpreserveAspectRatio::alignY() const
  {
    return _align_y;
  }

  //----------------------------------------------------------------------------
  bool SVGpreserveAspectRatio::scaleToFill() const
  {
    return (_align_x == ALIGN_NONE) && (_align_y == ALIGN_NONE);
  }

  //----------------------------------------------------------------------------
  bool SVGpreserveAspectRatio::scaleToFit() const
  {
    return !scaleToFill() && _meet;
  }

  //----------------------------------------------------------------------------
  bool SVGpreserveAspectRatio::scaleToCrop() const
  {
    return !scaleToFill() && !_meet;
  }

  //----------------------------------------------------------------------------
  bool SVGpreserveAspectRatio::meet() const
  {
    return _meet;
  }

  //----------------------------------------------------------------------------
  bool
  SVGpreserveAspectRatio::operator==(const SVGpreserveAspectRatio& rhs) const
  {
    return (_align_x == rhs._align_x)
        && (_align_y == rhs._align_y)
        && (_meet == rhs._meet || scaleToFill());
  }

  //----------------------------------------------------------------------------
  SVGpreserveAspectRatio SVGpreserveAspectRatio::parse(const std::string& str)
  {
    SVGpreserveAspectRatio ret;
    enum
    {
      PARSE_defer,
      PARSE_align,
      PARSE_meetOrSlice,
      PARSE_done,
      PARSE_error
    } parse_state = PARSE_defer;

    typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
    const boost::char_separator<char> del(" \t\n");

    tokenizer tokens(str.begin(), str.end(), del);
    for( tokenizer::const_iterator tok = tokens.begin();
                                   tok != tokens.end()
                                && parse_state != PARSE_error;
                                 ++tok )
    {
      const std::string& cur_tok = tok.current_token();

      switch( parse_state )
      {
        case PARSE_defer:
          if( cur_tok == "defer" )
          {
            SG_LOG( SG_GENERAL,
                    SG_INFO,
                    "SVGpreserveAspectRatio: 'defer' is ignored." );
            parse_state = PARSE_align;
            break;
          }
          // fall through
        case PARSE_align:
          if( cur_tok == "none" )
          {
            ret._align_x = ALIGN_NONE;
            ret._align_y = ALIGN_NONE;
          }
          else if( cur_tok.length() == 8 )
          {
            if(      strutils::starts_with(cur_tok, "xMin") )
              ret._align_x = ALIGN_MIN;
            else if( strutils::starts_with(cur_tok, "xMid") )
              ret._align_x = ALIGN_MID;
            else if( strutils::starts_with(cur_tok, "xMax") )
              ret._align_x = ALIGN_MAX;
            else
            {
              parse_state = PARSE_error;
              break;
            }

            if(      strutils::ends_with(cur_tok, "YMin") )
              ret._align_y = ALIGN_MIN;
            else if( strutils::ends_with(cur_tok, "YMid") )
              ret._align_y = ALIGN_MID;
            else if( strutils::ends_with(cur_tok, "YMax") )
              ret._align_y = ALIGN_MAX;
            else
            {
              parse_state = PARSE_error;
              break;
            }
          }
          else
          {
            parse_state = PARSE_error;
            break;
          }
          parse_state = PARSE_meetOrSlice;
          break;
        case PARSE_meetOrSlice:
          if( cur_tok == "meet" )
            ret._meet = true;
          else if( cur_tok == "slice" )
            ret._meet = false;
          else
          {
            parse_state = PARSE_error;
            break;
          }
          parse_state = PARSE_done;
          break;
        case PARSE_done:
          SG_LOG( SG_GENERAL,
                  SG_WARN,
                  "SVGpreserveAspectRatio: Ignoring superfluous token"
                  " '" << cur_tok << "'" );
          break;
        default:
          break;
      }
    }

    if( parse_state == PARSE_error )
    {
      SG_LOG( SG_GENERAL,
              SG_WARN,
              "SVGpreserveAspectRatio: Failed to parse: '" << str << "'" );
      return SVGpreserveAspectRatio();
    }

    return ret;
  }

} // namespace simgear
