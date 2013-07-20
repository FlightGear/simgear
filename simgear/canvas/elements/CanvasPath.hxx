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
#include <boost/preprocessor/iteration/iterate.hpp>

namespace simgear
{
namespace canvas
{
  class Path:
    public Element
  {
    public:
      static const std::string TYPE_NAME;
      static void staticInit();

      Path( const CanvasWeakPtr& canvas,
            const SGPropertyNode_ptr& node,
            const Style& parent_style,
            Element* parent = 0 );
      virtual ~Path();

      virtual void update(double dt);

      virtual osg::BoundingBox getTransformedBounds(const osg::Matrix& m) const;

#define BOOST_PP_ITERATION_LIMITS (0, 6)
#define BOOST_PP_FILENAME_1 \
        <simgear/canvas/elements/detail/add_segment_variadic.hxx>
#include BOOST_PP_ITERATE()

      /** Move path cursor */
      Path& moveTo(float x_abs, float y_abs);
      Path& move(float x_rel, float y_rel);

      /** Add a line */
      Path& lineTo(float x_abs, float y_abs);
      Path& line(float x_rel, float y_rel);

      /** Add a horizontal line */
      Path& horizTo(float x_abs);
      Path& horiz(float x_rel);

      /** Add a vertical line */
      Path& vertTo(float y_abs);
      Path& vert(float y_rel);

      /** Close the path (implicit lineTo to first point of path) */
      Path& close();

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
