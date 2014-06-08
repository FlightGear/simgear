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

#ifndef CANVAS_IMAGE_HXX_
#define CANVAS_IMAGE_HXX_

#include "CanvasElement.hxx"

#include <simgear/canvas/canvas_fwd.hxx>
#include <simgear/io/HTTPClient.hxx>
#include <simgear/misc/CSSBorder.hxx>
#include <simgear/misc/SVGpreserveAspectRatio.hxx>
#include <osg/Texture2D>

namespace simgear
{
namespace HTTP { class Request; }
namespace canvas
{

  class Image:
    public Element
  {
    public:
      static const std::string TYPE_NAME;
      static void staticInit();

      /**
       * @param node    Property node containing settings for this image:
       *                  rect/[left/right/top/bottom]  Dimensions of source
       *                                                rect
       *                  size[0-1]                     Dimensions of rectangle
       *                  [x,y]                         Position of rectangle
       */
      Image( const CanvasWeakPtr& canvas,
             const SGPropertyNode_ptr& node,
             const Style& parent_style = Style(),
             Element* parent = 0 );
      virtual ~Image();

      virtual void update(double dt);
      virtual void valueChanged(SGPropertyNode* child);

      void setSrcCanvas(CanvasPtr canvas);
      CanvasWeakPtr getSrcCanvas() const;

      void setImage(osg::Image *img);
      void setFill(const std::string& fill);

      /**
       * @see http://www.w3.org/TR/css3-background/#border-image-outset
       */
      void setOutset(const std::string& outset);

      /**
       * @see
       *   http://www.w3.org/TR/SVG11/coords.html#PreserveAspectRatioAttribute
       */
      void setPreserveAspectRatio(const std::string& scale);

      /**
       * Set image slice (aka. 9-scale)
       *
       * @see http://www.w3.org/TR/css3-background/#border-image-slice
       */
      void setSlice(const std::string& slice);

      /**
       * Set image slice width.
       *
       * By default the size of the 9-scale grid is the same as specified
       * with setSlice/"slice". Using this method allows setting values
       * different to the size in the source image.
       *
       * @see http://www.w3.org/TR/css3-background/#border-image-width
       */
      void setSliceWidth(const std::string& width);

      const SGRect<float>& getRegion() const;

      bool handleEvent(const EventPtr& event);

    protected:

      enum ImageAttributes
      {
        SRC_RECT       = LAST_ATTRIBUTE << 1, // Source image rectangle
        DEST_SIZE      = SRC_RECT << 1,       // Element size
        SRC_CANVAS     = DEST_SIZE << 1
      };

      virtual void childChanged(SGPropertyNode * child);

      void setupDefaultDimensions();
      SGRect<int> getTextureDimensions() const;

      void setQuad(size_t index, const SGVec2f& tl, const SGVec2f& br);
      void setQuadUV(size_t index, const SGVec2f& tl, const SGVec2f& br);

      void handleImageLoadDone(HTTP::Request*);
      bool loadImage( osgDB::ReaderWriter& reader,
                      const std::string& data,
                      HTTP::Request& request,
                      const std::string& type );

      osg::ref_ptr<osg::Texture2D> _texture;
      // TODO optionally forward events to canvas
      CanvasWeakPtr _src_canvas;
      HTTP::Request_ptr _http_request;

      osg::ref_ptr<osg::Geometry>  _geom;
      osg::ref_ptr<osg::DrawArrays>_prim;
      osg::ref_ptr<osg::Vec3Array> _vertices;
      osg::ref_ptr<osg::Vec2Array> _texCoords;
      osg::ref_ptr<osg::Vec4Array> _colors;

      SGPropertyNode *_node_src_rect;
      SGRect<float>   _src_rect,
                      _region;

      SVGpreserveAspectRatio _preserve_aspect_ratio;

      CSSBorder       _outset,
                      _slice,
                      _slice_width;
  };

} // namespace canvas
} // namespace canvas

#endif /* CANVAS_IMAGE_HXX_ */
