// Owner Drawn Gauge helper class
//
// Written by Harald JOHNSEN, started May 2005.
//
// Copyright (C) 2005  Harald JOHNSEN - hjohnsen@evc.net
//
// Ported to OSG by Tim Moore - Jun 2007
//
// Heavily modified to be usable for the 2d Canvas by Thomas Geymayer - April 2012
// Supports now multisampling/mipmapping, usage of the stencil buffer and placing
// the texture in the scene by certain filter criteria
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

#ifndef _SG_OD_GAUGE_HXX
#define _SG_OD_GAUGE_HXX

#include "canvas_fwd.hxx"

#include <osg/NodeCallback>
#include <osg/Group>

namespace osg
{
  class Camera;
  class Texture2D;
}

namespace simgear
{
namespace canvas
{

  /**
   * Owner Drawn Gauge (aka render-to-texture) helper class
   */
  class ODGauge
  {
    public:

      ODGauge();
      virtual ~ODGauge();

      /**
       * Set the size of the render target.
       *
       * @param size_x    X size
       * @param size_y    Y size. Defaults to size_x if not specified
       */
      void setSize(int size_x, int size_y = -1);

      /**
       * Set the size of the viewport
       *
       * @param width
       * @param height    Defaults to width if not specified
       */
      void setViewSize(int width, int height = -1);

      osg::Vec2s getViewSize() const;

      /**
       * DEPRECATED
       *
       * Get size of squared texture
       */
      int size() const { return _size_x; }

      /**
       * Set whether to use image coordinates or not.
       *
       * Default: origin == center of texture
       * Image Coords: origin == top left corner
       */
      void useImageCoords(bool use = true);

      /**
       * Enable/Disable using a stencil buffer
       */
      void useStencil(bool use = true);

      /**
       * Enable/Disable additive alpha blending (Can improve results with
       * transparent background)
       */
      void useAdditiveBlend(bool use = true);

      /**
       * Set sampling parameters for mipmapping and coverage sampling
       * antialiasing.
       *
       * @note color_samples is not allowed to be higher than coverage_samples
       *
       */
      void setSampling( bool mipmapping,
                        int coverage_samples = 0,
                        int color_samples = 0 );

      /**
       * Enable/Disable updating the texture (If disabled the contents of the
       * texture remains with the outcome of the last rendering pass)
       */
      void setRender(bool render);

      /**
       * Say if we can render to a texture.
       * @return true if rtt is available
       */
      bool serviceable() const;

      /**
       * Get the OSG camera for drawing this gauge.
       */
      osg::Camera* getCamera() const { return camera.get(); }

      osg::Texture2D* getTexture() const { return texture.get(); }

      // Real initialization function. Bad name.
      void allocRT(osg::NodeCallback* camera_cull_callback = 0);
      void reinit();
      void clear();

    protected:

      int _size_x,
          _size_y,
          _view_width,
          _view_height;

      enum Flags
      {
        AVAILABLE           = 1,
        USE_IMAGE_COORDS    = AVAILABLE << 1,
        USE_STENCIL         = USE_IMAGE_COORDS << 1,
        USE_MIPMAPPING      = USE_STENCIL << 1,
        USE_ADDITIVE_BLEND  = USE_MIPMAPPING << 1
      };

      uint32_t _flags;

      // Multisampling parameters
      int  _coverage_samples,
           _color_samples;

      osg::ref_ptr<osg::Camera> camera;
      osg::ref_ptr<osg::Texture2D> texture;

      bool updateFlag(Flags flag, bool enable);

      void updateCoordinateFrame();
      void updateStencil();
      void updateSampling();
      void updateBlendMode();

  };

} // namespace canvas
} // namespace simgear

#endif // _SG_OD_GAUGE_HXX
