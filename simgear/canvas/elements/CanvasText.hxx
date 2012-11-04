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

  class Text:
    public Element
  {
    public:
      Text( const CanvasWeakPtr& canvas,
            SGPropertyNode_ptr node,
            const Style& parent_style );
      ~Text();

      void setText(const char* text);
      void setFont(const char* name);
      void setAlignment(const char* align);

    protected:

      class TextOSG;
      osg::ref_ptr<TextOSG> _text;

      virtual void childChanged(SGPropertyNode * child);

      void handleHit(float x, float y);

  };

} // namespace canvas
} // namespace simgear

#endif /* CANVAS_TEXT_HXX_ */
