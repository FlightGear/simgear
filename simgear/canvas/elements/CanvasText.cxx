// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2012 Thomas Geymayer <tomgey@gmail.com>

/**
 * @file
 * @brief Text on the Canvas
 */

#include <simgear_config.h>

#include "CanvasText.hxx"

#include <sstream>
#include <iomanip>

#include <simgear/canvas/Canvas.hxx>
#include <simgear/canvas/CanvasSystemAdapter.hxx>
#include <simgear/scene/util/LoadShader.hxx>
#include <simgear/scene/util/parse_color.hxx>
#include <simgear/scene/util/SGProgram.hxx>
#include <osg/Version>
#include <osgDB/Registry>
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
      void setStroke(const std::string& color);
      void setBackgroundColor(const std::string& fill);

      float lineHeight() const;

      /// Get the number of lines
      size_t lineCount() const;

      /// Get line @a i
      TextLine lineAt(size_t i) const;

      /// Get nearest line to given y-coordinate
      TextLine nearestLine(float pos_y);
      SGVec2i sizeForWidth(int w);

      osg::BoundingBox computeBoundingBox() const override;

    protected:
      friend class TextLine;

      canvas::Text *_text_element;

      osg::StateSet *createStateSet() override;
      void computePositionsImplementation() override;
};

  class TextLine
  {
    public:
      TextLine();
      TextLine(size_t line, Text::TextOSG const* text);

      /// Number of characters on this line
      size_t size() const;
      bool empty() const;

      osg::Vec2 cursorPos(size_t i) const;
      osg::Vec2i nearestCursor(float x) const;

    protected:
      typedef Text::TextOSG::GlyphQuads GlyphQuads;

      Text::TextOSG const *_text;
      GlyphQuads const    *_quads;

      size_t _line,
             _begin,
             _end;
  };

  //----------------------------------------------------------------------------
  TextLine::TextLine():
    _text(NULL),
    _quads(NULL),
    _line(0),
    _begin(-1),
    _end(-1)
  {

  }

  //----------------------------------------------------------------------------
  TextLine::TextLine(size_t line, Text::TextOSG const* text):
    _text(text),
    _quads(NULL),
    _line(line),
    _begin(-1),
    _end(-1)
  {
    if( !text || text->_textureGlyphQuadMap.empty() || !_text->lineCount() )
      return;

    _quads = &text->_textureGlyphQuadMap.begin()->second;

    auto refCoords = _text->getCoords();
    osgText::TextBase::Coords::element_type &coords = *refCoords.get();
    const auto lineHeight = text->getCharacterHeight() + _text->getLineSpacing();

    bool foundBegin = false;
    for (size_t i = 0; i < coords.size(); i += 4) {
      const auto lineIndex = static_cast<size_t>(floorf(coords[i].y() / lineHeight));
      if (lineIndex == _line) {
        if (!foundBegin) {
          _begin = _end = i / 4;
          foundBegin = true;
        } else {
          _end = i / 4;
        }
      } else if (foundBegin) {
        // next line, we're done
          break;
      }
    }
  }

  //----------------------------------------------------------------------------
  size_t TextLine::size() const
  {
    return _end - _begin;
  }

  //----------------------------------------------------------------------------
  bool TextLine::empty() const
  {
    return _end == _begin;
  }

  //----------------------------------------------------------------------------
  osg::Vec2 TextLine::cursorPos(size_t i) const
  {
    if( i > size() )
      // Position after last character if out of range (TODO better exception?)
      i = size();

    osg::Vec2 pos(0, _text->_offset.y() + _line * _text->lineHeight());

    if( empty() ) {
      return pos;
    }

      osgText::TextBase::Coords refCoords = _text->getCoords();
      osgText::TextBase::Coords::element_type &coords = *refCoords.get();

      size_t global_i = _begin + i;

      if (global_i == _begin) {
          // before first character of line
          pos.x() = coords[_begin * 4].x();
      } else if (global_i == _end) {
          // After Last character of line
          pos.x() = coords[(_end - 1) * 4 + 2].x();
    } else {
          float prev_l = coords[(global_i - 1) * 4].x(),
              prev_r = coords[(global_i - 1) * 4 + 2].x(),
              cur_l = coords[global_i * 4].x();

          if (prev_l == prev_r)
              // If previous character width is zero set to begin of next character
              // (Happens eg. with spaces)
              pos.x() = cur_l;
          else
              // position at center between characters
              pos.x() = 0.5 * (prev_r + cur_l);
      }
    return pos;
  }

  //----------------------------------------------------------------------------
  osg::Vec2i TextLine::nearestCursor(float x) const
  {
    if (empty())
      return {static_cast<int>(_line), 0};

    osgText::TextBase::Coords refCoords = _text->getCoords();
    osgText::TextBase::Coords::element_type &coords = *refCoords.get();

    GlyphQuads::Glyphs const& glyphs = _quads->_glyphs;

    float const HIT_FRACTION = 0.6;
    float const character_width = _text->getCharacterHeight()
                                * _text->getCharacterAspectRatio();

    size_t i = _begin;
    for(; i < _end; ++i)
    {
      // Get threshold for mouse x position for setting cursor before or after
      // current character
      float threshold = coords[i * 4].x()
                      + HIT_FRACTION * glyphs[i]->getHorizontalAdvance()
                                     * character_width;

      if( x <= threshold )
        break;
    }

    return {static_cast<int>(_line), 
      static_cast<int>(i - _begin)};
  }

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
  void Text::TextOSG::setStroke(const std::string& stroke)
  {
    osg::Vec4 color;
    if( stroke == "none" || !parseColor(stroke, color) )
      setBackdropType(NONE);
    else
    {
      setBackdropType(OUTLINE);
      setBackdropColor(color);
    }
  }

  //----------------------------------------------------------------------------
  void Text::TextOSG::setBackgroundColor(const std::string& fill)
  {
    osg::Vec4 color;
    if( parseColor(fill, color) )
      setBoundingBoxColor( color );
  }

  //----------------------------------------------------------------------------
  float Text::TextOSG::lineHeight() const
  {
    return (1 + _lineSpacing) * _characterHeight;
  }

  //----------------------------------------------------------------------------
  size_t Text::TextOSG::lineCount() const
  {
    return _lineCount;
  }

  //----------------------------------------------------------------------------
  TextLine Text::TextOSG::lineAt(size_t i) const
  {
    return TextLine(i, this);
  }

  //----------------------------------------------------------------------------
  TextLine Text::TextOSG::nearestLine(float pos_y)
  {
    auto font = getActiveFont();

    if( !font || lineCount() <= 0 )
      return TextLine(0, this);

    float asc = .9f, desc = -.2f;
    font->getVerticalSize(asc, desc);

    float first_line_y = _offset.y()
                       - (1 + _lineSpacing / 2 + desc) * _characterHeight;

    size_t line_num = std::min<size_t>(
      std::max<size_t>(0, (pos_y - first_line_y) / lineHeight()),
      lineCount() - 1
    );

    return TextLine(line_num, this);
  }

  //----------------------------------------------------------------------------
  // simplified version of osgText::Text::computeGlyphRepresentation() to
  // just calculate the size for a given weight. Glpyh calculations/creating
  // is not necessary for this...
  SGVec2i Text::TextOSG::sizeForWidth(int w)
  {
    if( _text.empty() )
      return SGVec2i(0, 0);

    auto activefont = getActiveFont();

    if( !activefont )
      return SGVec2i(-1, -1);

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
                  osg::Vec2 delta(activefont->getKerning(_fontSize,
                      previous_charcode,
                      charcode,
                      _kerningType));
                  cursor.x() += delta.x() * wr;
                  cursor.y() += delta.y() * hr;
                  break;
                }
                case RIGHT_TO_LEFT:
                {
                  osg::Vec2 delta(activefont->getKerning(_fontSize, charcode,
                      previous_charcode,
                      _kerningType));
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

    return bb.size();
  }

  //----------------------------------------------------------------------------
  osg::BoundingBox Text::TextOSG::computeBoundingBox() const
  {
    osg::BoundingBox bb = osgText::Text::computeBoundingBox();
    return bb;
  }

  //----------------------------------------------------------------------------
  void Text::TextOSG::computePositionsImplementation()
  {
    TextBase::computePositionsImplementation();
  }

  //----------------------------------------------------------------------------
  osg::StateSet *Text::TextOSG::createStateSet()
  {
    osgText::Font *activeFont = getActiveFont();
    if (!activeFont) {
      return 0;
    }
    osgText::Font::StateSets &statesets = activeFont->getCachedStateSets();

    std::stringstream ss;
    ss.imbue(std::locale::classic());
    ss << std::fixed << std::setprecision(3);

    osg::StateSet::DefineList defineList;
    if (_backdropType != NONE) {
      ss.str("");
      ss << "vec4(" << _backdropColor.r()
         << ", "    << _backdropColor.g()
         << ", "    << _backdropColor.b()
         << ", "    << _backdropColor.a()
         << ")";
      defineList["BACKDROP_COLOR"] = osg::StateSet::DefinePair(ss.str(), osg::StateAttribute::ON);

      if (_backdropType == OUTLINE) {
        ss.str("");
        ss << _backdropHorizontalOffset;
        defineList["OUTLINE"] = osg::StateSet::DefinePair(ss.str(), osg::StateAttribute::ON);
      } else {
        osg::Vec2f offset(_backdropHorizontalOffset, _backdropVerticalOffset);
        switch(_backdropType) {
        case(DROP_SHADOW_BOTTOM_RIGHT) :    offset.set(_backdropHorizontalOffset, -_backdropVerticalOffset);  break;
        case(DROP_SHADOW_CENTER_RIGHT) :    offset.set(_backdropHorizontalOffset, 0.0f);                      break;
        case(DROP_SHADOW_TOP_RIGHT) :       offset.set(_backdropHorizontalOffset, _backdropVerticalOffset);   break;
        case(DROP_SHADOW_BOTTOM_CENTER) :   offset.set(0.0f, -_backdropVerticalOffset);                       break;
        case(DROP_SHADOW_TOP_CENTER) :      offset.set(0.0f, _backdropVerticalOffset);                        break;
        case(DROP_SHADOW_BOTTOM_LEFT) :     offset.set(-_backdropHorizontalOffset, -_backdropVerticalOffset); break;
        case(DROP_SHADOW_CENTER_LEFT) :     offset.set(-_backdropHorizontalOffset, 0.0f);                     break;
        case(DROP_SHADOW_TOP_LEFT) :        offset.set(-_backdropHorizontalOffset, _backdropVerticalOffset);  break;
        default: break;
        }
        ss.str("");
        ss << "vec2(" << offset.x() << ", " << offset.y() << ")";
        defineList["SHADOW"] = osg::StateSet::DefinePair(ss.str(), osg::StateAttribute::ON);
      }
    }

    {
      ss<<std::fixed<<std::setprecision(1);
      ss.str("");
      ss << float(_fontSize.second);
      defineList["GLYPH_DIMENSION"] = osg::StateSet::DefinePair(ss.str(), osg::StateAttribute::ON);
      ss.str("");
      ss << float(activeFont->getTextureWidthHint());
      defineList["TEXTURE_DIMENSION"] = osg::StateSet::DefinePair(ss.str(), osg::StateAttribute::ON);
    }

    if (_shaderTechnique > osgText::GREYSCALE) {
      defineList["SIGNED_DISTANCE_FIELD"] = osg::StateSet::DefinePair("1", osg::StateAttribute::ON);
    }

    // Return a cached stateset if it already exists
    if (!statesets.empty()) {
      for (auto itr = statesets.begin(); itr != statesets.end(); ++itr) {
        if ((*itr)->getDefineList() == defineList) {
          return itr->get();
        }
      }
    }

    // No matching cached stateset, create one from scratch
    osg::ref_ptr<osg::StateSet> stateSet = new osg::StateSet;
    stateSet->setDefineList(defineList);
    // Cache it
    statesets.push_back(stateSet.get());

    stateSet->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
    stateSet->setMode(GL_BLEND, osg::StateAttribute::ON);

    stateSet->addUniform(new osg::Uniform("glyphTexture", 0));

    auto program = new SGProgram;
    auto vs = new osg::Shader(osg::Shader::VERTEX);
    if (loadShaderFromDataFile(vs, "Shaders/Canvas/text.vert")) {
      program->addShader(vs);
    } else {
      SG_LOG(SG_GL, SG_ALERT, "canvas::Text: Failed to load vertex shader");
    }
    auto fs = new osg::Shader(osg::Shader::FRAGMENT);
    if (loadShaderFromDataFile(fs, "Shaders/Canvas/text.frag")) {
      program->addShader(fs);
    } else {
      SG_LOG(SG_GL, SG_ALERT, "canvas::Text: Failed to load fragment shader");
    }
    stateSet->setAttributeAndModes(program);

    return stateSet.release();
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
    addStyle("stroke", "color", &TextOSG::setStroke, text);
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

    osgDB::Registry* reg = osgDB::Registry::instance();
    if( !reg->getReaderWriterForExtension("ttf") )
      SG_LOG(SG_GL, SG_ALERT, "canvas::Text: Missing 'ttf' font reader");
  }

  //----------------------------------------------------------------------------
  Text::Text( const CanvasWeakPtr& canvas,
              const SGPropertyNode_ptr& node,
              const Style& parent_style,
              ElementWeakPtr parent ):
    Element(canvas, node, parent_style, parent),
    _text( new Text::TextOSG(this) )
  {
    staticInit();

    setDrawable(_text);
    _text->setDataVariance(osg::Object::DYNAMIC);
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
  void Text::setText(const std::string &text)
  {
    _text->setText(text, osgText::String::ENCODING_UTF8);
  }

  //----------------------------------------------------------------------------
  void Text::setFont(const std::string& name)
  {
    _text->setFont( Canvas::getSystemAdapter()->getFont(name) );
  }

  //----------------------------------------------------------------------------
  void Text::setAlignment(const std::string& align_string)
  {
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
    return _text->sizeForWidth(w).y();
  }

  //----------------------------------------------------------------------------
  int Text::maxWidth() const
  {
    return _text->sizeForWidth(INT_MAX).x();
  }

  //----------------------------------------------------------------------------
  size_t Text::lineCount() const
  {
    return _text->lineCount();
  }

  //----------------------------------------------------------------------------
  size_t Text::lineLength(size_t line) const
  {
    return _text->lineAt(line).size();
  }

  //----------------------------------------------------------------------------
  osg::Vec2i Text::getNearestCursor(const osg::Vec2& pos) const
  {
    return _text->nearestLine(pos.y()).nearestCursor(pos.x());
  }

  //----------------------------------------------------------------------------
  osg::Vec2 Text::getCursorPos(size_t line, size_t character) const
  {
    return _text->lineAt(line).cursorPos(character);
  }

  //----------------------------------------------------------------------------
  osg::StateSet* Text::getOrCreateStateSet()
  {
    if( !_scene_group.valid() )
      return nullptr;

    // Only check for StateSet on Transform, as the text stateset is shared
    // between all text instances using the same font (texture).
    return _scene_group->getOrCreateStateSet();
  }

} // namespace canvas
} // namespace simgear
