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
#include <simgear/misc/strutils.hxx>

// forward decls
class SGPickCallback;
class SGSceneUserData;

//////////////////////////////////////////////////////////////////////
// Pick animation
//////////////////////////////////////////////////////////////////////

class SGPickAnimation : public SGAnimation {
public:
  SGPickAnimation(const SGPropertyNode* configNode,
                  SGPropertyNode* modelRoot);
    
  // override so we can treat object-name specially
  virtual void apply(osg::Group& group);
    
  void apply(osg::Node* node);
protected:

      
  virtual osg::Group* createMainGroup(osg::Group* pr);
  
  virtual void setupCallbacks(SGSceneUserData* ud, osg::Group* parent);
private:
  class PickCallback;
  class VncCallback;
  
  string_list _proxyNames;
};


class SGKnobAnimation : public SGPickAnimation
{
public:
    SGKnobAnimation(const SGPropertyNode* configNode,
                    SGPropertyNode* modelRoot);

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
    
    
    /**
     * Scale the drag sensitivity. This provides a global hook for
     * the user to scale the senstivity of dragging according to
     * personal preference.
     */
    static void setDragSensitivity(double aFactor);
    
    
protected:
    virtual osg::Group* createMainGroup(osg::Group* pr);
      
    virtual void setupCallbacks(SGSceneUserData* ud, osg::Group* parent);
    
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
    
    
protected:
    virtual osg::Group* createMainGroup(osg::Group* pr);
    
    virtual void setupCallbacks(SGSceneUserData* ud, osg::Group* parent);
    
private:
    class UpdateCallback;
    
    SGVec3d _axis;
    SGSharedPtr<SGExpressiond const> _animationValue;
};

#endif // of SG_SCENE_PICK_ANIMATION_HXX

