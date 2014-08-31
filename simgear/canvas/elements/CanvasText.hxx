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
            Element* parent = 0 );
      ~Text();

      void setText(const char* text);
      void setFont(const char* name);
      void setAlignment(const char* align);

      int heightForWidth(int w) const;
      int maxWidth() const;

      /// Number of text lines.
      size_t lineCount() const;

      /// Number of characters in @a line.
      size_t lineLength(size_t line) const;

      osg::Vec2 getNearestCursor(const osg::Vec2& pos) const;
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
