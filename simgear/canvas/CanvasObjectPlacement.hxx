// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2013 Thomas Geymayer <tomgey@gmail.com>

/**
 * @file
 * @brief Canvas placement for placing a canvas texture onto osg objects
 */

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

      bool childChanged(SGPropertyNode* child) override;

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
