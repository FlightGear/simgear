///@file
/// An OpenVG path on the Canvas
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

#include <simgear_config.h>

#include "CanvasPath.hxx"
#include <simgear/scene/util/parse_color.hxx>
#include <simgear/misc/strutils.hxx>

#include <osg/Drawable>

#include <vg/openvg.h>
#include <cassert>
#include <cctype>

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

    static float parseCSSNumber(const std::string& s)
    {
        if (strutils::ends_with(s, "px")) {
            return std::stof(s.substr(0, s.length() - 2));
        } else if (s.back() == '%') {
            float f = std::stof(s.substr(0, s.length() - 1));
            return f / 100.0f;
        }
        return std::stof(s);
    }

    //----------------------------------------------------------------------------
    static bool parseSVGPathToVGPath(const std::string& svgPath, CmdList& commands, CoordList& coords)
    {
        const string_list& tokens = simgear::strutils::split_on_any_of(svgPath, "\t \n\r,");
        char activeSVGCommand = 0;
        bool isRelative = false;
        int tokensNeeded = 0;

        for (auto it = tokens.begin(); it != tokens.end(); ) {
            // set up the new command data
            if ((it->size() == 1) && std::isalpha(it->at(0))) {
                const char svgCommand = std::toupper(it->at(0));
                isRelative = std::islower(it->at(0));
                switch (svgCommand) {
                case 'Z':
                        tokensNeeded = 0;
                        break;
                case 'M':
                case 'L':
                case 'T':
                        tokensNeeded = 2;
                        break;
                case 'H':
                case 'V':
                        tokensNeeded = 1;
                        break;
                case 'C':
                        tokensNeeded = 6;
                        break;
                case 'S':
                case 'Q':
                        tokensNeeded = 4;
                        break;
                case 'A':
                        tokensNeeded = 7;
                        break;
                default:
                    SG_LOG(SG_GENERAL, SG_WARN, "unrecognized SVG path command: "
                           << *it << " at token " <<  std::distance(tokens.begin(), it));
                    return false;
                }

                activeSVGCommand = svgCommand;
                ++it; // advance to first coordinate token
            }

            const int numTokensRemaining = std::distance(it, tokens.end());
            if (numTokensRemaining < tokensNeeded) {
                SG_LOG(SG_GENERAL, SG_WARN, "insufficent SVG path tokens");
                return false;
            }

            bool pushTokensDirectly = true;
            if (activeSVGCommand == 'Z') {
                commands.push_back(VG_CLOSE_PATH);
                activeSVGCommand = 0;
            } else if (activeSVGCommand == 'M') {
                commands.push_back(VG_MOVE_TO | isRelative);
                activeSVGCommand = 'L';
            } else if (activeSVGCommand == 'L') {
                commands.push_back(VG_LINE_TO | isRelative);
            } else if (activeSVGCommand == 'H') {
                commands.push_back(VG_HLINE_TO | isRelative);
            } else if (activeSVGCommand == 'V') {
                commands.push_back(VG_HLINE_TO | isRelative);
            } else if (activeSVGCommand == 'C') {
                commands.push_back(VG_CUBIC_TO | isRelative);
            } else if (activeSVGCommand == 'S') {
                commands.push_back(VG_SCUBIC_TO | isRelative);
            } else if (activeSVGCommand == 'Q') {
                commands.push_back(VG_SCUBIC_TO | isRelative);
            } else if (activeSVGCommand == 'T') {
                commands.push_back(VG_SCUBIC_TO | isRelative);
            } else if (activeSVGCommand == 'A') {
                pushTokensDirectly = false; // deal with tokens manually
                coords.push_back(parseCSSNumber(*it++)); // rx
                coords.push_back(parseCSSNumber(*it++)); // ry
                coords.push_back(parseCSSNumber(*it++)); // x-axis rotation

                const bool isLargeArc = std::stoi(*it++); // large-angle
                const bool isCCW = std::stoi(*it++); // sweep-flag

                int vgCmd = isLargeArc ? (isCCW ? VG_LCCWARC_TO : VG_LCWARC_TO) :
                    (isCCW ? VG_SCCWARC_TO : VG_SCWARC_TO);

                coords.push_back(parseCSSNumber(*it++));
                coords.push_back(parseCSSNumber(*it++));
                commands.push_back(vgCmd | isRelative);
            } else {
                SG_LOG(SG_GENERAL, SG_WARN, "malformed SVG path string: expected a command at token:"
                       << std::distance(tokens.begin(), it) << " :" << *it);
                return false;
            }

            if (pushTokensDirectly) {
                for (int i=0; i<tokensNeeded;++i) {
                    coords.push_back(parseCSSNumber(*it++));
                }
            }
        } // of tokens iteration

        return true;
    }


    //---------------------------------------------------------------------------

    static SGVec2f parseRectCornerRadius(SGPropertyNode* node, const std::string& xDir, const std::string& yDir, bool& haveCorner)
    {
        haveCorner = false;
        std::string propName = "border-" + yDir + "-" + xDir + "-radius";
        if (!node->hasChild(propName)) {
            propName = "border-" + yDir + "-radius";
            if (!node->hasChild(propName)) {
                propName = "border-radius";
            }
        }

        PropertyList props = node->getChildren(propName);
        if (props.size() == 1) {
            double r = props.at(0)->getDoubleValue(propName);
            haveCorner = true;
            return SGVec2f(r, r);
        }

        if (props.size() >= 2 ) {
            haveCorner = true;
            return SGVec2f(props.at(0)->getDoubleValue(),
                           props.at(1)->getDoubleValue());
        }

        return SGVec2f(-1.0f, -1.0f);
    }

  //----------------------------------------------------------------------------
  class Path::PathDrawable:
    public osg::Drawable
  {
    public:
      PathDrawable(Path* path):
        _path_element(path)
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

      const char* className() const override
      { return "PathDrawable"; }
      osg::Object* cloneType() const override
      { return new PathDrawable(_path_element); }
      osg::Object* clone(const osg::CopyOp&) const override
      { return new PathDrawable(_path_element); }

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
        if( fill.empty() || fill == "none" )
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
       * Set path fill opacity (Only used if fill is not "none")
       */
      void setFillOpacity(float opacity)
      {
        _fill_opacity =
          static_cast<uint8_t>(SGMiscf::clip(opacity, 0.f, 1.f) * 255);
        _attributes_dirty |= FILL_COLOR;
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
        if( stroke.empty() || stroke == "none" )
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
       * Set path stroke opacity (only used if stroke is not "none")
       */
      void setStrokeOpacity(float opacity)
      {
        _stroke_opacity =
          static_cast<uint8_t>(SGMiscf::clip(opacity, 0.f, 1.f) * 255);
        _attributes_dirty |= STROKE_COLOR;
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
       * Set stroke-linejoin
       *
       * @see http://www.w3.org/TR/SVG/painting.html#StrokeLinejoinProperty
       */
      void setStrokeLinejoin(const std::string& linejoin)
      {
        if( linejoin == "round" )
          _stroke_linejoin = VG_JOIN_ROUND;
        else if( linejoin == "bevel" )
          _stroke_linejoin = VG_JOIN_BEVEL;
        else
          _stroke_linejoin = VG_JOIN_MITER;
      }

      /**
       * Draw callback
       */
      void drawImplementation(osg::RenderInfo& renderInfo) const override
      {
        if( _attributes_dirty & PATH )
          return;

        osg::State* state = renderInfo.getState();
        assert(state);

        state->setActiveTextureUnit(0);
        state->setClientActiveTextureUnit(0);
        state->disableAllVertexArrays();

        bool was_blend_enabled = state->getLastAppliedMode(GL_BLEND);
        bool was_stencil_enabled = state->getLastAppliedMode(GL_STENCIL_TEST);
        osg::StateAttribute const* blend_func =
          state->getLastAppliedAttribute(osg::StateAttribute::BLENDFUNC);

        // Setup paint
        if( _mode & VG_STROKE_PATH )
        {
          // Initialize/Update the paint
          if( _attributes_dirty & STROKE_COLOR )
          {
            if( _paint == VG_INVALID_HANDLE )
              _paint = vgCreatePaint();

            auto color = _stroke_color;
            color.a() *= _stroke_opacity / 255.f;
            vgSetParameterfv(_paint, VG_PAINT_COLOR, 4, color._v);

            _attributes_dirty &= ~STROKE_COLOR;
          }

          vgSetPaint(_paint, VG_STROKE_PATH);

          vgSetf(VG_STROKE_LINE_WIDTH, _stroke_width);
          vgSeti(VG_STROKE_CAP_STYLE, _stroke_linecap);
          vgSeti(VG_STROKE_JOIN_STYLE, _stroke_linejoin);
          vgSetfv( VG_STROKE_DASH_PATTERN,
                   _stroke_dash.size(),
                   _stroke_dash.empty() ? 0 : &_stroke_dash[0] );
        }
        if( _mode & VG_FILL_PATH )
        {
          // Initialize/update fill paint
          if( _attributes_dirty & FILL_COLOR )
          {
            if( _paint_fill == VG_INVALID_HANDLE )
              _paint_fill = vgCreatePaint();

            auto color = _fill_color;
            color.a() *= _fill_opacity / 255.f;
            vgSetParameterfv(_paint_fill, VG_PAINT_COLOR, 4, color._v);

            _attributes_dirty &= ~FILL_COLOR;
          }

          vgSetPaint(_paint_fill, VG_FILL_PATH);

          vgSeti(VG_FILL_RULE, _fill_rule);
        }

        // And finally draw the path
        if( _mode )
          vgDrawPath(_path, _mode);

        VGErrorCode err = vgGetError();
        if( err != VG_NO_ERROR )
          SG_LOG(SG_GL, SG_ALERT, "vgError: " << err);

        // Restore OpenGL state (TODO check if more is needed or integrate
        //                            better with OpenSceneGraph)
        if( was_blend_enabled )   glEnable(GL_BLEND);
        if( was_stencil_enabled ) glEnable(GL_STENCIL_TEST);
        if( blend_func )          blend_func->apply(*state);
      }

      osg::BoundingBox getTransformedBounds(const osg::Matrix& mat) const
      {
        osg::BoundingBox bb;

        osg::Vec2f cur(0, 0), // VG "Current point" (in local coordinates)
                   sub(0, 0); // beginning of current sub path
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
              cur = sub;
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
          {
            cur = points[ num_coords - 1 ];

            if( cmd == VG_MOVE_TO )
              sub = cur;
          }
        }

        return bb;
      }

      /**
       * Compute the bounding box
       */
      osg::BoundingBox computeBoundingBox()

      const override
      {
        if( _path == VG_INVALID_HANDLE || (_attributes_dirty & PATH) )
          return osg::BoundingBox();

        VGfloat min[2], size[2];
        vgPathBounds(_path, &min[0], &min[1], &size[0], &size[1]);

        _attributes_dirty &= ~BOUNDING_BOX;

        // vgPathBounds doesn't take stroke width into account
        float ext = 0.5 * _stroke_width;

        return osg::BoundingBox
        (
          min[0] - ext,           min[1] - ext,           -0.1,
          min[0] + size[0] + ext, min[1] + size[1] + ext,  0.1
        );
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

      mutable VGPath    _path {VG_INVALID_HANDLE};
      mutable VGPaint   _paint {VG_INVALID_HANDLE};
      mutable VGPaint   _paint_fill {VG_INVALID_HANDLE};
      mutable uint32_t  _attributes_dirty {~0u};

      CmdList   _cmds;
      CoordList _coords;

      VGbitfield            _mode {0};
      osg::Vec4f            _fill_color;
      uint8_t               _fill_opacity {255};
      VGFillRule            _fill_rule {VG_EVEN_ODD};
      osg::Vec4f            _stroke_color;
      uint8_t               _stroke_opacity {255};
      VGfloat               _stroke_width {1};
      std::vector<VGfloat>  _stroke_dash;
      VGCapStyle            _stroke_linecap {VG_CAP_BUTT};
      VGJoinStyle           _stroke_linejoin {VG_JOIN_MITER};

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
        {
          dirtyBound();

          // Recalculate bounding box now (prevent race condition)
          getBound();
        }
      }

      struct PathUpdateCallback:
        public osg::Drawable::UpdateCallback
      {
        void update(osg::NodeVisitor*, osg::Drawable* drawable) override
        {
          static_cast<PathDrawable*>(drawable)->update();
        }
      };
  };

  //----------------------------------------------------------------------------
  const std::string Path::TYPE_NAME = "path";

  //----------------------------------------------------------------------------
  void Path::staticInit()
  {
    if( isInit<Path>() )
      return;

    PathDrawableRef Path::*path = &Path::_path;

    addStyle("fill", "color", &PathDrawable::setFill, path);
    addStyle("fill-opacity", "numeric", &PathDrawable::setFillOpacity, path);
    addStyle("fill-rule", "", &PathDrawable::setFillRule, path);
    addStyle("stroke", "color", &PathDrawable::setStroke, path);
    addStyle("stroke-opacity", "numeric", &PathDrawable::setStrokeOpacity, path);
    addStyle("stroke-width", "numeric", &PathDrawable::setStrokeWidth, path);
    addStyle("stroke-dasharray", "", &PathDrawable::setStrokeDashArray, path);
    addStyle("stroke-linecap", "", &PathDrawable::setStrokeLinecap, path);
    addStyle("stroke-linejoin", "", &PathDrawable::setStrokeLinejoin, path);
  }

  //----------------------------------------------------------------------------
  Path::Path( const CanvasWeakPtr& canvas,
              const SGPropertyNode_ptr& node,
              const Style& parent_style,
              ElementWeakPtr parent ):
    Element(canvas, node, parent_style, parent),
    _path( new PathDrawable(this) ),
    _hasSVG(false),
    _hasRect(false)
  {
    staticInit();

    setDrawable(_path);
    setupStyle();
  }

  //----------------------------------------------------------------------------
  Path::~Path()
  {

  }

  //----------------------------------------------------------------------------
  osg::BoundingBox Path::getTransformedBounds(const osg::Matrix& m) const
  {
    return _path->getTransformedBounds(m);
  }

  //----------------------------------------------------------------------------
  Path& Path::addSegment(uint8_t cmd, std::initializer_list<float> coords)
  {
    _node->addChild("cmd")->setIntValue(cmd);

    for(float coord: coords)
      _node->addChild("coord")->setFloatValue(coord);

    return *this;
  }

  //----------------------------------------------------------------------------
  Path& Path::moveTo(float x_abs, float y_abs)
  {
    return addSegment(VG_MOVE_TO_ABS, {x_abs, y_abs});
  }

  //----------------------------------------------------------------------------
  Path& Path::move(float x_rel, float y_rel)
  {
    return addSegment(VG_MOVE_TO_REL, {x_rel, y_rel});
  }

  //----------------------------------------------------------------------------
  Path& Path::lineTo(float x_abs, float y_abs)
  {
    return addSegment(VG_LINE_TO_ABS, {x_abs, y_abs});
  }

  //----------------------------------------------------------------------------
  Path& Path::line(float x_rel, float y_rel)
  {
    return addSegment(VG_LINE_TO_REL, {x_rel, y_rel});
  }

  //----------------------------------------------------------------------------
  Path& Path::horizTo(float x_abs)
  {
    return addSegment(VG_HLINE_TO_ABS, {x_abs});
  }

  //----------------------------------------------------------------------------
  Path& Path::horiz(float x_rel)
  {
    return addSegment(VG_HLINE_TO_REL, {x_rel});
  }

  //----------------------------------------------------------------------------
  Path& Path::vertTo(float y_abs)
  {
    return addSegment(VG_VLINE_TO_ABS, {y_abs});
  }

  //----------------------------------------------------------------------------
  Path& Path::vert(float y_rel)
  {
    return addSegment(VG_VLINE_TO_REL, {y_rel});
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
  void Path::setSVGPath(const std::string& svgPath)
  {
    _node->setStringValue("svg", svgPath);
    _hasSVG = true;
    _attributes_dirty |= SVG;
  }

  //----------------------------------------------------------------------------
  void Path::setRect(const SGRect<float> &r)
  {
    _rect = r;
    _hasRect = true;
    _attributes_dirty |= RECT;
  }

  //----------------------------------------------------------------------------
  void Path::setRoundRect(const SGRect<float> &r, float radiusX, float radiusY)
  {
    if (radiusY < 0.0) {
        radiusY = radiusX;
    }

    setRect(r);
    _node->getChild("border-radius", 0, true)->setDoubleValue(radiusX);
    _node->getChild("border-radius", 1, true)->setDoubleValue(radiusY);
  }

  //----------------------------------------------------------------------------
  void Path::updateImpl(double dt)
  {
    Element::updateImpl(dt);

    if( _attributes_dirty & (CMDS | COORDS) )
    {
      _path->setSegments
      (
        _node->getChildValues<VGubyte, int>("cmd"),
        _node->getChildValues<VGfloat, float>("coord")
      );

      _attributes_dirty &= ~(CMDS | COORDS);
    }

    // SVG path overrides manual cmd/coord specification
    if( _hasSVG && (_attributes_dirty & SVG) )
    {
      CmdList cmds;
      CoordList coords;
      parseSVGPathToVGPath(_node->getStringValue("svg"), cmds, coords);
      _path->setSegments(cmds, coords);
      _attributes_dirty &= ~SVG;
    }

    if( _hasRect &&(_attributes_dirty & RECT) )
    {
      parseRectToVGPath();
      _attributes_dirty &= ~RECT;
    }
  }

  //----------------------------------------------------------------------------
  void Path::childChanged(SGPropertyNode* child)
  {
    if( child->getParent() != _node )
      return;
      
    const std::string& name = child->getNameString();
    const std::string &prName = child->getParent()->getNameString();

    if( strutils::starts_with(name, "border-") )
    {
      _attributes_dirty |= RECT;
      return;
    }

    if (prName == "rect")
    {
      _hasRect = true;
      if (name == "left") {
          _rect.setLeft(child->getDoubleValue());
      } else if (name == "top") {
          _rect.setTop(child->getDoubleValue());
      } else if (name == "right") {
          _rect.setRight(child->getDoubleValue());
      } else if (name == "bottom") {
          _rect.setBottom(child->getDoubleValue());
      } else if (name == "width") {
          _rect.setWidth(child->getDoubleValue());
      } else if (name == "height") {
          _rect.setHeight(child->getDoubleValue());
      }
      _attributes_dirty |= RECT;
      return;
    }

    if( name == "cmd" )
      _attributes_dirty |= CMDS;
    else if( name == "coord" )
      _attributes_dirty |= COORDS;
    else if ( name == "svg")
    {
      _hasSVG = true;
      _attributes_dirty |= SVG;
    }
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

  //----------------------------------------------------------------------------
  void operator+=(CoordList& base, const std::initializer_list<VGfloat>& other)
  {
    base.insert(base.end(), other.begin(), other.end());
  }

  //----------------------------------------------------------------------------
  void Path::parseRectToVGPath()
  {
    CmdList commands;
    CoordList coords;
    commands.reserve(4);
    coords.reserve(8);

    bool haveCorner = false;
    SGVec2f topLeft = parseRectCornerRadius(_node, "left", "top", haveCorner);
    if (haveCorner) {
        commands.push_back(VG_MOVE_TO_ABS);
        coords += {_rect.l(), _rect.t() + topLeft.y()};
        commands.push_back(VG_SCCWARC_TO_REL);
        coords += {topLeft.x(), topLeft.y(), 0.0, topLeft.x(), -topLeft.y()};
    } else {
        commands.push_back(VG_MOVE_TO_ABS);
        coords += {_rect.l(), _rect.t()};
    }

    SGVec2f topRight = parseRectCornerRadius(_node, "right", "top", haveCorner);
    if (haveCorner) {
        commands.push_back(VG_HLINE_TO_ABS);
        coords += {_rect.r() - topRight.x()};
        commands.push_back(VG_SCCWARC_TO_REL);
        coords += {topRight.x(), topRight.y(), 0.0, topRight.x(), topRight.y()};
    } else {
        commands.push_back(VG_HLINE_TO_ABS);
        coords += {_rect.r()};
    }

    SGVec2f bottomRight = parseRectCornerRadius(_node, "right", "bottom", haveCorner);
    if (haveCorner) {
        commands.push_back(VG_VLINE_TO_ABS);
        coords += {_rect.b() - bottomRight.y()};
        commands.push_back(VG_SCCWARC_TO_REL);
        coords += {bottomRight.x(), bottomRight.y(), 0.0, -bottomRight.x(), bottomRight.y()};
    } else {
        commands.push_back(VG_VLINE_TO_ABS);
        coords += {_rect.b()};
    }

    SGVec2f bottomLeft = parseRectCornerRadius(_node, "left", "bottom", haveCorner);
    if (haveCorner) {
        commands.push_back(VG_HLINE_TO_ABS);
        coords += {_rect.l() + bottomLeft.x()};
        commands.push_back(VG_SCCWARC_TO_REL);
        coords += {bottomLeft.x(), bottomLeft.y(), 0.0, -bottomLeft.x(), -bottomLeft.y()};
    } else {
        commands.push_back(VG_HLINE_TO_ABS);
        coords += {_rect.l()};
    }

    commands.push_back(VG_CLOSE_PATH);
    _path->setSegments(commands, coords);
  }

} // namespace canvas
} // namespace simgear
