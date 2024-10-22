// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2005 Harald Johnsen
// SPDX-FileCopyrightText: 2007 Tim Moore
// SPDX-FileCopyrightText: 2012 Thomas Geymayer <tomgey@gmail.com>

/**
 * @file
 * @brief Owner Drawn Gauge helper class
 *
 * Supports multisampling/mipmapping, usage of the stencil buffer and
 * placing the texture in the scene by certain filter criteria.
 */

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include "ODGauge.hxx"
#include "Canvas.hxx"
#include "CanvasSystemAdapter.hxx"

#include <simgear/debug/logstream.hxx>
#include <simgear/scene/util/RenderConstants.hxx>

#include <osg/Texture2D>
#include <osg/AlphaFunc>
#include <osg/BlendFunc>
#include <osg/Camera>
#include <osg/Matrix>
#include <osg/PolygonMode>
#include <osg/ShadeModel>
#include <osg/StateSet>
#include <osg/FrameBufferObject> // for GL_DEPTH_STENCIL_EXT on Windows
#include <osgUtil/RenderBin>

#include <cassert>

namespace simgear
{
namespace canvas
{

  class PreOrderBin:
    public osgUtil::RenderBin
  {
    public:

      PreOrderBin()
      {}
      PreOrderBin( const RenderBin& rhs,
                   const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY ):
        RenderBin(rhs, copyop)
      {}

      virtual osg::Object* cloneType() const
      {
        return new PreOrderBin();
      }
      virtual osg::Object* clone(const osg::CopyOp& copyop) const
      {
        return new PreOrderBin(*this,copyop);
      }
      virtual bool isSameKindAs(const osg::Object* obj) const
      {
        return dynamic_cast<const PreOrderBin*>(obj) != 0L;
      }
      virtual const char* className() const
      {
        return "PreOrderBin";
      }

      virtual void sort()
      {
        // Do not sort to keep traversal order...
      }
  };

#ifndef OSG_INIT_SINGLETON_PROXY
  /**
   * http://svn.openscenegraph.org/osg/OpenSceneGraph/trunk/include/osg/Object
   *
   * Helper macro that creates a static proxy object to call singleton function
   * on it's construction, ensuring that the singleton gets initialized at
   * startup.
   */
#  define OSG_INIT_SINGLETON_PROXY(ProxyName, Func)\
          static struct ProxyName{ ProxyName() { Func; } } s_##ProxyName;
#endif

  OSG_INIT_SINGLETON_PROXY(
    PreOrderBinProxy,
    (osgUtil::RenderBin::addRenderBinPrototype("PreOrderBin", new PreOrderBin))
  );

  //----------------------------------------------------------------------------
  ODGauge::ODGauge():
    _size_x( -1 ),
    _size_y( -1 ),
    _view_width( -1 ),
    _view_height( -1 ),
    _flags(0),
    _coverage_samples( 0 ),
    _color_samples( 0 )
  {

  }

  //----------------------------------------------------------------------------
  ODGauge::~ODGauge()
  {
    clear();
  }

  //----------------------------------------------------------------------------
  void ODGauge::setSize(int size_x, int size_y)
  {
    _size_x = size_x;
    _size_y = size_y < 0 ? size_x : size_y;

    if (serviceable()) {
      texture->setTextureSize(_size_x, _size_y);
      texture->dirtyTextureObject();

      camera->setViewport(0, 0, _size_x, _size_y);
      camera->dirtyAttachmentMap();
      // We used to recreate the texture and camera when resizing, indirectly
      // enabling rendering. Emulate that behaviour for backwards compatibility.
      setRender(true);

      // The new size might require a different number of mipmaps
      updateSampling();
    }
  }

  //----------------------------------------------------------------------------
  void ODGauge::setViewSize(int width, int height)
  {
    _view_width = width;
    _view_height = height < 0 ? width : height;

    if (camera)
      updateCoordinateFrame();
  }

  //----------------------------------------------------------------------------
  osg::Vec2s ODGauge::getViewSize() const
  {
    return osg::Vec2s(_view_width, _view_height);
  }

  //----------------------------------------------------------------------------
  void ODGauge::useImageCoords(bool use)
  {
    if( updateFlag(USE_IMAGE_COORDS, use) && camera )
      updateCoordinateFrame();
  }

  //----------------------------------------------------------------------------
  void ODGauge::useStencil(bool use)
  {
    if( updateFlag(USE_STENCIL, use) && camera )
      updateStencil();
  }

  //----------------------------------------------------------------------------
  void ODGauge::useAdditiveBlend(bool use)
  {
    if( updateFlag(USE_ADDITIVE_BLEND, use) && camera )
      updateBlendMode();
  }

  //----------------------------------------------------------------------------
  void ODGauge::setSampling( bool mipmapping,
                             int coverage_samples,
                             int color_samples )
  {
    if(    !updateFlag(USE_MIPMAPPING, mipmapping)
        && _coverage_samples == coverage_samples
        && _color_samples == color_samples )
      return;

    if( color_samples > coverage_samples )
    {
      SG_LOG
      (
        SG_GL,
        SG_WARN,
        "ODGauge::setSampling: color_samples > coverage_samples not allowed!"
      );
      color_samples = coverage_samples;
    }

    _coverage_samples = coverage_samples;
    _color_samples = color_samples;

    if (camera && texture)
      updateSampling();
  }

  //----------------------------------------------------------------------------
  void ODGauge::setMaxAnisotropy(float anis)
  {
    texture->setMaxAnisotropy(anis);
  }

  //----------------------------------------------------------------------------
  void ODGauge::setRender(bool render)
  {
    camera->setNodeMask(render ? 0xffffffff : 0);
  }

  //----------------------------------------------------------------------------
  bool ODGauge::serviceable() const
  {
    return _flags & AVAILABLE;
  }

  //----------------------------------------------------------------------------
  void ODGauge::allocRT(osg::NodeCallback* camera_cull_callback)
  {
    // Make sure everything is initialized from scratch
    clear();

    camera = new osg::Camera;
    camera->setDataVariance(osg::Object::DYNAMIC);
    camera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
    // Do not resize the projection matrix automatically. We do this manually in
    // updateCoordinateFrame().
    camera->setProjectionResizePolicy(osg::Camera::FIXED);
    camera->setRenderOrder(osg::Camera::PRE_RENDER);
    camera->setClearColor(osg::Vec4(0.0f, 0.0f, 0.0f, 0.0f));
    camera->setClearStencil(0);
    camera->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT);
    camera->setViewport(0, 0, _size_x, _size_y);

    updateCoordinateFrame();
    updateStencil();

    if (camera_cull_callback) {
      camera->setCullCallback(camera_cull_callback);
    }

    osg::StateSet* stateSet = camera->getOrCreateStateSet();
    stateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
    stateSet->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);
    stateSet->setAttributeAndModes(
      new osg::PolygonMode(osg::PolygonMode::FRONT_AND_BACK,
                           osg::PolygonMode::FILL),
      osg::StateAttribute::ON);

    texture = new osg::Texture2D;
    texture->setUseHardwareMipMapGeneration(true);
    texture->setResizeNonPowerOfTwoHint(false);
    texture->setTextureSize(_size_x, _size_y);
    texture->setInternalFormat(GL_RGBA8);
    texture->setSourceFormat(GL_RGBA);
    texture->setSourceType(GL_UNSIGNED_BYTE);

    updateSampling();
    updateBlendMode();

    if( Canvas::getSystemAdapter() )
      Canvas::getSystemAdapter()->addCamera(camera.get());

    _flags |= AVAILABLE;
  }

  //----------------------------------------------------------------------------
  void ODGauge::clear()
  {
    if( camera.valid() && Canvas::getSystemAdapter() )
      Canvas::getSystemAdapter()->removeCamera(camera.get());
    camera = nullptr;
    texture = nullptr;

    _flags &= ~AVAILABLE;
  }

  //----------------------------------------------------------------------------
  bool ODGauge::updateFlag(Flags flag, bool enable)
  {
    if( bool(_flags & flag) == enable )
      return false;

    _flags ^= flag;
    return true;
  }

  //----------------------------------------------------------------------------
  void ODGauge::updateCoordinateFrame()
  {
    assert( camera );

    if( _view_width < 0 )
      _view_width = _size_x;
    if( _view_height < 0 )
      _view_height = _size_y;

    if( _flags & USE_IMAGE_COORDS ) {
      camera->setProjectionMatrix(
        osg::Matrix::ortho2D(0.0, _view_width, _view_height, 0.0));
    } else {
      camera->setProjectionMatrix(
        osg::Matrix::ortho2D(-_view_width *0.5, _view_width *0.5,
                             -_view_height*0.5, _view_height*0.5));
    }
  }

  //----------------------------------------------------------------------------
  void ODGauge::updateStencil()
  {
    assert( camera );

    GLbitfield mask = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT;

    if( _flags & USE_STENCIL )
    {
      // REVIEW: Memory Leak - 264 bytes in 3 blocks are indirectly lost
      camera->attach( osg::Camera::PACKED_DEPTH_STENCIL_BUFFER,
                       GL_DEPTH_STENCIL_EXT );
      mask |= GL_STENCIL_BUFFER_BIT;
    }
    else
    {
      camera->detach(osg::Camera::PACKED_DEPTH_STENCIL_BUFFER);
    }

    camera->setClearMask(mask);
  }

  //----------------------------------------------------------------------------
  void ODGauge::updateSampling()
  {
    assert( camera );
    assert( texture );

    int mipmap_levels = (_flags & USE_MIPMAPPING) ?
      osg::Image::computeNumberOfMipmapLevels(_size_x, _size_y, 1) : 0;
    texture->setNumMipmapLevels(mipmap_levels);
    texture->setFilter(
      osg::Texture2D::MIN_FILTER,
      (_flags & USE_MIPMAPPING) ? osg::Texture2D::LINEAR_MIPMAP_LINEAR
                                : osg::Texture2D::LINEAR
    );
    camera->attach(
      osg::Camera::COLOR_BUFFER0,
      texture.get(),
      0, 0,
      _flags & USE_MIPMAPPING,
      _coverage_samples,
      _color_samples
    );
  }

  //----------------------------------------------------------------------------
  void ODGauge::updateBlendMode()
  {
    assert( camera );

    camera->getOrCreateStateSet()
      ->setAttributeAndModes
      (
        (_flags & USE_ADDITIVE_BLEND)
          ? new osg::BlendFunc( osg::BlendFunc::SRC_ALPHA,
                                osg::BlendFunc::ONE_MINUS_SRC_ALPHA,
                                osg::BlendFunc::ONE,
                                osg::BlendFunc::ONE )
          : new osg::BlendFunc( osg::BlendFunc::SRC_ALPHA,
                                osg::BlendFunc::ONE_MINUS_SRC_ALPHA )
      );
  }

} // namespace canvas
} // namespace simgear
