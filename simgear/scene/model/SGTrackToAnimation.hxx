// TrackTo animation
//
// http://wiki.blender.org/index.php/Doc:2.6/Manual/Constraints/Tracking/Locked_Track
// TODO: http://wiki.blender.org/index.php/Doc:2.6/Manual/Constraints/Tracking/Track_To
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

#ifndef SG_TRACK_TO_ANIMATION_HXX_
#define SG_TRACK_TO_ANIMATION_HXX_

#include <simgear/scene/model/animation.hxx>

/**
 * Animation to let an object always track another object. An optional second
 * slave object can be specified which is rotate to always fit the space between
 * the root object and the target object. This can be used to eg. create a gear
 * scissor animation.
 */
class SGTrackToAnimation:
  public SGAnimation
{
  public:
    SGTrackToAnimation( osg::Node* node,
                        const SGPropertyNode* configNode,
                        SGPropertyNode* modelRoot );

    virtual osg::Group* createAnimationGroup(osg::Group& parent);

  protected:
    class UpdateCallback;

    osg::Group     *_target_group,
                   *_slave_group;

    void log(sgDebugPriority p, const std::string& msg) const;
};

#endif /* SG_TRACK_TO_ANIMATION_HXX_ */
