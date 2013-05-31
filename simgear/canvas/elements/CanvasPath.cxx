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

#include "CanvasPath.hxx"
#include <simgear/scene/util/parse_color.hxx>

#include <osg/Drawable>
#include <osg/BlendFunc>

#include <vg/openvg.h>
#include <cassert>

namespace simgear
{
namespace canvas
{
  typedef std::vector<VGubyte>  CmdList;
  typedef std::vector<VGfloat>  CoordList;


  static const VGubyte shCoordsPerCommand[] = {
    0, /* VG_CLOSE_PATH */
    2, /* VG_MOVE_TO */
    2, /* VG_LINE_TO */
    1, /* VG_HLINE_TO */
    1, /* VG_VLINE_TO */
    4, /* VG_QUAD_TO */
    6, /* VG_CUBIC_TO */
    2, /* VG_SQUAD_TO */
    4, /* VG_SCUBIC_TO */
    5, /* VG_SCCWARC_TO */
    5, /* VG_SCWARC_TO */
    5, /* VG_LCCWARC_TO */
    5  /* VG_LCWARC_TO */
  };
  static const VGubyte shNumCommands = sizeof(shCoordsPerCommand)
                                     / sizeof(shCoordsPerCommand[0]);

  /**
   * Helper to split and convert comma/whitespace separated floating point
   * values
   */
  std::vector<float> splitAndConvert(const char del[], const std::string& str);

  class Path::PathDrawable:
    public osg::Drawable
  {
    public:
      PathDrawable(Path* path):
        _path_element(path),
        _path(VG_INVALID_HANDLE),
        _paint(VG_INVALID_HANDLE),
        _paint_fill(VG_INVALID_HANDLE),
        _attributes_dirty(~0),
        _mode(0),
        _fill_rule(VG_EVEN_ODD),
        _stroke_width(1),
        _stroke_linecap(VG_CAP_BUTT)
      {
        setSupportsDisplayList(false);
        setDataVariance(Object::DYNAMIC);

        setUpdateCallback(new PathUpdateCallback());
      }

      virtual ~PathDrawable()
      {
        if( _path != VG_INVALID_HANDLE )
          vgDestroyPath(_path);
        if( _paint != VG_INVALID_HANDLE )
          vgDestroyPaint(_paint);
        if( _paint_fill != VG_INVALID_HANDLE )
          vgDestroyPaint(_paint_fill);
      }

      virtual const char* className() const { return "PathDrawable"; }
      virtual osg::Object* cloneType() const { return new PathDrawable(_path_element); }
      virtual osg::Object* clone(const osg::CopyOp&) const { return new PathDrawable(_path_element); }

      /**
       * Replace the current path segments with the new ones
       *
       * @param cmds    List of OpenVG path commands
       * @param coords  List of coordinates/parameters used by #cmds
       */
      void setSegments(const CmdList& cmds, const CoordList& coords)
      {
        _cmds = cmds;
        _coords = coords;

        _attributes_dirty |= (PATH | BOUNDING_BOX);
      }

      /**
       * Set path fill paint ("none" if not filled)
       */
      void setFill(const std::string& fill)
      {
        if( fill == "none" )
        {
          _mode &= ~VG_FILL_PATH;
        }
        else if( parseColor(fill, _fill_color) )
        {
          _mode |= VG_FILL_PATH;
          _attributes_dirty |= FILL_COLOR;
        }
        else
        {
          SG_LOG
          (
            SG_GENERAL,
            SG_WARN,
            "canvas::Path Unknown fill: " << fill
          );
        }
      }

      /**
       * Set path fill rule ("pseudo-nonzero" or "evenodd")
       *
       * @warning As the current nonzero implementation causes sever artifacts
       *          for every concave path we call it pseudo-nonzero, so that
       *          everyone is warned that it won't work as expected :)
       */
      void setFillRule(const std::string& fill_rule)
      {
        if( fill_rule == "pseudo-nonzero" )
          _fill_rule = VG_NON_ZERO;
        else // if( fill_rule == "evenodd" )
          _fill_rule = VG_EVEN_ODD;
      }

      /**
       * Set path stroke paint ("none" if no stroke)
       */
      void setStroke(const std::string& stroke)
      {
        if( stroke == "none" )
        {
          _mode &= ~VG_STROKE_PATH;
        }
        else if( parseColor(stroke, _stroke_color) )
        {
          _mode |= VG_STROKE_PATH;
                    _attributes_dirty |= STROKE_COLOR;
        }
        else
        {
          SG_LOG
          (
            SG_GENERAL,
            SG_WARN,
            "canvas::Path Unknown stroke: " << stroke
          );
        }
      }

      /**
       * Set stroke width
       */
      void setStrokeWidth(float width)
      {
        _stroke_width = width;
        _attributes_dirty |= BOUNDING_BOX;
      }

      /**
       * Set stroke dash (line stipple)
       */
      void setStrokeDashArray(const std::string& dash)
      {
        _stroke_dash = splitAndConvert(",\t\n ", dash);
      }

      /**
       * Set stroke-linecap
       *
       * @see http://www.w3.org/TR/SVG/painting.html#StrokeLinecapProperty
       */
      void setStrokeLinecap(const std::string& linecap)
      {
        if( linecap == "round" )
          _stroke_linecap = VG_CAP_ROUND;
        else if( linecap == "square" )
          _stroke_linecap = VG_CAP_SQUARE;
        else
          _stroke_linecap = VG_CAP_BUTT;
      }

      /**
       * Draw callback
       */
      virtual void drawImplementation(osg::RenderInfo& renderInfo) const
      {
        if( _attributes_dirty & PATH )
          return;

        osg::State* state = renderInfo.getState();
        assert(state);

        state->setActiveTextureUnit(0);
        state->setClientActiveTextureUnit(0);
        state->disableAllVertexArrays();

        glPushAttrib(~0u); // Don't use GL_ALL_ATTRIB_BITS as on my machine it
                           // eg. doesn't include GL_MULTISAMPLE_BIT
        glPushClientAttrib(~0u);

        // Initialize/Update the paint
        if( _attributes_dirty & STROKE_COLOR )
        {
          if( _paint == VG_INVALID_HANDLE )
            _paint = vgCreatePaint();

          vgSetParameterfv(_paint, VG_PAINT_COLOR, 4, _stroke_color._v);

          _attributes_dirty &= ~STROKE_COLOR;
        }

        // Initialize/update fill paint
        if( _attributes_dirty & FILL_COLOR )
        {
          if( _paint_fill == VG_INVALID_HANDLE )
            _paint_fill = vgCreatePaint();

          vgSetParameterfv(_paint_fill, VG_PAINT_COLOR, 4, _fill_color._v);

          _attributes_dirty &= ~FILL_COLOR;
        }

        // Setup paint
        if( _mode & VG_STROKE_PATH )
        {
          vgSetPaint(_paint, VG_STROKE_PATH);

          vgSetf(VG_STROKE_LINE_WIDTH, _stroke_width);
          vgSeti(VG_STROKE_CAP_STYLE, _stroke_linecap);
          vgSetfv( VG_STROKE_DASH_PATTERN,
                   _stroke_dash.size(),
                   _stroke_dash.empty() ? 0 : &_stroke_dash[0] );
        }
        if( _mode & VG_FILL_PATH )
        {
          vgSetPaint(_paint_fill, VG_FILL_PATH);

          vgSeti(VG_FILL_RULE, _fill_rule);
        }

        // And finally draw the path
        if( _mode )
          vgDrawPath(_path, _mode);

        VGErrorCode err = vgGetError();
        if( err != VG_NO_ERROR )
          SG_LOG(SG_GL, SG_ALERT, "vgError: " << err);

        glPopAttrib();
        glPopClientAttrib();
      }

      osg::BoundingBox getTransformedBounds(const osg::Matrix& mat) const
      {
        osg::BoundingBox bb;

        osg::Vec2f cur; // VG "Current point" (in local coordinates)
        VGubyte cmd_index = 0;
        for( size_t i = 0,              ci = 0;
                    i < _cmds.size() && ci < _coords.size();
                  ++i,                  ci += shCoordsPerCommand[cmd_index] )
        {
          VGubyte rel = _cmds[i] & 1,
                  cmd = _cmds[i] & ~1;

          cmd_index = cmd / 2;
          if( cmd_index >= shNumCommands )
            return osg::BoundingBox();

          const VGubyte max_coords = 3;
          osg::Vec2f points[max_coords];
          VGubyte num_coords = 0;

          switch( cmd )
          {
            case VG_CLOSE_PATH:
              break;
            case VG_MOVE_TO:
            case VG_LINE_TO:
            case VG_SQUAD_TO:
              // x0, y0
              points[ num_coords++ ].set(_coords[ci], _coords[ci + 1]);
              break;
            case VG_HLINE_TO:
              // x0
              points[ num_coords++ ].set( _coords[ci] + (rel ? cur.x() : 0),
                                          cur.y() );
              // We have handled rel/abs already, so no need to do it again...
              rel = 0;
              break;
            case VG_VLINE_TO:
              // y0
              points[ num_coords++ ].set( cur.x(),
                                          _coords[ci] + (rel ? cur.y() : 0) );
              // We have handled rel/abs already, so no need to do it again...
              rel = 0;
              break;
            case VG_QUAD_TO:
            case VG_SCUBIC_TO:
              // x0,y0,x1,y1
              points[ num_coords++ ].set(_coords[ci    ], _coords[ci + 1]);
              points[ num_coords++ ].set(_coords[ci + 2], _coords[ci + 3]);
              break;
            case VG_CUBIC_TO:
              // x0,y0,x1,y1,x2,y2
              points[ num_coords++ ].set(_coords[ci    ], _coords[ci + 1]);
              points[ num_coords++ ].set(_coords[ci + 2], _coords[ci + 3]);
              points[ num_coords++ ].set(_coords[ci + 4], _coords[ci + 5]);
              break;
            case VG_SCCWARC_TO:
            case VG_SCWARC_TO:
            case VG_LCCWARC_TO:
            case VG_LCWARC_TO:
              // rh,rv,rot,x0,y0
              points[ num_coords++ ].set(_coords[ci + 3], _coords[ci + 4]);
              break;
            default:
              SG_LOG(SG_GL, SG_WARN, "Unknown VG command: " << (int)cmd);
              return osg::BoundingBox();
          }

          assert(num_coords <= max_coords);
          for(VGubyte i = 0; i < num_coords; ++i)
          {
            if( rel )
              points[i] += cur;

            bb.expandBy( transformPoint(mat, points[i]) );
          }

          if( num_coords > 0 )
            cur = points[ num_coords - 1 ];
        }

        return bb;
      }

      /**
       * Compute the bounding box
       */
      virtual osg::BoundingBox computeBound() const
      {
        if( _path == VG_INVALID_HANDLE || (_attributes_dirty & PATH) )
          return osg::BoundingBox();

        VGfloat min[2], size[2];
        vgPathBounds(_path, &min[0], &min[1], &size[0], &size[1]);

        _attributes_dirty &= ~BOUNDING_BOX;

        // vgPathBounds doesn't take stroke width into account
        float ext = 0.5 * _stroke_width;

        osg::BoundingBox bb
        (
          min[0] - ext,           min[1] - ext,           -0.1,
          min[0] + size[0] + ext, min[1] + size[1] + ext,  0.1
        );
        _path_element->setBoundingBox(bb);

        return bb;
      }

    private:

      enum Attributes
      {
        PATH            = 0x0001,
        STROKE_COLOR    = PATH << 1,
        FILL_COLOR      = STROKE_COLOR << 1,
        BOUNDING_BOX    = FILL_COLOR << 1
      };

      Path *_path_element;

      mutable VGPath    _path;
      mutable VGPaint   _paint;
      mutable VGPaint   _paint_fill;
      mutable uint32_t  _attributes_dirty;

      CmdList   _cmds;
      CoordList _coords;

      VGbitfield            _mode;
      osg::Vec4f            _fill_color;
      VGFillRule            _fill_rule;
      osg::Vec4f            _stroke_color;
      VGfloat               _stroke_width;
      std::vector<VGfloat>  _stroke_dash;
      VGCapStyle            _stroke_linecap;

      osg::Vec3f transformPoint( const osg::Matrix& m,
                                 osg::Vec2f pos ) const
      {
        return osg::Vec3
        (
          m(0, 0) * pos[0] + m(1, 0) * pos[1] + m(3, 0),
          m(0, 1) * pos[0] + m(1, 1) * pos[1] + m(3, 1),
          0
        );
      }

      /**
       * Initialize/Update the OpenVG path
       */
      void update()
      {
        if( !vgHasContextSH() )
          return;

        if( _attributes_dirty & PATH )
        {
          const VGbitfield caps = VG_PATH_CAPABILITY_APPEND_TO
                                | VG_PATH_CAPABILITY_MODIFY
                                | VG_PATH_CAPABILITY_PATH_BOUNDS;

          if( _path == VG_INVALID_HANDLE )
            _path = vgCreatePath(
              VG_PATH_FORMAT_STANDARD,
              VG_PATH_DATATYPE_F,
              1.f, 0.f, // scale,bias
              _cmds.size(), _coords.size(),
              caps
            );
          else
            vgClearPath(_path, caps);

          if( !_cmds.empty() && !_coords.empty() )
            vgAppendPathData(_path, _cmds.size(), &_cmds[0], &_coords[0]);

          _attributes_dirty &= ~PATH;
          _attributes_dirty |= BOUNDING_BOX;
        }

        if( _attributes_dirty & BOUNDING_BOX )
          dirtyBound();
      }

      struct PathUpdateCallback:
        public osg::Drawable::UpdateCallback
      {
        virtual void update(osg::NodeVisitor*, osg::Drawable* drawable)
        {
          static_cast<PathDrawable*>(drawable)->update();
        }
      };
  };

  //----------------------------------------------------------------------------
  Path::Path( const CanvasWeakPtr& canvas,
              const SGPropertyNode_ptr& node,
              const Style& parent_style,
              Element* parent ):
    Element(canvas, node, parent_style, parent),
    _path( new PathDrawable(this) )
  {
    setDrawable(_path);

    if( !isInit<Path>() )
    {
      PathDrawableRef Path::*path = &Path::_path;

      addStyle("fill", "color", &PathDrawable::setFill, path);
      addStyle("fill-rule", "", &PathDrawable::setFillRule, path);
      addStyle("stroke", "color", &PathDrawable::setStroke, path);
      addStyle("stroke-width", "numeric", &PathDrawable::setStrokeWidth, path);
      addStyle("stroke-dasharray", "", &PathDrawable::setStrokeDashArray, path);
      addStyle("stroke-linecap", "", &PathDrawable::setStrokeLinecap, path);
    }

    setupStyle();
  }

  //----------------------------------------------------------------------------
  Path::~Path()
  {

  }

  //----------------------------------------------------------------------------
  void Path::update(double dt)
  {
    if( _attributes_dirty & (CMDS | COORDS) )
    {
      _path->setSegments
      (
        _node->getChildValues<VGubyte, int>("cmd"),
        _node->getChildValues<VGfloat, float>("coord")
      );

      _attributes_dirty &= ~(CMDS | COORDS);
    }

    Element::update(dt);
  }

  //----------------------------------------------------------------------------
  osg::BoundingBox Path::getTransformedBounds(const osg::Matrix& m) const
  {
    return _path->getTransformedBounds(m);
  }

  //----------------------------------------------------------------------------
  Path& Path::moveTo(float x_abs, float y_abs)
  {
    return addSegment(VG_MOVE_TO_ABS, x_abs, y_abs);
  }

  //----------------------------------------------------------------------------
  Path& Path::move(float x_rel, float y_rel)
  {
    return addSegment(VG_MOVE_TO_REL, x_rel, y_rel);
  }

  //----------------------------------------------------------------------------
  Path& Path::lineTo(float x_abs, float y_abs)
  {
    return addSegment(VG_LINE_TO_ABS, x_abs, y_abs);
  }

  //----------------------------------------------------------------------------
  Path& Path::line(float x_rel, float y_rel)
  {
    return addSegment(VG_LINE_TO_REL, x_rel, y_rel);
  }

  //----------------------------------------------------------------------------
  Path& Path::horizTo(float x_abs)
  {
    return addSegment(VG_HLINE_TO_ABS, x_abs);
  }

  //----------------------------------------------------------------------------
  Path& Path::horiz(float x_rel)
  {
    return addSegment(VG_HLINE_TO_REL, x_rel);
  }

  //----------------------------------------------------------------------------
  Path& Path::vertTo(float y_abs)
  {
    return addSegment(VG_VLINE_TO_ABS, y_abs);
  }

  //----------------------------------------------------------------------------
  Path& Path::vert(float y_rel)
  {
    return addSegment(VG_VLINE_TO_REL, y_rel);
  }

  //----------------------------------------------------------------------------
  Path& Path::close()
  {
    return addSegment(VG_CLOSE_PATH);
  }

  //----------------------------------------------------------------------------
  void Path::childRemoved(SGPropertyNode* child)
  {
    childChanged(child);
  }

  //----------------------------------------------------------------------------
  void Path::childChanged(SGPropertyNode* child)
  {
    if( child->getParent() != _node )
      return;

    if( child->getNameString() == "cmd" )
      _attributes_dirty |= CMDS;
    else if( child->getNameString() == "coord" )
      _attributes_dirty |= COORDS;
  }

  //----------------------------------------------------------------------------
  std::vector<float> splitAndConvert(const char del[], const std::string& str)
  {
    std::vector<float> values;
    size_t pos = 0;
    for(;;)
    {
      pos = str.find_first_not_of(del, pos);
      if( pos == std::string::npos )
        break;

      char *end = 0;
      float val = strtod(&str[pos], &end);
      if( end == &str[pos] || !end )
        break;

      values.push_back(val);
      pos = end - &str[0];
    }
    return values;
  }

} // namespace canvas
} // namespace simgear
