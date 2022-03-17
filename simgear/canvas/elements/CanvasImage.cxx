///@file
/// An image on the Canvas
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

#include "CanvasImage.hxx"

#include <simgear/canvas/Canvas.hxx>
#include <simgear/canvas/CanvasMgr.hxx>
#include <simgear/canvas/CanvasSystemAdapter.hxx>
#include <simgear/canvas/events/KeyboardEvent.hxx>
#include <simgear/canvas/events/MouseEvent.hxx>
#include <simgear/scene/util/OsgMath.hxx>
#include <simgear/scene/util/parse_color.hxx>
#include <simgear/misc/sg_path.hxx>

#include <osg/Array>
#include <osg/Geometry>
#include <osg/PrimitiveSet>
#include <osgDB/Registry>
#include <osg/Version>
#include <simgear/scene/util/SGReaderWriterOptions.hxx>

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
      void cullNextFrame();

    private:
      CanvasWeakPtr _canvas;
      mutable bool  _cull_next_frame;

      virtual bool cull( osg::NodeVisitor* nv,
                         osg::Drawable* drawable,
                         osg::RenderInfo* renderInfo ) const;
  };

  //----------------------------------------------------------------------------
  CullCallback::CullCallback(const CanvasWeakPtr& canvas):
    _canvas( canvas ),
    _cull_next_frame( false )
  {

  }

  //----------------------------------------------------------------------------
  void CullCallback::cullNextFrame()
  {
    _cull_next_frame = true;
  }

  //----------------------------------------------------------------------------
  bool CullCallback::cull( osg::NodeVisitor* nv,
                           osg::Drawable* drawable,
                           osg::RenderInfo* renderInfo ) const
  {
    CanvasPtr canvas = _canvas.lock();
    if( canvas )
      canvas->enableRendering();

    if( !_cull_next_frame )
      // TODO check if window/image should be culled
      return false;

    _cull_next_frame = false;
    return true;
  }

  //----------------------------------------------------------------------------
  const std::string Image::TYPE_NAME = "image";

  //----------------------------------------------------------------------------
  void Image::staticInit()
  {
    if( isInit<Image>() )
      return;

    addStyle("fill", "color", &Image::setFill);
    addStyle("outset", "", &Image::setOutset);
    addStyle("preserveAspectRatio", "", &Image::setPreserveAspectRatio);
    addStyle("slice", "", &Image::setSlice);
    addStyle("slice-width", "", &Image::setSliceWidth);

    osgDB::Registry* reg = osgDB::Registry::instance();
    if( !reg->getReaderWriterForExtension("png") )
      SG_LOG(SG_GL, SG_ALERT, "canvas::Image: Missing 'png' image reader");
  }

  //----------------------------------------------------------------------------
  Image::Image( const CanvasWeakPtr& canvas,
                const SGPropertyNode_ptr& node,
                const Style& parent_style,
                ElementWeakPtr parent ):
    Element(canvas, node, parent_style, parent),
    _texture(new osg::Texture2D),
    _node_src_rect( node->getNode("source", 0, true) )
  {
    staticInit();

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
    _geom->setTexCoordArray(0, _texCoords, osg::Array::BIND_PER_VERTEX);

    _colors = new osg::Vec4Array(1);
    _colors->setDataVariance(osg::Object::DYNAMIC);
    _geom->setColorArray(_colors, osg::Array::BIND_OVERALL);

    _prim = new osg::DrawArrays(osg::PrimitiveSet::QUADS);
    _prim->set(osg::PrimitiveSet::QUADS, 0, 4);
    _prim->setDataVariance(osg::Object::DYNAMIC);
    _geom->addPrimitiveSet(_prim);

    setDrawable(_geom);

    setFill("#ffffff"); // TODO how should we handle default values?
    setupStyle();
  }

  //----------------------------------------------------------------------------
  Image::~Image()
  {
    if( _http_request )
    {
      Canvas::getSystemAdapter()
        ->getHTTPClient()
        ->cancelRequest(_http_request, "image destroyed");
    }
  }

  //----------------------------------------------------------------------------
  void Image::valueChanged(SGPropertyNode* child)
  {
    // If the image is switched from invisible to visible, and it shows a
    // canvas, we need to delay showing it by one frame to ensure the canvas is
    // updated before the image is displayed.
    //
    // As canvas::Element handles and filters changes to the "visible" property
    // we can not check this in Image::childChanged but instead have to override
    // Element::valueChanged.
    if(    !isVisible()
        && child->getParent() == _node
        && child->getNameString() == "visible"
        && child->getBoolValue() )
    {
      CullCallback* cb =
#if OSG_VERSION_LESS_THAN(3,3,2)
        static_cast<CullCallback*>
#else
        dynamic_cast<CullCallback*>
#endif
        ( _geom->getCullCallback() );

      if( cb )
        cb->cullNextFrame();
    }

    Element::valueChanged(child);
  }

  //----------------------------------------------------------------------------
  void Image::setSrcCanvas(CanvasPtr canvas)
  {
    CanvasPtr src_canvas = _src_canvas.lock(),
              self_canvas = _canvas.lock();

    if( src_canvas )
      src_canvas->removeParentCanvas(self_canvas);
    if( self_canvas )
      self_canvas->removeChildCanvas(src_canvas);

    _src_canvas = src_canvas = canvas;
    _attributes_dirty |= SRC_CANVAS;
    _geom->setCullCallback(canvas ? new CullCallback(canvas) : 0);

    if( src_canvas )
    {
      setupDefaultDimensions();

      if( self_canvas )
      {
        self_canvas->addChildCanvas(src_canvas);
        src_canvas->addParentCanvas(self_canvas);
      }
    }
  }

  //----------------------------------------------------------------------------
  CanvasWeakPtr Image::getSrcCanvas() const
  {
    return _src_canvas;
  }

  //----------------------------------------------------------------------------
  void Image::setImage(osg::ref_ptr<osg::Image> img)
  {
    // remove canvas...
    setSrcCanvas( CanvasPtr() );

    _texture->setResizeNonPowerOfTwoHint(false);
    _texture->setImage(img);
    _texture->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
    _texture->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);
    _geom->getOrCreateStateSet()
         ->setTextureAttributeAndModes(0, _texture);

    if( img )
      setupDefaultDimensions();
  }

  //----------------------------------------------------------------------------
  void Image::setFill(const std::string& fill)
  {
    osg::Vec4 color(1,1,1,1);
    if(    !fill.empty() // If no color is given default to white
        && !parseColor(fill, color) )
      return;
    setFill(color);
  }

    //----------------------------------------------------------------------------
    void Image::setFill(const osg::Vec4& color)
    {
        _colors->front() = color;
        _colors->dirty();
    }


  //----------------------------------------------------------------------------
  void Image::setOutset(const std::string& outset)
  {
    _outset = CSSBorder::parse(outset);
    _attributes_dirty |= DEST_SIZE;
  }

  //----------------------------------------------------------------------------
  void Image::setPreserveAspectRatio(const std::string& scale)
  {
    _preserve_aspect_ratio = SVGpreserveAspectRatio::parse(scale);
    _attributes_dirty |= SRC_RECT;
  }

  //----------------------------------------------------------------------------
  void Image::setSourceRect(const SGRect<float>& sourceRect)
  {
      _attributes_dirty |= SRC_RECT;
      _src_rect = sourceRect;
  }

  //----------------------------------------------------------------------------
  void Image::setSlice(const std::string& slice)
  {
    _slice = CSSBorder::parse(slice);
    _attributes_dirty |= SRC_RECT | DEST_SIZE;
  }

  //----------------------------------------------------------------------------
  void Image::setSliceWidth(const std::string& width)
  {
    _slice_width = CSSBorder::parse(width);
    _attributes_dirty |= DEST_SIZE;
  }

  //----------------------------------------------------------------------------
  const SGRect<float>& Image::getRegion() const
  {
    return _region;
  }

  //----------------------------------------------------------------------------
  bool Image::handleEvent(const EventPtr& event)
  {
    bool handled = Element::handleEvent(event);

    CanvasPtr src_canvas = _src_canvas.lock();
    if( !src_canvas )
      return handled;

    if( MouseEventPtr mouse_event = dynamic_cast<MouseEvent*>(event.get()) )
    {
      mouse_event.reset( new MouseEvent(*mouse_event) );

      mouse_event->client_pos = mouse_event->local_pos
                              - toOsg(_region.getMin());

      osg::Vec2f size(_region.width(), _region.height());
      if( _outset.isValid() )
      {
        CSSBorder::Offsets outset =
          _outset.getAbsOffsets(getTextureDimensions());

        mouse_event->client_pos += osg::Vec2f(outset.l, outset.t);
        size.x() += outset.l + outset.r;
        size.y() += outset.t + outset.b;
      }

      // Scale event pos according to canvas view size vs. displayed/screen size
      mouse_event->client_pos.x() *= src_canvas->getViewWidth() / size.x();
      mouse_event->client_pos.y() *= src_canvas->getViewHeight()/ size.y();
      mouse_event->local_pos = mouse_event->client_pos;

      handled |= src_canvas->handleMouseEvent(mouse_event);
    }
    else if( KeyboardEventPtr keyboard_event =
               dynamic_cast<KeyboardEvent*>(event.get()) )
    {
      handled |= src_canvas->handleKeyboardEvent(keyboard_event);
    }

    return handled;
  }

  //----------------------------------------------------------------------------
  void Image::updateImpl(double dt)
  {
    Element::updateImpl(dt);

    osg::Texture2D* texture = dynamic_cast<osg::Texture2D*>
    (
      _geom->getOrCreateStateSet()
           ->getTextureAttribute(0, osg::StateAttribute::TEXTURE)
    );
    auto canvas = _src_canvas.lock();

    if(    (_attributes_dirty & SRC_CANVAS)
           // check if texture has changed (eg. due to resizing)
        || (canvas && texture != canvas->getTexture()) )
    {
      _geom->getOrCreateStateSet()
           ->setTextureAttribute(0, canvas ? canvas->getTexture() : 0);

      if( !canvas || canvas->isInit() )
        _attributes_dirty &= ~SRC_CANVAS;
    }

    if( !_attributes_dirty )
      return;

    const SGRect<int>& tex_dim = getTextureDimensions();

    // http://www.w3.org/TR/css3-background/#border-image-slice

    // The ‘fill’ keyword, if present, causes the middle part of the image to be
    // preserved. (By default it is discarded, i.e., treated as empty.)
    bool fill = (_slice.getKeyword() == "fill");

    if( _attributes_dirty & DEST_SIZE )
    {
      size_t num_vertices = (_slice.isValid() ? (fill ? 9 : 8) : 1) * 4;

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
      if( _outset.isValid() )
      {
        const CSSBorder::Offsets& outset = _outset.getAbsOffsets(tex_dim);
        region.t() -= outset.t;
        region.r() += outset.r;
        region.b() += outset.b;
        region.l() -= outset.l;
      }

      if( !_slice.isValid() )
      {
        setQuad(0, region.getMin(), region.getMax());

        if( !_preserve_aspect_ratio.scaleToFill() )
          // We need to update texture coordinates to keep the aspect ratio
          _attributes_dirty |= SRC_RECT;
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

        const CSSBorder::Offsets& slice =
          (_slice_width.isValid() ? _slice_width : _slice)
          .getAbsOffsets(tex_dim);
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

      if( !_slice.isValid() )
      {
        // Image scaling preserving aspect ratio. Change texture coordinates to
        // scale image accordingly.
        //
        // TODO allow to specify what happens to not filled space (eg. color,
        //      or texture repeat/mirror)
        //
        // http://www.w3.org/TR/SVG11/coords.html#PreserveAspectRatioAttribute
        if( !_preserve_aspect_ratio.scaleToFill() )
        {
          osg::BoundingBox const& bb = getBoundingBox();
          float dst_width = bb._max.x() - bb._min.x(),
                dst_height = bb._max.y() - bb._min.y();
          float scale_x = dst_width / tex_dim.width(),
                scale_y = dst_height / tex_dim.height();

          float scale = _preserve_aspect_ratio.scaleToFit()
                      ? std::min(scale_x, scale_y)
                      : std::max(scale_x, scale_y);

          if( scale_x != scale )
          {
            float d = scale_x / scale - 1;
            if(  _preserve_aspect_ratio.alignX()
              == SVGpreserveAspectRatio::ALIGN_MIN )
            {
              src_rect.r() += d;
            }
            else if(  _preserve_aspect_ratio.alignX()
                   == SVGpreserveAspectRatio::ALIGN_MAX )
            {
              src_rect.l() -= d;
            }
            else
            {
              src_rect.l() -= d / 2;
              src_rect.r() += d / 2;
            }
          }

          if( scale_y != scale )
          {
            float d = scale_y / scale - 1;
            if(  _preserve_aspect_ratio.alignY()
              == SVGpreserveAspectRatio::ALIGN_MIN )
            {
              src_rect.b() -= d;
            }
            else if(  _preserve_aspect_ratio.alignY()
                   == SVGpreserveAspectRatio::ALIGN_MAX )
            {
              src_rect.t() += d;
            }
            else
            {
              src_rect.t() += d / 2;
              src_rect.b() -= d / 2;
            }
          }
        }

        setQuadUV(0, src_rect.getMin(), src_rect.getMax());
      }
      else
      {
        const CSSBorder::Offsets& slice = _slice.getRelOffsets(tex_dim);
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
    else if( name == "src" || name == "file" )
    {
      if( name == "file" )
        SG_LOG(SG_GL, SG_WARN, "'file' is deprecated. Use 'src' instead");

      // Abort pending request
      if( _http_request )
      {
        Canvas::getSystemAdapter()
          ->getHTTPClient()
          ->cancelRequest(_http_request, "setting new image");
        _http_request.reset();
      }

      static const std::string PROTOCOL_SEP = "://";

      std::string url = child->getStringValue(),
                  protocol, path;

      size_t sep_pos = url.find(PROTOCOL_SEP);
      if( sep_pos != std::string::npos )
      {
        protocol = url.substr(0, sep_pos);
        path = url.substr(sep_pos + PROTOCOL_SEP.length());
      }
      else
        path = url;

      if( protocol == "canvas" )
      {
        CanvasPtr canvas = _canvas.lock();
        if( !canvas )
        {
          SG_LOG(SG_GL, SG_ALERT, "canvas::Image: No canvas available");
          return;
        }

        CanvasMgr* canvas_mgr = canvas->getCanvasMgr();
        if( !canvas_mgr )
        {
          SG_LOG(SG_GL, SG_ALERT, "canvas::Image: Failed to get CanvasMgr");
          return;
        }

        const SGPropertyNode* canvas_node =
          canvas_mgr->getPropertyRoot()
                    ->getParent()
                    ->getNode( path );
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
      else if( protocol == "http" || protocol == "https" )
      // TODO check https
      {
        _http_request =
          Canvas::getSystemAdapter()
          ->getHTTPClient()
          ->load(url)
          // TODO handle capture of 'this'
          ->done(this, &Image::handleImageLoadDone);
      }
      else
      {
        setImage( Canvas::getSystemAdapter()->getImage(path) );
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
  void Image::handleImageLoadDone(HTTP::Request* req)
  {
    // Ignore stale/expired requests
    if( _http_request != req )
      return;
    _http_request.reset();

    if( req->responseCode() != 200 )
    {
      SG_LOG(SG_IO, SG_WARN, "failed to download '" << req->url() << "': "
                                                    << req->responseReason());
      return;
    }

    const std::string ext = SGPath(req->path()).extension(),
                      mime = req->responseMime();

    SG_LOG(SG_IO, SG_INFO, "received " << req->url() <<
                           " (ext=" << ext << ", MIME=" << mime << ")");

    const std::string& img_data =
      static_cast<HTTP::MemoryRequest*>(req)->responseBody();
    osgDB::Registry* reg = osgDB::Registry::instance();

    // First try to detect image type by extension
    osgDB::ReaderWriter* rw = reg->getReaderWriterForExtension(ext);
    if( rw && loadImage(*rw, img_data, *req, "extension") )
      return;

    // Now try with MIME type
    rw = reg->getReaderWriterForMimeType(mime);
    if( rw && loadImage(*rw, img_data, *req, "MIME type") )
      return;

    SG_LOG(SG_IO, SG_WARN, "unable to read image '" << req->url() << "'");
  }

  //----------------------------------------------------------------------------
  bool Image::loadImage( osgDB::ReaderWriter& reader,
                         const std::string& data,
                         HTTP::Request& request,
                         const std::string& type )
  {
    SG_LOG(SG_IO, SG_DEBUG, "use image reader detected by " << type);

    osg::ref_ptr<SGReaderWriterOptions> opt;
    opt = SGReaderWriterOptions::copyOrCreate(osgDB::Registry::instance()->getOptions());
    opt->setLoadOriginHint(SGReaderWriterOptions::LoadOriginHint::ORIGIN_CANVAS);

    std::istringstream data_strm(data);
    osgDB::ReaderWriter::ReadResult result = reader.readImage(data_strm, opt);
    if( result.success() )
    {
      setImage( result.takeImage() );
      return true;
    }

    SG_LOG(SG_IO, SG_WARN, "failed to read image '" << request.url() << "': "
                                                    << result.message());

    return false;
  }

  void Image::fillRect(const SGRect<int>& rect, const std::string& c)
  {
    osg::Vec4 color(1,1,1,1);
    if(!c.empty() && !parseColor(c, color))
      return;
    
    fillRect(rect, color);
  }

void fillRow(GLubyte* row, GLuint pixel, GLuint width, GLuint pixelBytes)
{
  GLubyte* dst = row;
  for (GLuint x = 0; x < width; ++x) {
    memcpy(dst, &pixel, pixelBytes);
    dst += pixelBytes;
  }
}

SGRect<int> intersectRect(const SGRect<int>& a, const SGRect<int>& b)
{
  SGVec2<int> m1 = max(a.getMin(), b.getMin());
  SGVec2<int> m2 = min(a.getMax(), b.getMax());
  return SGRect<int>(m1, m2);
}

  void Image::fillRect(const SGRect<int>& rect, const osg::Vec4& color)
  {
    osg::ref_ptr<osg::Image> image = _texture->getImage();
    if (!image) {
      allocateImage();
      image = _texture->getImage();
    }
      
      if (image->getDataVariance() != osg::Object::DYNAMIC) {
          image->setDataVariance(osg::Object::DYNAMIC);
      }
      
    const auto format = image->getInternalTextureFormat();
    
    auto clippedRect = intersectRect(rect, SGRect<int>(0, 0, image->s(), image->t()));
    if ((clippedRect.width() == 0) || (clippedRect.height() == 0)) {
      return;
    }
    
    GLubyte* rowData = nullptr;
    size_t rowByteSize = 0;
    GLuint pixelWidth = clippedRect.width();
    GLuint pixel = 0;
    GLuint pixelBytes = 0;
    
    switch (format) {
    case GL_RGBA8:
    case GL_RGBA:
        rowByteSize = pixelWidth * 4;
        rowData = static_cast<GLubyte*>(alloca(rowByteSize));
        
        // assume litte-endian, so read out backwards, hence when we memcpy
        // the data, it ends up in RGBA order
        pixel = color.asABGR();
        pixelBytes = 4;
        fillRow(rowData, pixel, pixelWidth, pixelBytes);
        break;
    
    case GL_RGB8:
    case GL_RGB:
        rowByteSize = pixelWidth * 3;
        rowData = static_cast<GLubyte*>(alloca(rowByteSize));
        pixel = color.asABGR();
        pixelBytes = 3;
        fillRow(rowData, pixel, pixelWidth, pixelBytes);
        break;
        
    default:
      SG_LOG(SG_IO, SG_WARN, "Image::fillRect: unsupported internal image format:" << format);
      return;
    }
    
    for (int row=clippedRect.t(); row < clippedRect.b(); ++row) {
      GLubyte* imageData = image->data(clippedRect.l(), row);
      memcpy(imageData, rowData, rowByteSize);
    }
      
    image->dirty();
    auto c = getCanvas().lock();
    c->enableRendering(true); // force a repaint
  }

  void Image::setPixel(int x, int y, const std::string& c)
  {
    osg::Vec4 color(1,1,1,1);
    if(!c.empty() && !parseColor(c, color))
      return;
    
    setPixel(x, y, color);
  }

  void Image::setPixel(int x, int y, const osg::Vec4& color)
  {
    osg::ref_ptr<osg::Image> image = _texture->getImage();
    if (!image) {
        allocateImage();
        image = _texture->getImage();
    }
     
    if (image->getDataVariance() != osg::Object::DYNAMIC) {
      image->setDataVariance(osg::Object::DYNAMIC);
    }
      
    image->setColor(color, x, y);
  }

    void Image::dirtyPixels()
    {
        osg::ref_ptr<osg::Image> image = _texture->getImage();
        if (!image)
            return;
        image->dirty();
        auto c = getCanvas().lock();
        c->enableRendering(true); // force a repaint
    }

  void Image::allocateImage()
  {
    osg::Image* image = new osg::Image;
    // default to RGBA
    image->allocateImage(_node->getIntValue("size[0]"), _node->getIntValue("size[1]"), 1, GL_RGBA, GL_UNSIGNED_BYTE);
    image->setInternalTextureFormat(GL_RGBA);
    _texture->setImage(image);
  }


osg::ref_ptr<osg::Image> Image::getImage() const
{
    if (!_texture)
        return {};
    return _texture->getImage();
}

} // namespace canvas
} // namespace simgear
