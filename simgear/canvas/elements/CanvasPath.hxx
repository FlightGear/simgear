// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2012 Thomas Geymayer <tomgey@gmail.com>

/**
 * @file
 * @brief An OpenVG path on the Canvas
 */

#ifndef CANVAS_PATH_HXX_
#define CANVAS_PATH_HXX_

#include "CanvasElement.hxx"
#include <simgear/math/SGRect.hxx>
#include <initializer_list>

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
            ElementWeakPtr parent = 0 );
      virtual ~Path();

      osg::BoundingBox
      getTransformedBounds(const osg::Matrix& m) const override;

      /** Add a segment with the given command and coordinates */
      Path& addSegment(uint8_t cmd, std::initializer_list<float> coords = {});

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

      void setSVGPath(const std::string& svgPath);

      void setRect(const SGRectf& r);
      void setRoundRect(const SGRectf& r, float radiusX, float radiusY = -1.0);

    protected:
      enum PathAttributes
      {
        CMDS       = LAST_ATTRIBUTE << 1,
        COORDS     = CMDS << 1,
        SVG        = COORDS << 1,
        RECT       = SVG << 1
      };

      class PathDrawable;
      typedef osg::ref_ptr<PathDrawable> PathDrawableRef;
      PathDrawableRef _path;

      bool _hasSVG : 1;
      bool _hasRect : 1;
      SGRectf _rect;

      void updateImpl(double dt) override;

      void childRemoved(SGPropertyNode * child) override;
      void childChanged(SGPropertyNode * child) override;

      void parseRectToVGPath();
  };

} // namespace canvas
} // namespace simgear

#endif /* CANVAS_PATH_HXX_ */
