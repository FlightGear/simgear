// An OpenVG path on the Canvas
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

#ifndef CANVAS_PATH_HXX_
#define CANVAS_PATH_HXX_

#include "CanvasElement.hxx"

namespace simgear
{
namespace canvas
{
  class Path:
    public Element
  {
    public:
      Path( const CanvasWeakPtr& canvas,
            const SGPropertyNode_ptr& node,
            const Style& parent_style,
            Element* parent = 0 );
      virtual ~Path();

      virtual void update(double dt);

      virtual osg::BoundingBox getTransformedBounds(const osg::Matrix& m) const;

    protected:

      enum PathAttributes
      {
        CMDS       = LAST_ATTRIBUTE << 1,
        COORDS     = CMDS << 1
      };

      class PathDrawable;
      typedef osg::ref_ptr<PathDrawable> PathDrawableRef;
      PathDrawableRef _path;

      virtual void childRemoved(SGPropertyNode * child);
      virtual void childChanged(SGPropertyNode * child);
  };

} // namespace canvas
} // namespace simgear

#endif /* CANVAS_PATH_HXX_ */
