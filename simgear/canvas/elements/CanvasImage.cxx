// An image on the Canvas
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

#include "CanvasImage.hxx"

#include <simgear/canvas/Canvas.hxx>
#include <simgear/canvas/CanvasMgr.hxx>
#include <simgear/canvas/CanvasSystemAdapter.hxx>
#include <simgear/canvas/MouseEvent.hxx>
#include <simgear/scene/util/parse_color.hxx>
#include <simgear/misc/sg_path.hxx>

#include <osg/Array>
#include <osg/Geometry>
#include <osg/PrimitiveSet>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/range.hpp>
#include <boost/tokenizer.hpp>

namespace simgear
{
namespace canvas
{
  /**
   * Callback to enable/disable rendering of canvas displayed inside windows or
   * other canvases.
   */
  class CullCallback:
    public osg::Drawable::CullCallback
  {
    public:
      CullCallback(const CanvasWeakPtr& canvas);

    private:
      CanvasWeakPtr _canvas;

      virtual bool cull( osg::NodeVisitor* nv,
                         osg::Drawable* drawable,
                         osg::RenderInfo* renderInfo ) const;
  };

  //----------------------------------------------------------------------------
  CullCallback::CullCallback(const CanvasWeakPtr& canvas):
    _canvas( canvas )
  {

  }

  //----------------------------------------------------------------------------
  bool CullCallback::cull( osg::NodeVisitor* nv,
                           osg::Drawable* drawable,
                           osg::RenderInfo* renderInfo ) const
  {
    if( !_canvas.expired() )
      _canvas.lock()->enableRendering();

    // TODO check if window/image should be culled
    return false;
  }

  //----------------------------------------------------------------------------
  Image::Image( const CanvasWeakPtr& canvas,
                const SGPropertyNode_ptr& node,
                const Style& parent_style,
                Element* parent ):
    Element(canvas, node, parent_style, parent),
    _texture(new osg::Texture2D),
    _node_src_rect( node->getNode("source", 0, true) ),
    _src_rect(0,0),
    _region(0,0)
  {
    _geom = new osg::Geometry;
    _geom->setUseDisplayList(false);

    osg::StateSet *stateSet = _geom->getOrCreateStateSet();
    stateSet->setTextureAttributeAndModes(0, _texture.get());
    stateSet->setDataVariance(osg::Object::STATIC);

    // allocate arrays for the image
    _vertices = new osg::Vec3Array(4);
    _vertices->setDataVariance(osg::Object::DYNAMIC);
    _geom->setVertexArray(_vertices);

    _texCoords = new osg::Vec2Array(4);
    _texCoords->setDataVariance(osg::Object::DYNAMIC);
    _geom->setTexCoordArray(0, _texCoords);

    _colors = new osg::Vec4Array(1);
    _colors->setDataVariance(osg::Object::DYNAMIC);
    _geom->setColorBinding(osg::Geometry::BIND_OVERALL);
    _geom->setColorArray(_colors);

    _prim = new osg::DrawArrays(osg::PrimitiveSet::QUADS);
    _prim->set(osg::PrimitiveSet::QUADS, 0, 4);
    _prim->setDataVariance(osg::Object::DYNAMIC);
    _geom->addPrimitiveSet(_prim);

    setDrawable(_geom);

    addStyle("fill", &Image::setFill, this);
    addStyle("slice", &Image::setSlice, this);
    addStyle("slice-width", &Image::setSliceWidth, this);
    addStyle("outset", &Image::setOutset, this);

    setFill("#ffffff"); // TODO how should we handle default values?

    setupStyle();
  }

  //----------------------------------------------------------------------------
  Image::~Image()
  {

  }

  //----------------------------------------------------------------------------
  void Image::update(double dt)
  {
    Element::update(dt);

    if( !_attributes_dirty )
      return;

    const SGRect<int>& tex_dim = getTextureDimensions();

    // http://www.w3.org/TR/css3-background/#border-image-slice

    // The ‘fill’ keyword, if present, causes the middle part of the image to be
    // preserved. (By default it is discarded, i.e., treated as empty.)
    bool fill = (_slice.keyword == "fill");

    if( _attributes_dirty & DEST_SIZE )
    {
      size_t num_vertices = (_slice.valid ? (fill ? 9 : 8) : 1) * 4;

      if( num_vertices != _prim->getNumPrimitives() )
      {
        _vertices->resize(num_vertices);
        _texCoords->resize(num_vertices);
        _prim->setCount(num_vertices);
        _prim->dirty();

        _attributes_dirty |= SRC_RECT;
      }

      // http://www.w3.org/TR/css3-background/#border-image-outset
      SGRect<float> region = _region;
      if( _outset.valid )
      {
        const CSSOffsets& outset = _outset.getAbsOffsets(tex_dim);
        region.t() -= outset.t;
        region.r() += outset.r;
        region.b() += outset.b;
        region.l() -= outset.l;
      }

      if( !_slice.valid )
      {
        setQuad(0, region.getMin(), region.getMax());
      }
      else
      {
        /*
        Image slice, 9-scale, whatever it is called. The four corner images
        stay unscaled (tl, tr, bl, br) whereas the other parts are scaled to
        fill the remaining space up to the specified size.

        x[0] x[1]     x[2] x[3]
          |    |        |    |
          -------------------- - y[0]
          | tl |   top  | tr |
          -------------------- - y[1]
          |    |        |    |
          | l  |        |  r |
          | e  | center |  i |
          | f  |        |  g |
          | t  |        |  h |
          |    |        |  t |
          -------------------- - y[2]
          | bl | bottom | br |
          -------------------- - y[3]
         */

        const CSSOffsets& slice =
          (_slice_width.valid ? _slice_width : _slice).getAbsOffsets(tex_dim);
        float x[4] = {
          region.l(),
          region.l() + slice.l,
          region.r() - slice.r,
          region.r()
        };
        float y[4] = {
          region.t(),
          region.t() + slice.t,
          region.b() - slice.b,
          region.b()
        };

        int i = 0;
        for(int ix = 0; ix < 3; ++ix)
          for(int iy = 0; iy < 3; ++iy)
          {
            if( ix == 1 && iy == 1 && !fill )
              // The ‘fill’ keyword, if present, causes the middle part of the
              // image to be filled.
              continue;

            setQuad( i++,
                     SGVec2f(x[ix    ], y[iy    ]),
                     SGVec2f(x[ix + 1], y[iy + 1]) );
          }
      }

      _vertices->dirty();
      _attributes_dirty &= ~DEST_SIZE;
      _geom->dirtyBound();
      setBoundingBox(_geom->getBound());
    }

    if( _attributes_dirty & SRC_RECT )
    {
      SGRect<float> src_rect = _src_rect;
      if( !_node_src_rect->getBoolValue("normalized", true) )
      {
        src_rect.t() /= tex_dim.height();
        src_rect.r() /= tex_dim.width();
        src_rect.b() /= tex_dim.height();
        src_rect.l() /= tex_dim.width();
      }

      // Image coordinate systems y-axis is flipped
      std::swap(src_rect.t(), src_rect.b());

      if( !_slice.valid )
      {
        setQuadUV(0, src_rect.getMin(), src_rect.getMax());
      }
      else
      {
        const CSSOffsets& slice = _slice.getRelOffsets(tex_dim);
        float x[4] = {
          src_rect.l(),
          src_rect.l() + slice.l,
          src_rect.r() - slice.r,
          src_rect.r()
        };
        float y[4] = {
          src_rect.t(),
          src_rect.t() - slice.t,
          src_rect.b() + slice.b,
          src_rect.b()
        };

        int i = 0;
        for(int ix = 0; ix < 3; ++ix)
          for(int iy = 0; iy < 3; ++iy)
          {
            if( ix == 1 && iy == 1 && !fill )
              // The ‘fill’ keyword, if present, causes the middle part of the
              // image to be filled.
              continue;

            setQuadUV( i++,
                       SGVec2f(x[ix    ], y[iy    ]),
                       SGVec2f(x[ix + 1], y[iy + 1]) );
          }
      }

      _texCoords->dirty();
      _attributes_dirty &= ~SRC_RECT;
    }
  }

  //----------------------------------------------------------------------------
  void Image::setSrcCanvas(CanvasPtr canvas)
  {
    if( !_src_canvas.expired() )
      _src_canvas.lock()->removeDependentCanvas(_canvas);

    _src_canvas = canvas;
    _geom->getOrCreateStateSet()
         ->setTextureAttribute(0, canvas ? canvas->getTexture() : 0);
    _geom->setCullCallback(canvas ? new CullCallback(canvas) : 0);

    if( !_src_canvas.expired() )
    {
      setupDefaultDimensions();
      _src_canvas.lock()->addDependentCanvas(_canvas);
    }
  }

  //----------------------------------------------------------------------------
  CanvasWeakPtr Image::getSrcCanvas() const
  {
    return _src_canvas;
  }

  //----------------------------------------------------------------------------
  void Image::setImage(osg::Image *img)
  {
    // remove canvas...
    setSrcCanvas( CanvasPtr() );

    _texture->setImage(img);
    _geom->getOrCreateStateSet()
         ->setTextureAttributeAndModes(0, _texture);

    if( img )
      setupDefaultDimensions();
  }

  //----------------------------------------------------------------------------
  void Image::setFill(const std::string& fill)
  {
    osg::Vec4 color;
    if( !parseColor(fill, color) )
      return;

    _colors->front() = color;
    _colors->dirty();
  }

  //----------------------------------------------------------------------------
  void Image::setSlice(const std::string& slice)
  {
    _slice = parseSideOffsets(slice);
    _attributes_dirty |= SRC_RECT | DEST_SIZE;
  }

  //----------------------------------------------------------------------------
  void Image::setSliceWidth(const std::string& width)
  {
    _slice_width = parseSideOffsets(width);
    _attributes_dirty |= DEST_SIZE;
  }

  //----------------------------------------------------------------------------
  void Image::setOutset(const std::string& outset)
  {
    _outset = parseSideOffsets(outset);
    _attributes_dirty |= DEST_SIZE;
  }

  //----------------------------------------------------------------------------
  const SGRect<float>& Image::getRegion() const
  {
    return _region;
  }

  //----------------------------------------------------------------------------
  bool Image::handleMouseEvent(MouseEventPtr event)
  {
    CanvasPtr src_canvas = _src_canvas.lock();

    if( !src_canvas )
      return false;

    if( _outset.valid )
    {
      CSSOffsets outset = _outset.getAbsOffsets(getTextureDimensions());

      event.reset( new MouseEvent(*event) );
      event->client_pos += osg::Vec2f(outset.l, outset.t);
      event->client_pos.x() *= src_canvas->getViewWidth()
                             / (_region.width() + outset.l + outset.r);
      event->client_pos.y() *= src_canvas->getViewHeight()
                             / (_region.height() + outset.t + outset.b);
    }

    return src_canvas->handleMouseEvent(event);
  }

  //----------------------------------------------------------------------------
  Image::CSSOffsets
  Image::CSSBorder::getRelOffsets(const SGRect<int>& dim) const
  {
    CSSOffsets ret;
    for(int i = 0; i < 4; ++i)
    {
      ret.val[i] = offsets.val[i];
      if( !types.rel[i] )
        ret.val[i] /= (i & 1) ? dim.height() : dim.width();
    }
    return ret;
  }

  //----------------------------------------------------------------------------
  Image::CSSOffsets
  Image::CSSBorder::getAbsOffsets(const SGRect<int>& dim) const
  {
    CSSOffsets ret;
    for(int i = 0; i < 4; ++i)
    {
      ret.val[i] = offsets.val[i];
      if( types.rel[i] )
        ret.val[i] *= (i & 1) ? dim.height() : dim.width();
    }
    return ret;
  }

  //----------------------------------------------------------------------------
  void Image::childChanged(SGPropertyNode* child)
  {
    const std::string& name = child->getNameString();

    if( child->getParent() == _node_src_rect )
    {
      _attributes_dirty |= SRC_RECT;

      if(      name == "left" )
        _src_rect.setLeft( child->getFloatValue() );
      else if( name == "right" )
        _src_rect.setRight( child->getFloatValue() );
      else if( name == "top" )
        _src_rect.setTop( child->getFloatValue() );
      else if( name == "bottom" )
        _src_rect.setBottom( child->getFloatValue() );

      return;
    }
    else if( child->getParent() != _node )
      return;

    if( name == "x" )
    {
      _region.setX( child->getFloatValue() );
      _attributes_dirty |= DEST_SIZE;
    }
    else if( name == "y" )
    {
      _region.setY( child->getFloatValue() );
      _attributes_dirty |= DEST_SIZE;
    }
    else if( name == "size" )
    {
      if( child->getIndex() == 0 )
        _region.setWidth( child->getFloatValue() );
      else
        _region.setHeight( child->getFloatValue() );

      _attributes_dirty |= DEST_SIZE;
    }
    else if( name == "file" )
    {
      static const std::string CANVAS_PROTOCOL = "canvas://";
      const std::string& path = child->getStringValue();

      CanvasPtr canvas = _canvas.lock();
      if( !canvas )
      {
        SG_LOG(SG_GL, SG_ALERT, "canvas::Image: No canvas available");
        return;
      }

      if( boost::starts_with(path, CANVAS_PROTOCOL) )
      {
        CanvasMgr* canvas_mgr = canvas->getCanvasMgr();
        if( !canvas_mgr )
        {
          SG_LOG(SG_GL, SG_ALERT, "canvas::Image: Failed to get CanvasMgr");
          return;
        }

        const SGPropertyNode* canvas_node =
          canvas_mgr->getPropertyRoot()
                    ->getParent()
                    ->getNode( path.substr(CANVAS_PROTOCOL.size()) );
        if( !canvas_node )
        {
          SG_LOG(SG_GL, SG_ALERT, "canvas::Image: No such canvas: " << path);
          return;
        }

        // TODO add support for other means of addressing canvases (eg. by
        // name)
        CanvasPtr src_canvas = canvas_mgr->getCanvas( canvas_node->getIndex() );
        if( !src_canvas )
        {
          SG_LOG(SG_GL, SG_ALERT, "canvas::Image: Invalid canvas: " << path);
          return;
        }

        setSrcCanvas(src_canvas);
      }
      else
      {
        setImage( canvas->getSystemAdapter()->getImage(path) );
      }
    }
  }

  //----------------------------------------------------------------------------
  void Image::setupDefaultDimensions()
  {
    if( !_src_rect.width() || !_src_rect.height() )
    {
      // Show whole image by default
      _node_src_rect->setBoolValue("normalized", true);
      _node_src_rect->setFloatValue("right", 1);
      _node_src_rect->setFloatValue("bottom", 1);
    }

    if( !_region.width() || !_region.height() )
    {
      // Default to image size.
      // TODO handle showing only part of image?
      const SGRect<int>& dim = getTextureDimensions();
      _node->setFloatValue("size[0]", dim.width());
      _node->setFloatValue("size[1]", dim.height());
    }
  }

  //----------------------------------------------------------------------------
  SGRect<int> Image::getTextureDimensions() const
  {
    CanvasPtr canvas = _src_canvas.lock();
    SGRect<int> dim(0,0);

    // Use canvas/image dimensions rather than texture dimensions, as they could
    // be resized internally by OpenSceneGraph (eg. to nearest power of two).
    if( canvas )
    {
      dim.setRight( canvas->getViewWidth() );
      dim.setBottom( canvas->getViewHeight() );
    }
    else if( _texture )
    {
      osg::Image* img = _texture->getImage();

      if( img )
      {
        dim.setRight( img->s() );
        dim.setBottom( img->t() );
      }
    }

    return dim;
  }

  //----------------------------------------------------------------------------
  void Image::setQuad(size_t index, const SGVec2f& tl, const SGVec2f& br)
  {
    int i = index * 4;
    (*_vertices)[i + 0].set(tl.x(), tl.y(), 0);
    (*_vertices)[i + 1].set(br.x(), tl.y(), 0);
    (*_vertices)[i + 2].set(br.x(), br.y(), 0);
    (*_vertices)[i + 3].set(tl.x(), br.y(), 0);
  }

  //----------------------------------------------------------------------------
  void Image::setQuadUV(size_t index, const SGVec2f& tl, const SGVec2f& br)
  {
    int i = index * 4;
    (*_texCoords)[i + 0].set(tl.x(), tl.y());
    (*_texCoords)[i + 1].set(br.x(), tl.y());
    (*_texCoords)[i + 2].set(br.x(), br.y());
    (*_texCoords)[i + 3].set(tl.x(), br.y());
  }

  //----------------------------------------------------------------------------
  Image::CSSBorder Image::parseSideOffsets(const std::string& str) const
  {
    if( str.empty() )
      return CSSBorder();

    // [<number>'%'?]{1,4} (top[,right[,bottom[,left]]])
    //
    // Percentages are relative to the size of the image: the width of the
    // image for the horizontal offsets, the height for vertical offsets.
    // Numbers represent pixels in the image.
    int c = 0;
    CSSBorder ret;

    typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
    const boost::char_separator<char> del(" \t\n");

    tokenizer tokens(str.begin(), str.end(), del);
    for( tokenizer::const_iterator tok = tokens.begin();
         tok != tokens.end() && c < 4;
         ++tok )
    {
      if( isalpha(*tok->begin()) )
        ret.keyword = *tok;
      else
      {
        bool rel = ret.types.rel[c] = (*tok->rbegin() == '%');
        ret.offsets.val[c] =
          // Negative values are not allowed and values bigger than the size of
          // the image are interpreted as ‘100%’. TODO check max
          std::max
          (
            0.f,
            boost::lexical_cast<float>
            (
              rel ? boost::make_iterator_range(tok->begin(), tok->end() - 1)
                  : *tok
            )
            /
            (rel ? 100 : 1)
          );
        ++c;
      }
    }

    // When four values are specified, they set the offsets on the top, right,
    // bottom and left sides in that order.

#define CSS_COPY_VAL(dest, src)\
  {\
    ret.offsets.val[dest] = ret.offsets.val[src];\
    ret.types.rel[dest] = ret.types.rel[src];\
  }

    if( c < 4 )
    {
      if( c < 3 )
      {
        if( c < 2 )
          // if the right is missing, it is the same as the top.
          CSS_COPY_VAL(1, 0);

        // if the bottom is missing, it is the same as the top
        CSS_COPY_VAL(2, 0);
      }

      // If the left is missing, it is the same as the right
      CSS_COPY_VAL(3, 1);
    }

#undef CSS_COPY_VAL

    ret.valid = true;
    return ret;
  }

} // namespace canvas
} // namespace simgear
