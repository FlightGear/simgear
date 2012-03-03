/* -*-c++-*-
 *
 * Copyright (C) 2006-2009 Mathias Froehlich 
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
#include <osg/PagedLOD>
#include <osgUtil/UpdateVisitor>

#include "OsgMath.hxx"

class SGUpdateVisitor : public osgUtil::UpdateVisitor {
public:
  SGUpdateVisitor() :
    mVisibility(-1)
  {
    setTraversalMode(osg::NodeVisitor::TRAVERSE_ACTIVE_CHILDREN);
    setVisibility(10000);
  }
  void setViewData(const SGVec3d& globalEyePos,
                   const SGQuatd& globalViewOrientation)
  {
    mGlobalGeodEyePos = SGGeod::fromCart(globalEyePos);
    _currentEyePos = toOsg(globalEyePos);
    mGlobalEyePos = globalEyePos;
    mGlobalViewOr = globalViewOrientation;
    mGlobalHorizLocalOr = SGQuatd::fromLonLat(mGlobalGeodEyePos);
    mHorizLocalNorth = mGlobalHorizLocalOr.backTransform(SGVec3d(1, 0, 0));
    mHorizLocalEast = mGlobalHorizLocalOr.backTransform(SGVec3d(0, 1, 0));
    mHorizLocalDown = mGlobalHorizLocalOr.backTransform(SGVec3d(0, 0, 1));
  }

  void setVisibility(double visibility)
  {
    if (mVisibility == visibility)
      return;
    mVisibility = visibility;
    mSqrVisibility = visibility*visibility;

    double m_log01 = -log( 0.01 );
    double sqrt_m_log01 = sqrt( m_log01 );
    double fog_exp_density = m_log01 / visibility;
    double fog_exp2_density = sqrt_m_log01 / visibility;
    double ground_exp2_punch_through = sqrt_m_log01 / (visibility * 1.5);
    double rwy_exp2_punch_through, taxi_exp2_punch_through;
    if ( visibility < 8000 ) {
      rwy_exp2_punch_through = sqrt_m_log01 / (visibility * 2.5);
      taxi_exp2_punch_through = sqrt_m_log01 / (visibility * 1.5);
    } else {
      rwy_exp2_punch_through = sqrt_m_log01 / ( 8000 * 2.5 );
      taxi_exp2_punch_through = sqrt_m_log01 / ( 8000 * 1.5 );
    }
    
    mFogExpDensity = fog_exp_density;
    mFogExp2Density = fog_exp2_density;
    mRunwayFogExp2Density = rwy_exp2_punch_through;
    mTaxiFogExp2Density = taxi_exp2_punch_through;
    mGroundLightsFogExp2Density = ground_exp2_punch_through;
  }

  double getVisibility() const
  { return mVisibility; }
  double getSqrVisibility() const
  { return mSqrVisibility; }

  double getFogExpDensity() const
  { return mFogExpDensity; }
  double getFogExp2Density() const
  { return mFogExp2Density; }
  double getRunwayFogExp2Density() const
  { return mRunwayFogExp2Density; }
  double getTaxiFogExp2Density() const
  { return mTaxiFogExp2Density; }
  double getGroundLightsFogExp2Density() const
  { return mGroundLightsFogExp2Density; }

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
                const SGVec4f& diffuse, const SGVec4f& specular,
                const SGVec4f& fogColor, double sunAngleDeg)
  {
    mLightDirection = direction;
    mAmbientLight = ambient;
    mDiffuseLight = diffuse;
    mSpecularLight = specular;
    mFogColor = fogColor;
    mSunAngleDeg = sunAngleDeg;
  }

  const SGVec3f& getLightDirection() const
  { return mLightDirection; }
  const SGVec4f& getAmbientLight() const
  { return mAmbientLight; }
  const SGVec4f& getDiffuseLight() const
  { return mDiffuseLight; }
  const SGVec4f& getSpecularLight() const
  { return mSpecularLight; }
  const SGVec4f& getFogColor() const
  { return mFogColor; }

  double getSunAngleDeg() const
  { return mSunAngleDeg; }

  virtual void apply(osg::Node& node)
  {
    if (!needToEnterNode(node))
      return;
    osgUtil::UpdateVisitor::apply(node);
  }
  // To avoid expiry of LOD nodes that are in range and that are updated,
  // mark them with the last traversal number, even if they are culled away
  // by the cull frustum.
  virtual void apply(osg::PagedLOD& pagedLOD)
  {
    if (!needToEnterNode(pagedLOD))
      return;
    if (getFrameStamp())
      pagedLOD.setFrameNumberOfLastTraversal(getFrameStamp()->getFrameNumber());
    osgUtil::UpdateVisitor::apply(pagedLOD);
  }
  // To be able to traverse correctly only the active children, we need to
  // track the model view matrices during update.
  virtual void apply(osg::Transform& transform)
  {
    if (!needToEnterNode(transform))
      return;
    osg::Matrix matrix = _matrix;
    transform.computeLocalToWorldMatrix(_matrix, this);
    osgUtil::UpdateVisitor::apply(transform);
    _matrix = matrix;
  }
  virtual void apply(osg::Camera& camera)
  {
    if (!needToEnterNode(camera))
      return;
    if (camera.getReferenceFrame() == osg::Camera::ABSOLUTE_RF) {
      osg::Vec3d currentEyePos = _currentEyePos;
      _currentEyePos = osg::Vec3d(0, 0, 0);
      apply(static_cast<osg::Transform&>(camera));
      _currentEyePos = currentEyePos;
    } else {
      apply(static_cast<osg::Transform&>(camera));
    }
  }
  // Function to make the LOD traversal only enter that children that
  // are visible on the screen.
  virtual float getDistanceToViewPoint(const osg::Vec3& pos, bool) const
  { return (_currentEyePos - _matrix.preMult(osg::Vec3d(pos))).length(); }

protected:
  bool needToEnterNode(const osg::Node& node) const
  {
    if (!node.isCullingActive())
      return true;
    return isSphereInRange(node.getBound());
  }
  bool isSphereInRange(const osg::BoundingSphere& sphere) const
  {
    if (!sphere.valid())
      return false;
    float maxDist = mVisibility + sphere._radius;
    osg::Vec3d center = _matrix.preMult(osg::Vec3d(sphere._center));
    return (_currentEyePos - center).length2() <= maxDist*maxDist;
  }
    
private:
  osg::Matrix _matrix;
  osg::Vec3d _currentEyePos;

  SGGeod mGlobalGeodEyePos;
  SGVec3d mGlobalEyePos;
  SGQuatd mGlobalViewOr;
  SGQuatd mGlobalHorizLocalOr;
  SGVec3d mHorizLocalNorth;
  SGVec3d mHorizLocalEast;
  SGVec3d mHorizLocalDown;

  double mVisibility;
  double mSqrVisibility;
  double mFogExpDensity;
  double mFogExp2Density;
  double mRunwayFogExp2Density;
  double mTaxiFogExp2Density;
  double mGroundLightsFogExp2Density;

  SGVec3f mLightDirection;
  SGVec4f mAmbientLight;
  SGVec4f mDiffuseLight;
  SGVec4f mSpecularLight;
  SGVec4f mFogColor;

  double mSunAngleDeg;
};

#endif
