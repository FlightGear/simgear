// Copyright (C) 2004-2007  Vivian Meazza
// Copyright (C) 2004-2009  Mathias Froehlich - Mathias.Froehlich@web.de
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

#include "SGInteractionAnimation.hxx"

#include <osg/Geode>
#include <osg/NodeVisitor>
#include <osg/TemplatePrimitiveFunctor>

#include <simgear/bvh/BVHGroup.hxx>
#include <simgear/bvh/BVHLineGeometry.hxx>
#include <simgear/scene/util/OsgMath.hxx>
#include <simgear/scene/util/SGSceneUserData.hxx>

class SGInteractionAnimation::LineCollector : public osg::NodeVisitor {
    struct LinePrimitiveFunctor {
        LinePrimitiveFunctor() : _lineCollector(0)
        { }
        void operator() (const osg::Vec3&, bool)
        { }
        void operator() (const osg::Vec3& v1, const osg::Vec3& v2, bool)
        { if (_lineCollector) _lineCollector->addLine(v1, v2); }
        void operator() (const osg::Vec3&, const osg::Vec3&, const osg::Vec3&,
                         bool)
        { }
        void operator() (const osg::Vec3&, const osg::Vec3&, const osg::Vec3&,
                         const osg::Vec3&, bool)
        { }
        LineCollector* _lineCollector;
    };
    
public:
    LineCollector() :
        osg::NodeVisitor(osg::NodeVisitor::NODE_VISITOR,
                         osg::NodeVisitor::TRAVERSE_ALL_CHILDREN)
    { }
    virtual void apply(osg::Geode& geode)
    {
        osg::TemplatePrimitiveFunctor<LinePrimitiveFunctor> pf;
        pf._lineCollector = this;
        for (unsigned i = 0; i < geode.getNumDrawables(); ++i) {
            geode.getDrawable(i)->accept(pf);
        }
    }
    virtual void apply(osg::Node& node)
    {
        traverse(node);
    }
    virtual void apply(osg::Transform& transform)
    {
        osg::Matrix matrix = _matrix;
        if (transform.computeLocalToWorldMatrix(_matrix, this))
            traverse(transform);
        _matrix = matrix;
    }
    
    const std::vector<SGLineSegmentf>& getLineSegments() const
    { return _lineSegments; }
    
    void addLine(const osg::Vec3& v1, const osg::Vec3& v2)
    {
        // Trick to get the ends in the right order.
        // Use the x axis in the original coordinate system. Choose the
        // most negative x-axis as the one pointing forward
        SGVec3f tv1(toSG(_matrix.preMult(v1)));
        SGVec3f tv2(toSG(_matrix.preMult(v2)));
        if (tv1[0] > tv2[0])
            _lineSegments.push_back(SGLineSegmentf(tv1, tv2));
        else
            _lineSegments.push_back(SGLineSegmentf(tv2, tv1));
    }

    void addBVHElements(osg::Node& node, simgear::BVHLineGeometry::Type type)
    {
        if (_lineSegments.empty())
            return;

        SGSceneUserData* userData;
        userData = SGSceneUserData::getOrCreateSceneUserData(&node);

        simgear::BVHNode* bvNode = userData->getBVHNode();
        if (!bvNode && _lineSegments.size() == 1) {
            simgear::BVHLineGeometry* bvLine;
            bvLine = new simgear::BVHLineGeometry(_lineSegments.front(), type);
            userData->setBVHNode(bvLine);
            return;
        }

        simgear::BVHGroup* group = new simgear::BVHGroup;
        if (bvNode)
            group->addChild(bvNode);

        for (unsigned i = 0; i < _lineSegments.size(); ++i) {
            simgear::BVHLineGeometry* bvLine;
            bvLine = new simgear::BVHLineGeometry(_lineSegments[i], type);
            group->addChild(bvLine);
        }
        userData->setBVHNode(group);
    }
    
private:
    osg::Matrix _matrix;
    std::vector<SGLineSegmentf> _lineSegments;
};

SGInteractionAnimation::SGInteractionAnimation(simgear::SGTransientModelData &modelData) :
  SGAnimation(modelData)
{
}

void
SGInteractionAnimation::install(osg::Node& node)
{
  SGAnimation::install(node);

  if (!getConfig()->hasChild("type"))
      return;
  std::string interactionType;
  interactionType = getConfig()->getStringValue("interaction-type", "");

  LineCollector lineCollector;
  node.accept(lineCollector);

  if (interactionType == "carrier-catapult") {
      lineCollector.addBVHElements(node,
                                   simgear::BVHLineGeometry::CarrierCatapult);
  } else if (interactionType == "carrier-wire") {
      lineCollector.addBVHElements(node,
                                   simgear::BVHLineGeometry::CarrierWire);
  } else {
      SG_LOG(SG_IO, SG_ALERT, "Unknown interaction animation "
             "interaction-type \"" << interactionType << "\". Ignoring!");
  }
}


