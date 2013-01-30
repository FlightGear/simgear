/* -*-c++-*-
 *
 * Copyright (C) 2013 James Turner
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */
     
#ifndef SG_SCENE_PICK_ANIMATION_HXX
#define SG_SCENE_PICK_ANIMATION_HXX

#include <simgear/scene/model/animation.hxx>


//////////////////////////////////////////////////////////////////////
// Pick animation
//////////////////////////////////////////////////////////////////////

class SGPickAnimation : public SGAnimation {
public:
  SGPickAnimation(const SGPropertyNode* configNode,
                  SGPropertyNode* modelRoot);
  virtual osg::Group* createAnimationGroup(osg::Group& parent);
private:
  class PickCallback;
  class VncCallback;
};

#endif // of SG_SCENE_PICK_ANIMATION_HXX

