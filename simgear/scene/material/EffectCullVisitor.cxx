// Copyright (C) 2008  Timothy Moore timoore@redhat.com
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include <osg/StateSet>

#include "EffectCullVisitor.hxx"

#include "EffectGeode.hxx"
#include "Effect.hxx"
#include "Technique.hxx"

namespace simgear
{

using osgUtil::CullVisitor;

EffectCullVisitor::EffectCullVisitor(bool collectLights, const std::string &effScheme) :
    _collectLights(collectLights),
    _effScheme(effScheme)
{
}

EffectCullVisitor::EffectCullVisitor(const EffectCullVisitor& rhs) :
    osg::Object(), CullVisitor(rhs)
{
}

CullVisitor* EffectCullVisitor::clone() const
{
    return new EffectCullVisitor(*this);
}

void EffectCullVisitor::apply(osg::Node &node)
{
    CullVisitor::apply(node);
    if (_collectLights) {
        // TODO: Properly cull lights outside the viewport
        // (override computeBounds() in SGLight)
        SGLight *light = dynamic_cast<SGLight *>(&node);
        if (light)
            _lightList.push_back(light);
    }
}

void EffectCullVisitor::apply(osg::Geode& node)
{
    if (isCulled(node))
        return;
    EffectGeode *eg = dynamic_cast<EffectGeode*>(&node);
    if (!eg) {
        CullVisitor::apply(node);
        return;
    }
    Effect* effect = eg->getEffect();
    Technique* technique = 0;
    if (!effect) {
        CullVisitor::apply(node);
        return;
    } else if (!(technique = effect->chooseTechnique(&getRenderInfo(), _effScheme))) {
        return;
    }
    // push the node's state.
    osg::StateSet* node_state = node.getStateSet();
    if (node_state)
        pushStateSet(node_state);
    for (EffectGeode::DrawablesIterator beginItr = eg->drawablesBegin(),
             e = eg->drawablesEnd();
         beginItr != e;
         beginItr = technique->processDrawables(beginItr, e, this,
                                                eg->isCullingActive()))
        ;
    // pop the node's state off the geostate stack.
    if (node_state)
        popStateSet();

}

void EffectCullVisitor::reset()
{
    _lightList.clear();

    osgUtil::CullVisitor::reset();
}

}
