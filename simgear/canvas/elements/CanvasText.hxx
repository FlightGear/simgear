// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2012 Thomas Geymayer <tomgey@gmail.com>

/**
 * @file
 * @brief Text on the Canvas
 */

#ifndef CANVAS_TEXT_HXX_
#define CANVAS_TEXT_HXX_

#include "CanvasElement.hxx"
#include <osgText/Text>
#include <map>
#include <vector>

namespace simgear
{
namespace canvas
{

  class TextLine;
  class Text:
    public Element
  {
    public:
      static const std::string TYPE_NAME;
      static void staticInit();

      Text( const CanvasWeakPtr& canvas,
            const SGPropertyNode_ptr& node,
            const Style& parent_style,
            ElementWeakPtr parent = 0 );
      virtual ~Text();

      void setText(const std::string& text);
      void setFont(const std::string& name);
      void setAlignment(const std::string& align_string);

      int heightForWidth(int w) const;
      int maxWidth() const;

      /// Number of text lines.
      size_t lineCount() const;

      /// Number of characters in @a line.
      size_t lineLength(size_t line) const;

      /**
       * @brief map a pixel location to a line,char position
       * Rounding is applied to make this work 'as expected' for
       * clicking on text, i.e clicks closer to the right edge
       * return the character to the right.
      */
      osg::Vec2i getNearestCursor(const osg::Vec2& pos) const;
      
      /**
       * @brief Map line,char location to the top-left of the
       * glyph's box.
       * 
       * @param line 
       * @param character 
       * @return osg::Vec2 : top-left of the glyph box in pixels
       */
      osg::Vec2 getCursorPos(size_t line, size_t character) const;

    protected:

      friend class TextLine;
      class TextOSG;
      osg::ref_ptr<TextOSG> _text;

      virtual osg::StateSet* getOrCreateStateSet();

  };

} // namespace canvas
} // namespace simgear

#endif /* CANVAS_TEXT_HXX_ */
