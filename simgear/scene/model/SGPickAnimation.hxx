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

// forward decls
class SGPickCallback;

//////////////////////////////////////////////////////////////////////
// Pick animation
//////////////////////////////////////////////////////////////////////

class SGPickAnimation : public SGAnimation {
public:
  SGPickAnimation(const SGPropertyNode* configNode,
                  SGPropertyNode* modelRoot);
  virtual osg::Group* createAnimationGroup(osg::Group& parent);
    
    
protected:
    void innerSetupPickGroup(osg::Group* commonGroup, osg::Group& parent);
    
private:
  class PickCallback;
  class VncCallback;
};


class SGKnobAnimation : public SGPickAnimation
{
public:
    SGKnobAnimation(const SGPropertyNode* configNode,
                    SGPropertyNode* modelRoot);
    virtual osg::Group* createAnimationGroup(osg::Group& parent);

    /**
     * by default mouse wheel up corresponds to increment (CW)
     * and mouse-wheel down corresponds to decrement (CCW).
     * Since no one can agree on that, make it a global toggle.
     */
    static void setAlternateMouseWheelDirection(bool aToggle);
    
    /**
     * by default mouse is dragged left-right to change knobs.
     * set this to true to default to up-down. Individual knobs
     * can overrider this,
     */
    static void setAlternateDragAxis(bool aToggle);
private:
    class UpdateCallback;
    
    SGVec3d _axis;
    SGVec3d _center;
    SGSharedPtr<SGExpressiond const> _animationValue;
};

class SGSliderAnimation : public SGPickAnimation
{
public:
    SGSliderAnimation(const SGPropertyNode* configNode,
                    SGPropertyNode* modelRoot);
    virtual osg::Group* createAnimationGroup(osg::Group& parent);
    
private:
    class UpdateCallback;
    
    SGVec3d _axis;
    SGSharedPtr<SGExpressiond const> _animationValue;
};

#endif // of SG_SCENE_PICK_ANIMATION_HXX

