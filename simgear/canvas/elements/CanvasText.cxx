// A text on the Canvas
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

#include "CanvasText.hxx"
#include <simgear/canvas/Canvas.hxx>
#include <simgear/canvas/CanvasSystemAdapter.hxx>
#include <simgear/scene/util/parse_color.hxx>
#include <osg/Version>
#include <osgText/Text>

namespace simgear
{
namespace canvas
{
  class Text::TextOSG:
    public osgText::Text
  {
    public:

      TextOSG(canvas::Text* text);

      void setFontResolution(int res);
      void setCharacterAspect(float aspect);
      void setLineHeight(float factor);
      void setFill(const std::string& fill);
      void setBackgroundColor(const std::string& fill);

      int heightForWidth(int w) const;
      osg::Vec2 handleHit(const osg::Vec2f& pos);

      virtual osg::BoundingBox computeBound() const;

    protected:

      canvas::Text *_text_element;

      virtual void computePositions(unsigned int contextID) const;
  };

  //----------------------------------------------------------------------------
  Text::TextOSG::TextOSG(canvas::Text* text):
    _text_element(text)
  {
    setBackdropImplementation(NO_DEPTH_BUFFER);
  }

  //----------------------------------------------------------------------------
  void Text::TextOSG::setFontResolution(int res)
  {
    TextBase::setFontResolution(res, res);
  }

  //----------------------------------------------------------------------------
  void Text::TextOSG::setCharacterAspect(float aspect)
  {
    setCharacterSize(getCharacterHeight(), aspect);
  }

  //----------------------------------------------------------------------------
  void Text::TextOSG::setLineHeight(float factor)
  {
    setLineSpacing(factor - 1);
  }

  //----------------------------------------------------------------------------
  void Text::TextOSG::setFill(const std::string& fill)
  {
//    if( fill == "none" )
//      TODO No text
//    else
    osg::Vec4 color;
    if( parseColor(fill, color) )
      setColor( color );
  }

  //----------------------------------------------------------------------------
  void Text::TextOSG::setBackgroundColor(const std::string& fill)
  {
    osg::Vec4 color;
    if( parseColor(fill, color) )
      setBoundingBoxColor( color );
  }

  //----------------------------------------------------------------------------
  // simplified version of osgText::Text::computeGlyphRepresentation() to
  // just calculate the height for a given weight. Glpyh calculations/creating
  // is not necessary for this...
  int Text::TextOSG::heightForWidth(int w) const
  {
    if( _text.empty() )
      return 0;

    osgText::Font* activefont = const_cast<osgText::Font*>(getActiveFont());
    if( !activefont )
      return -1;

    float max_width_safe = _maximumWidth;
    const_cast<TextOSG*>(this)->_maximumWidth = w;

    SGRecti bb;

    osg::Vec2 startOfLine_coords(0.0f,0.0f);
    osg::Vec2 cursor(startOfLine_coords);
    osg::Vec2 local(0.0f,0.0f);

    unsigned int previous_charcode = 0;
    unsigned int line_length = 0;
    bool horizontal = _layout != VERTICAL;
    bool kerning = true;

    float hr = _characterHeight;
    float wr = hr / getCharacterAspectRatio();

    // osg should really care more about const :-/
    osgText::String& text = const_cast<osgText::String&>(_text);
    typedef osgText::String::iterator TextIterator;

    for( TextIterator itr = text.begin(); itr != text.end(); )
    {
      // record the start of the current line
      TextIterator startOfLine_itr = itr;

      // find the end of the current line.
      osg::Vec2 endOfLine_coords(cursor);
      TextIterator endOfLine_itr =
        const_cast<TextOSG*>(this)->computeLastCharacterOnLine(
          endOfLine_coords, itr, text.end()
        );

      line_length = endOfLine_itr - startOfLine_itr;

      // Set line position to correct alignment.
      switch( _layout )
      {
        case LEFT_TO_RIGHT:
        {
          switch( _alignment )
          {
            // nothing to be done for these
            //case LEFT_TOP:
            //case LEFT_CENTER:
            //case LEFT_BOTTOM:
            //case LEFT_BASE_LINE:
            //case LEFT_BOTTOM_BASE_LINE:
            //  break;
            case CENTER_TOP:
            case CENTER_CENTER:
            case CENTER_BOTTOM:
            case CENTER_BASE_LINE:
            case CENTER_BOTTOM_BASE_LINE:
              cursor.x() = (cursor.x() - endOfLine_coords.x()) * 0.5f;
              break;
            case RIGHT_TOP:
            case RIGHT_CENTER:
            case RIGHT_BOTTOM:
            case RIGHT_BASE_LINE:
            case RIGHT_BOTTOM_BASE_LINE:
              cursor.x() = cursor.x() - endOfLine_coords.x();
              break;
            default:
              break;
          }
          break;
        }
        case RIGHT_TO_LEFT:
        {
          switch( _alignment )
          {
            case LEFT_TOP:
            case LEFT_CENTER:
            case LEFT_BOTTOM:
            case LEFT_BASE_LINE:
            case LEFT_BOTTOM_BASE_LINE:
              cursor.x() = 2 * cursor.x() - endOfLine_coords.x();
              break;
            case CENTER_TOP:
            case CENTER_CENTER:
            case CENTER_BOTTOM:
            case CENTER_BASE_LINE:
            case CENTER_BOTTOM_BASE_LINE:
              cursor.x() = cursor.x()
                  + (cursor.x() - endOfLine_coords.x()) * 0.5f;
              break;
              // nothing to be done for these
              //case RIGHT_TOP:
              //case RIGHT_CENTER:
              //case RIGHT_BOTTOM:
              //case RIGHT_BASE_LINE:
              //case RIGHT_BOTTOM_BASE_LINE:
              //  break;
            default:
              break;
          }
          break;
        }
        case VERTICAL:
        {
          switch( _alignment )
          {
            // TODO: current behaviour top baselines lined up in both cases - need to implement
            //       top of characters alignment - Question is this necessary?
            // ... otherwise, nothing to be done for these 6 cases
            //case LEFT_TOP:
            //case CENTER_TOP:
            //case RIGHT_TOP:
            //  break;
            //case LEFT_BASE_LINE:
            //case CENTER_BASE_LINE:
            //case RIGHT_BASE_LINE:
            //  break;
            case LEFT_CENTER:
            case CENTER_CENTER:
            case RIGHT_CENTER:
              cursor.y() = cursor.y()
                  + (cursor.y() - endOfLine_coords.y()) * 0.5f;
              break;
            case LEFT_BOTTOM_BASE_LINE:
            case CENTER_BOTTOM_BASE_LINE:
            case RIGHT_BOTTOM_BASE_LINE:
              cursor.y() = cursor.y() - (line_length * _characterHeight);
              break;
            case LEFT_BOTTOM:
            case CENTER_BOTTOM:
            case RIGHT_BOTTOM:
              cursor.y() = 2 * cursor.y() - endOfLine_coords.y();
              break;
            default:
              break;
          }
          break;
        }
      }

      if( itr != endOfLine_itr )
      {

        for(;itr != endOfLine_itr;++itr)
        {
          unsigned int charcode = *itr;

          osgText::Glyph* glyph = activefont->getGlyph(_fontSize, charcode);
          if( glyph )
          {
            float width = (float) (glyph->getWidth()) * wr;
            float height = (float) (glyph->getHeight()) * hr;

            if( _layout == RIGHT_TO_LEFT )
            {
              cursor.x() -= glyph->getHorizontalAdvance() * wr;
            }

            // adjust cursor position w.r.t any kerning.
            if( kerning && previous_charcode )
            {
              switch( _layout )
              {
                case LEFT_TO_RIGHT:
                {
                  osg::Vec2 delta( activefont->getKerning( previous_charcode,
                                                           charcode,
                                                           _kerningType ) );
                  cursor.x() += delta.x() * wr;
                  cursor.y() += delta.y() * hr;
                  break;
                }
                case RIGHT_TO_LEFT:
                {
                  osg::Vec2 delta( activefont->getKerning( charcode,
                                                           previous_charcode,
                                                           _kerningType ) );
                  cursor.x() -= delta.x() * wr;
                  cursor.y() -= delta.y() * hr;
                  break;
                }
                case VERTICAL:
                  break; // no kerning when vertical.
              }
            }

            local = cursor;
            osg::Vec2 bearing( horizontal ? glyph->getHorizontalBearing()
                                          : glyph->getVerticalBearing() );
            local.x() += bearing.x() * wr;
            local.y() += bearing.y() * hr;

            // set up the coords of the quad
            osg::Vec2 upLeft = local + osg::Vec2(0.f, height);
            osg::Vec2 lowLeft = local;
            osg::Vec2 lowRight = local + osg::Vec2(width, 0.f);
            osg::Vec2 upRight = local + osg::Vec2(width, height);

            // move the cursor onto the next character.
            // also expand bounding box
            switch( _layout )
            {
              case LEFT_TO_RIGHT:
                cursor.x() += glyph->getHorizontalAdvance() * wr;
                bb.expandBy(lowLeft.x(), lowLeft.y());
                bb.expandBy(upRight.x(), upRight.y());
                break;
              case VERTICAL:
                cursor.y() -= glyph->getVerticalAdvance() * hr;
                bb.expandBy(upLeft.x(), upLeft.y());
                bb.expandBy(lowRight.x(), lowRight.y());
                break;
              case RIGHT_TO_LEFT:
                bb.expandBy(lowRight.x(), lowRight.y());
                bb.expandBy(upLeft.x(), upLeft.y());
                break;
            }
            previous_charcode = charcode;
          }
        }

        // skip over spaces and return.
        while( itr != text.end() && *itr == ' ' )
          ++itr;
        if( itr != text.end() && *itr == '\n' )
          ++itr;
      }
      else
      {
        ++itr;
      }

      // move to new line.
      switch( _layout )
      {
        case LEFT_TO_RIGHT:
        {
          startOfLine_coords.y() -= _characterHeight * (1.0 + _lineSpacing);
          cursor = startOfLine_coords;
          previous_charcode = 0;
          break;
        }
        case RIGHT_TO_LEFT:
        {
          startOfLine_coords.y() -= _characterHeight * (1.0 + _lineSpacing);
          cursor = startOfLine_coords;
          previous_charcode = 0;
          break;
        }
        case VERTICAL:
        {
          startOfLine_coords.x() += _characterHeight * (1.0 + _lineSpacing)
                                  / getCharacterAspectRatio();
          cursor = startOfLine_coords;
          previous_charcode = 0;
          break;
        }
      }
    }

    const_cast<TextOSG*>(this)->_maximumWidth = max_width_safe;

    return bb.height();
  }

  //----------------------------------------------------------------------------
  osg::Vec2 Text::TextOSG::handleHit(const osg::Vec2f& pos)
  {
    float line_height = _characterHeight + _lineSpacing;

    // TODO check with align other than TOP
    float first_line_y = -0.5 * _lineSpacing;//_offset.y() - _characterHeight;
    size_t line = std::max<int>(0, (pos.y() - first_line_y) / line_height);

    if( _textureGlyphQuadMap.empty() )
      return osg::Vec2(-1, -1);

    // TODO check when it can be larger
    assert( _textureGlyphQuadMap.size() == 1 );

    const GlyphQuads& glyphquad = _textureGlyphQuadMap.begin()->second;
    const GlyphQuads::Glyphs& glyphs = glyphquad._glyphs;
    const GlyphQuads::Coords2& coords = glyphquad._coords;
    const GlyphQuads::LineNumbers& line_numbers = glyphquad._lineNumbers;

    const float HIT_FRACTION = 0.6;
    const float character_width = getCharacterHeight()
                                * getCharacterAspectRatio();

    float y = (line + 0.5) * line_height;

    bool line_found = false;
    for(size_t i = 0; i < line_numbers.size(); ++i)
    {
      if( line_numbers[i] != line )
      {
        if( !line_found )
        {
          if( line_numbers[i] < line )
            // Wait for the correct line...
            continue;

          // We have already passed the correct line -> It's empty...
          return osg::Vec2(0, y);
        }

        // Next line and not returned -> not before any character
        // -> return position after last character of line
        return osg::Vec2(coords[(i - 1) * 4 + 2].x(), y);
      }

      line_found = true;

      // Get threshold for mouse x position for setting cursor before or after
      // current character
      float threshold = coords[i * 4].x()
                      + HIT_FRACTION * glyphs[i]->getHorizontalAdvance()
                                     * character_width;

      if( pos.x() <= threshold )
      {
        osg::Vec2 hit(0, y);
        if( i == 0 || line_numbers[i - 1] != line )
          // first character of line
          hit.x() = coords[i * 4].x();
        else if( coords[(i - 1) * 4].x() == coords[(i - 1) * 4 + 2].x() )
          // If previous character width is zero set to begin of next character
          // (Happens eg. with spaces)
          hit.x() = coords[i * 4].x();
        else
          // position at center between characters
          hit.x() = 0.5 * (coords[(i - 1) * 4 + 2].x() + coords[i * 4].x());

        return hit;
      }
    }

    // Nothing found -> return position after last character
    return osg::Vec2
    (
      coords.back().x(),
      (_lineCount - 0.5) * line_height
    );
  }

  //----------------------------------------------------------------------------
  osg::BoundingBox Text::TextOSG::computeBound() const
  {
    osg::BoundingBox bb = osgText::Text::computeBound();

#if OSG_VERSION_LESS_THAN(3,1,0)
    if( bb.valid() )
    {
      // TODO bounding box still doesn't seem always right (eg. with center
      //      horizontal alignment not completely accurate)
      bb._min.y() += _offset.y();
      bb._max.y() += _offset.y();
    }
#endif

    return bb;
  }

  //----------------------------------------------------------------------------
  void Text::TextOSG::computePositions(unsigned int contextID) const
  {
    if( _textureGlyphQuadMap.empty() || _layout == VERTICAL )
      return osgText::Text::computePositions(contextID);

    // TODO check when it can be larger
    assert( _textureGlyphQuadMap.size() == 1 );

    const GlyphQuads& quads = _textureGlyphQuadMap.begin()->second;
    const GlyphQuads::Glyphs& glyphs = quads._glyphs;
    const GlyphQuads::Coords2& coords = quads._coords;
    const GlyphQuads::LineNumbers& line_numbers = quads._lineNumbers;

    float wr = _characterHeight / getCharacterAspectRatio();

    size_t cur_line = static_cast<size_t>(-1);
    for(size_t i = 0; i < glyphs.size(); ++i)
    {
      // Check horizontal offsets

      bool first_char = cur_line != line_numbers[i];
      cur_line = line_numbers[i];

      bool last_char = (i + 1 == glyphs.size())
                    || (cur_line != line_numbers[i + 1]);

      if( first_char || last_char )
      {
        // From osg/src/osgText/Text.cpp:
        //
        // osg::Vec2 upLeft = local+osg::Vec2(0.0f-fHorizQuadMargin, ...);
        // osg::Vec2 lowLeft = local+osg::Vec2(0.0f-fHorizQuadMargin, ...);
        // osg::Vec2 lowRight = local+osg::Vec2(width+fHorizQuadMargin, ...);
        // osg::Vec2 upRight = local+osg::Vec2(width+fHorizQuadMargin, ...);

        float left = coords[i * 4].x(),
              right = coords[i * 4 + 2].x(),
              width = glyphs[i]->getWidth() * wr;

        // (local + width + fHoriz) - (local - fHoriz) = width + 2*fHoriz | -width
        float margin = 0.5f * (right - left - width),
              cursor_x = left + margin
                       - glyphs[i]->getHorizontalBearing().x() * wr;

        if( first_char )
        {
          if( cur_line == 0 || cursor_x < _textBB._min.x() )
            _textBB._min.x() = cursor_x;
        }

        if( last_char )
        {
          float cursor_w = cursor_x + glyphs[i]->getHorizontalAdvance() * wr;

          if( cur_line == 0 || cursor_w > _textBB._max.x() )
            _textBB._max.x() = cursor_w;
        }
      }
    }

    return osgText::Text::computePositions(contextID);
  }

  //----------------------------------------------------------------------------
  const std::string Text::TYPE_NAME = "text";

  //----------------------------------------------------------------------------
  void Text::staticInit()
  {
    if( isInit<Text>() )
      return;

    osg::ref_ptr<TextOSG> Text::*text = &Text::_text;

    addStyle("fill", "color", &TextOSG::setFill, text);
    addStyle("background", "color", &TextOSG::setBackgroundColor, text);
    addStyle("character-size",
             "numeric",
             static_cast<
               void (TextOSG::*)(float)
             > (&TextOSG::setCharacterSize),
             text);
    addStyle("character-aspect-ratio",
             "numeric",
             &TextOSG::setCharacterAspect, text);
    addStyle("line-height", "numeric", &TextOSG::setLineHeight, text);
    addStyle("font-resolution", "numeric", &TextOSG::setFontResolution, text);
    addStyle("padding", "numeric", &TextOSG::setBoundingBoxMargin, text);
    //  TEXT              = 1 default
    //  BOUNDINGBOX       = 2
    //  FILLEDBOUNDINGBOX = 4
    //  ALIGNMENT         = 8
    addStyle<int>("draw-mode", "", &TextOSG::setDrawMode, text);
    addStyle("max-width", "numeric", &TextOSG::setMaximumWidth, text);
    addStyle("font", "", &Text::setFont);
    addStyle("alignment", "", &Text::setAlignment);
    addStyle("text", "", &Text::setText, false);
  }

  //----------------------------------------------------------------------------
  Text::Text( const CanvasWeakPtr& canvas,
              const SGPropertyNode_ptr& node,
              const Style& parent_style,
              Element* parent ):
    Element(canvas, node, parent_style, parent),
    _text( new Text::TextOSG(this) )
  {
    staticInit();

    setDrawable(_text);
    _text->setCharacterSizeMode(osgText::Text::OBJECT_COORDS);
    _text->setAxisAlignment(osgText::Text::USER_DEFINED_ROTATION);
    _text->setRotation(osg::Quat(osg::PI, osg::X_AXIS));

    setupStyle();
  }

  //----------------------------------------------------------------------------
  Text::~Text()
  {

  }

  //----------------------------------------------------------------------------
  void Text::setText(const char* text)
  {
    _text->setText(text, osgText::String::ENCODING_UTF8);
  }

  //----------------------------------------------------------------------------
  void Text::setFont(const char* name)
  {
    _text->setFont( Canvas::getSystemAdapter()->getFont(name) );
  }

  //----------------------------------------------------------------------------
  void Text::setAlignment(const char* align)
  {
    const std::string align_string(align);
    if( 0 ) return;
#define ENUM_MAPPING(enum_val, string_val) \
    else if( align_string == string_val )\
      _text->setAlignment( osgText::Text::enum_val );
#include "text-alignment.hxx"
#undef ENUM_MAPPING
    else
    {
      if( !align_string.empty() )
        SG_LOG
        (
          SG_GENERAL,
          SG_WARN,
          "canvas::Text: unknown alignment '" << align_string << "'"
        );
      _text->setAlignment(osgText::Text::LEFT_BASE_LINE);
    }
  }

  //----------------------------------------------------------------------------
#if 0
  const char* Text::getAlignment() const
  {
    switch( _text->getAlignment() )
    {
#define ENUM_MAPPING(enum_val, string_val) \
      case osgText::Text::enum_val:\
        return string_val;
#include "text-alignment.hxx"
#undef ENUM_MAPPING
      default:
        return "unknown";
    }
  }
#endif

  //----------------------------------------------------------------------------
  int Text::heightForWidth(int w) const
  {
    return _text->heightForWidth(w);
  }

  //----------------------------------------------------------------------------
  osg::Vec2 Text::getNearestCursor(const osg::Vec2& pos) const
  {
    return _text->handleHit(pos);
  }

} // namespace canvas
} // namespace simgear
