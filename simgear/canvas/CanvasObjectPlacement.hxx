///@file
/// Placement for putting a canvas texture onto OpenSceneGraph objects.
///
/// It also provides a SGPickCallback for passing mouse events to the canvas and
/// manages emissive lighting of the placed canvas.
//
// Copyright (C) 2013  Thomas Geymayer <tomgey@gmail.com>
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

#ifndef CANVAS_OBJECT_PLACEMENT_HXX_
#define CANVAS_OBJECT_PLACEMENT_HXX_

#include "CanvasPlacement.hxx"
#include "canvas_fwd.hxx"

#include <simgear/scene/util/SGSceneUserData.hxx>
#include <osg/Material>

namespace simgear
{
namespace canvas
{

  /**
   * Place a Canvas onto an osg object (as texture).
   */
  class ObjectPlacement:
    public Placement
  {
    public:

      typedef osg::ref_ptr<osg::Group> GroupPtr;
      typedef osg::ref_ptr<osg::Material> MaterialPtr;

      ObjectPlacement( SGPropertyNode* node,
                       const GroupPtr& group,
                       const CanvasWeakPtr& canvas );
      virtual ~ObjectPlacement();

      /**
       * Set emissive lighting of the object the canvas is placed on.
       */
      void setEmission(float emit);

      /**
       * Set whether pick events should be captured.
       */
      void setCaptureEvents(bool enable);

      virtual bool childChanged(SGPropertyNode* child);

    protected:
      typedef SGSharedPtr<SGPickCallback> PickCallbackPtr;
      typedef osg::ref_ptr<SGSceneUserData> SGSceneUserDataPtr;

      GroupPtr            _group;
      MaterialPtr         _material;
      CanvasWeakPtr       _canvas;
      PickCallbackPtr     _pick_cb;
      SGSceneUserDataPtr  _scene_user_data;
  };

} // namespace canvas
} // namespace simgear

#endif /* CANVAS_OBJECT_PLACEMENT_HXX_ */
