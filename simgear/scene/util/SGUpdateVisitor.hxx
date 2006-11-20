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

class SGUpdateVisitor : public osgUtil::UpdateVisitor {
public:
  SGUpdateVisitor()
  {
    setTraversalMode(osg::NodeVisitor::TRAVERSE_ACTIVE_CHILDREN);
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
};

#endif
