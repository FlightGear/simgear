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
#include <simgear/scene/util/parse_color.hxx>
#include <simgear/misc/sg_path.hxx>

#include <osg/Array>
#include <osg/Geometry>
#include <osg/PrimitiveSet>

#include <boost/algorithm/string/predicate.hpp>

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
                SGPropertyNode_ptr node,
                const Style& parent_style ):
    Element(canvas, node, parent_style),
    _texture(new osg::Texture2D),
    _node_src_rect( node->getNode("source", 0, true) )
  {
    _geom = new osg::Geometry;
    _geom->setUseDisplayList(false);

    osg::StateSet *stateSet = _geom->getOrCreateStateSet();
    stateSet->setTextureAttributeAndModes(0, _texture.get());
    stateSet->setDataVariance(osg::Object::STATIC);

    // allocate arrays for the image
    _vertices = new osg::Vec3Array(4);
    _vertices->setDataVariance(osg::Object::STATIC);
    _geom->setVertexArray(_vertices);

    _texCoords = new osg::Vec2Array(4);
    _texCoords->setDataVariance(osg::Object::STATIC);
    _geom->setTexCoordArray(0, _texCoords);

    _colors = new osg::Vec4Array(4);
    _colors->setDataVariance(osg::Object::STATIC);
    _geom->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    _geom->setColorArray(_colors);

    osg::DrawArrays* prim = new osg::DrawArrays(osg::PrimitiveSet::QUADS);
    prim->set(osg::PrimitiveSet::QUADS, 0, 4);
    prim->setDataVariance(osg::Object::STATIC);
    _geom->addPrimitiveSet(prim);

    setDrawable(_geom);

    addStyle("fill", &Image::setFill, this);
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

    if( _attributes_dirty & DEST_SIZE )
    {
      (*_vertices)[0].set(_region.l(), _region.t(), 0);
      (*_vertices)[1].set(_region.r(), _region.t(), 0);
      (*_vertices)[2].set(_region.r(), _region.b(), 0);
      (*_vertices)[3].set(_region.l(), _region.b(), 0);
      _vertices->dirty();

      _attributes_dirty &= ~DEST_SIZE;
      _geom->dirtyBound();
      setBoundingBox(_geom->getBound());
    }

    if( _attributes_dirty & SRC_RECT )
    {
      double u0 = _src_rect.l(),
             u1 = _src_rect.r(),
             v0 = _src_rect.b(),
             v1 = _src_rect.t();

      if( !_node_src_rect->getBoolValue("normalized", true) )
      {
        const Rect<int>& tex_dim = getTextureDimensions();

        u0 /= tex_dim.width();
        u1 /= tex_dim.width();
        v0 /= tex_dim.height();
        v1 /= tex_dim.height();
      }

      (*_texCoords)[0].set(u0, v0);
      (*_texCoords)[1].set(u1, v0);
      (*_texCoords)[2].set(u1, v1);
      (*_texCoords)[3].set(u0, v1);
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

    for( int i = 0; i < 4; ++i )
      (*_colors)[i] = color;
    _colors->dirty();
  }

  //----------------------------------------------------------------------------
  const Rect<float>& Image::getRegion() const
  {
    return _region;
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
      const Rect<int>& tex_dim = getTextureDimensions();

      _node_src_rect->setBoolValue("normalized", false);
      _node_src_rect->setFloatValue("right", tex_dim.width());
      _node_src_rect->setFloatValue("bottom", tex_dim.height());
    }

    if( !_region.width() || !_region.height() )
    {
      _node->setFloatValue("size[0]", _src_rect.width());
      _node->setFloatValue("size[1]", _src_rect.height());
    }
  }

  //----------------------------------------------------------------------------
  Rect<int> Image::getTextureDimensions() const
  {
    osg::Texture2D *texture = !_src_canvas.expired()
                            ? _src_canvas.lock()->getTexture()
                            : _texture.get();

    if( !texture )
      return Rect<int>();

    return Rect<int>
    (
      0,0,
      texture->getTextureWidth(),
      texture->getTextureHeight()
    );
  }

} // namespace canvas
} // namespace simgear
