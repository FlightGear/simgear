/* -*-c++-*-
 *
 * Copyright (C) 2006 Mathias Froehlich 
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

#ifndef SG_SCENE_UPDATEVISITOR_HXX
#define SG_SCENE_UPDATEVISITOR_HXX

#include <osg/NodeVisitor>
#include <osgUtil/UpdateVisitor>

#include "simgear/math/SGMath.hxx"

class SGUpdateVisitor : public osgUtil::UpdateVisitor {
public:
  SGUpdateVisitor()
  {
    // Need to traverse all children, else some LOD nodes do not get updated
    // Note that the broad number of updates is not done due to
    // the update callback in the global position node.
    setTraversalMode(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN);
  }
  void setViewData(const SGVec3d& globalEyePos,
                   const SGQuatd& globalViewOrientation)
  {
    mGlobalGeodEyePos = SGGeod::fromCart(globalEyePos);
    mGlobalEyePos = globalEyePos;
    mGlobalViewOr = globalViewOrientation;
    mGlobalHorizLocalOr = SGQuatd::fromLonLat(mGlobalGeodEyePos);
    mHorizLocalNorth = mGlobalHorizLocalOr.backTransform(SGVec3d(1, 0, 0));
    mHorizLocalEast = mGlobalHorizLocalOr.backTransform(SGVec3d(0, 1, 0));
    mHorizLocalDown = mGlobalHorizLocalOr.backTransform(SGVec3d(0, 0, 1));
  }

  void setSceneryCenter(const SGVec3d& sceneryCenter)
  {
    mSceneryCenter = sceneryCenter;
  }

  void setVisibility(double visibility)
  {
    mVisibility = visibility;
    mSqrVisibility = visibility*visibility;
  }

  double getVisibility() const
  { return mVisibility; }
  double getSqrVisibility() const
  { return mSqrVisibility; }

  const SGVec3d& getGlobalEyePos() const
  { return mGlobalEyePos; }
  const SGGeod& getGeodEyePos() const
  { return mGlobalGeodEyePos; }
  const SGQuatd& getGlobalViewOr() const
  { return mGlobalViewOr; }
  const SGQuatd& getGlobalHorizLocalOr() const
  { return mGlobalViewOr; }
  const SGVec3d& getHorizLocalNorth() const
  { return mHorizLocalNorth; }
  const SGVec3d& getHorizLocalEast() const
  { return mHorizLocalEast; }
  const SGVec3d& getHorizLocalDown() const
  { return mHorizLocalDown; }

  void setLight(const SGVec3f& direction, const SGVec4f& ambient,
                const SGVec4f& diffuse, const SGVec4f& specular)
  {
    mLightDirection = direction;
    mAmbientLight = ambient;
    mDiffuseLight = diffuse;
  }

  const SGVec3f& getLightDirection() const
  { return mLightDirection; }
  const SGVec4f& getAmbientLight() const
  { return mAmbientLight; }
  const SGVec4f& getDiffuseLight() const
  { return mDiffuseLight; }
  const SGVec4f& getSpecularLight() const
  { return mSpecularLight; }

private:
  SGGeod mGlobalGeodEyePos;
  SGVec3d mGlobalEyePos;
  SGQuatd mGlobalViewOr;
  SGQuatd mGlobalHorizLocalOr;
  SGVec3d mHorizLocalNorth;
  SGVec3d mHorizLocalEast;
  SGVec3d mHorizLocalDown;

  SGVec3d mSceneryCenter;

  double mVisibility;
  double mSqrVisibility;

  SGVec3f mLightDirection;
  SGVec4f mAmbientLight;
  SGVec4f mDiffuseLight;
  SGVec4f mSpecularLight;
};

#endif
