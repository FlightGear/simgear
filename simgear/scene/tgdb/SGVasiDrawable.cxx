/* -*-c++-*-
 *
 * Copyright (C) 2006-2007 Mathias Froehlich 
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

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include <simgear/compiler.h>

#include "SGVasiDrawable.hxx"

#include <simgear/scene/util/OsgMath.hxx>

struct SGVasiDrawable::LightData {
  LightData(const SGVec3f& p, const SGVec3f& n, const SGVec3f& up) :
    position(p),
    normal(n),
    horizontal(normalize(cross(up, n))),
    normalCrossHorizontal(normalize(cross(n, horizontal)))
  { }
  
  void draw(const SGVec4f& color) const
  {
    glBegin(GL_POINTS);
    glColor4fv(color.data());
    glNormal3fv(normal.data());
    glVertex3fv(position.data());
    glEnd();
  }
  
  SGVec3f position;
  SGVec3f normal;
  SGVec3f horizontal;
  SGVec3f normalCrossHorizontal;
};

SGVasiDrawable::SGVasiDrawable(const SGVasiDrawable& vd, const osg::CopyOp&) :
  _lights(vd._lights),
  _red(vd._red),
  _white(vd._white)
{
  setUseDisplayList(false);
  setSupportsDisplayList(false);
}

SGVasiDrawable::SGVasiDrawable(const SGVec4f& red, const SGVec4f& white) :
  _red(red),
  _white(white)
{
  setUseDisplayList(false);
  setSupportsDisplayList(false);
}

void
SGVasiDrawable::addLight(const SGVec3f& position, const SGVec3f& normal,
                         const SGVec3f& up, float azimutDeg)
{
  SGVec3f horizontal(normalize(cross(up, normal)));
  SGVec3f zeroGlideSlope = normalize(cross(horizontal, up));
  SGQuatf rotation = SGQuatf::fromAngleAxisDeg(azimutDeg, horizontal);
  SGVec3f azimutGlideSlope = rotation.transform(zeroGlideSlope);
  addLight(position, azimutGlideSlope, up);
}

void
SGVasiDrawable::addLight(const SGVec3f& position, const SGVec3f& normal,
                         const SGVec3f& up)
{
  _lights.push_back(LightData(position, normal, up));
}

void
SGVasiDrawable::drawImplementation(osg::RenderInfo& renderInfo) const
{
  // Make sure we have the current state set
//   renderInfo.getState()->apply();

  // Retrieve the eye point in local coords
  osg::Matrix m;
  m.invert(renderInfo.getState()->getModelViewMatrix());
  SGVec3f eyePoint(toSG(m.preMult(osg::Vec3(0, 0, 0))));
  
  // paint the points
  for (unsigned i = 0; i < _lights.size(); ++i)
    draw(eyePoint, _lights[i]);
}

osg::BoundingBox SGVasiDrawable::computeBoundingBox() const
{
  osg::BoundingBox bb;
  for (unsigned i = 0; i < _lights.size(); ++i)
    bb.expandBy(toOsg(_lights[i].position));
  
  // blow up to avoid being victim to small feature culling ...
  bb.expandBy(bb._min - osg::Vec3(1, 1, 1));
  bb.expandBy(bb._max + osg::Vec3(1, 1, 1));
  return bb;
}

SGVec4f
SGVasiDrawable::getColor(float angleDeg) const
{
  float transDeg = 0.05f;
  if (angleDeg < -transDeg) {
    return _red;
  } else if (angleDeg < transDeg) {
    float fac = angleDeg*0.5f/transDeg + 0.5f;
    return _red + fac*(_white - _red);
  } else {
    return _white;
  }
}

void
SGVasiDrawable::draw(const SGVec3f& eyePoint, const LightData& light) const
{
  // vector pointing from the light position to the eye
  SGVec3f lightToEye = eyePoint - light.position;
  
  // dont' draw, we are behind it
  if (dot(lightToEye, light.normal) < SGLimitsf::min())
    return;
  
  // Now project the eye point vector into the plane defined by the
  // glideslope direction and the up direction
  SGVec3f projLightToEye = lightToEye
    - light.horizontal*dot(lightToEye, light.horizontal);
  
  // dont' draw, if we are to near, looks like we are already behind
  float sqrProjLightToEyeLength = dot(projLightToEye, projLightToEye);
  if (sqrProjLightToEyeLength < 1e-3*1e-3)
    return;
  
  // the scalar product of the glide slope up direction with the eye vector
  float dotProd = dot(projLightToEye, light.normalCrossHorizontal);
  float sinAngle = dotProd/sqrt(sqrProjLightToEyeLength);
  if (sinAngle < -1)
    sinAngle = -1;
  if (1 < sinAngle)
    sinAngle = 1;
  
  float angleDeg = SGMiscf::rad2deg(asin(sinAngle));
  light.draw(getColor(angleDeg));
}

