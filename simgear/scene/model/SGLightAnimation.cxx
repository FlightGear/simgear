// Copyright (C) 2011 Frederic Bouvier (fredfgfs01@free.fr)
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
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include "animation.hxx"
#include "ConditionNode.hxx"

#include <osg/Geometry>
#include <osg/MatrixTransform>
#include <simgear/scene/material/EffectGeode.hxx>
#include <boost/scoped_array.hpp>

SGLightAnimation::SGLightAnimation(const SGPropertyNode* configNode,
                                   SGPropertyNode* modelRoot) :
    SGAnimation(configNode, modelRoot)
{
}

osg::Group*
SGLightAnimation::createAnimationGroup(osg::Group& parent)
{
    osg::MatrixTransform* grp = new osg::MatrixTransform;
    osg::Vec3 position( getConfig()->getDoubleValue("position/x-m"),
                        getConfig()->getDoubleValue("position/y-m"),
                        getConfig()->getDoubleValue("position/z-m") );
    grp->setMatrix(osg::Matrix::translate(position));
  parent.addChild(grp);
  grp->setNodeMask( simgear::MODELLIGHT_BIT );
  return grp;
}

void
SGLightAnimation::install(osg::Node& node)
{
  SGAnimation::install(node);

  std::string light_type = getConfig()->getStringValue("light-type");
  if (light_type == "spot") {
    osg::Vec3 axis( getConfig()->getDoubleValue("axis/x"),
                    getConfig()->getDoubleValue("axis/y"),
                    getConfig()->getDoubleValue("axis/z") );
    double l = axis.length();
    if (l < 0.001) return;
    axis /= l;

    osg::Vec3 p1(  axis.z(), axis.x(), axis.y() ),
              p2 = axis ^ p1;
    p1 = p2 ^ axis;

    float n = getConfig()->getFloatValue("near-m"),
          f = getConfig()->getFloatValue("far-m"),
          a1 = getConfig()->getFloatValue("angle-inner-deg"),
          a2 = getConfig()->getFloatValue("angle-outer-deg"),
          r1 = n * tan( a2 * SG_DEGREES_TO_RADIANS ),
          r2 = f * tan( a2 * SG_DEGREES_TO_RADIANS );

    osg::Geometry* cone = new osg::Geometry;
cone->setUseDisplayList(false);
    osg::Vec3Array* vertices = new osg::Vec3Array(36);
    (*vertices)[0] = osg::Vec3(0.0,0.0,0.0);
    for (int i=0; i<16; ++i) {
      (*vertices)[16-i]    = axis*f + p1*r2*cos( i * 2 * M_PI / 16 ) + p2*r2*sin( i * 2 * M_PI / 16 );
    }
    (*vertices)[17] = (*vertices)[1];

    // Bottom
    (*vertices)[18] = axis*f;
    for (int i=0; i<16; ++i) {
      (*vertices)[19+i]    = axis*f + p1*r2*cos( i * 2 * M_PI / 16 ) + p2*r2*sin( i * 2 * M_PI / 16 );
    }
    (*vertices)[35] = (*vertices)[19];

    osg::Vec4Array* colours = new osg::Vec4Array(1);
    (*colours)[0].set(  getConfig()->getFloatValue("color/red"),
                        getConfig()->getFloatValue("color/green"),
                        getConfig()->getFloatValue("color/blue"),
                        1.0f);
    cone->setColorArray(colours);
    cone->setColorBinding(osg::Geometry::BIND_OVERALL);

    osg::Vec3Array* normals = new osg::Vec3Array(1);
    (*normals)[0] = axis;
    cone->setNormalArray(normals);
    cone->setNormalBinding(osg::Geometry::BIND_OVERALL);

    cone->setVertexArray(vertices);
    cone->addPrimitiveSet( new osg::DrawArrays( GL_TRIANGLE_FAN, 0, 18 ) );
    cone->addPrimitiveSet( new osg::DrawArrays( GL_TRIANGLE_FAN, 18, 18 ) );

    simgear::EffectGeode* geode = new simgear::EffectGeode;
    geode->addDrawable( cone );

    node.asGroup()->addChild( geode );
    simgear::Effect *effect = simgear::makeEffect("Effects/light-spot", true);
    if (effect) {
      effect->parametersProp->setFloatValue("inner-angle",getConfig()->getFloatValue("angle-inner-deg"));
      effect->parametersProp->setFloatValue("outer-angle",getConfig()->getFloatValue("angle-outer-deg"));
      geode->setEffect(effect);
    }
  }
}
