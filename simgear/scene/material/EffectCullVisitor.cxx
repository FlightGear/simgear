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
#include <osg/Texture2D>
#include <osg/LOD>

#include "EffectCullVisitor.hxx"

#include "EffectGeode.hxx"
#include "Effect.hxx"
#include "Technique.hxx"

#include <simgear/scene/util/RenderConstants.hxx>

namespace simgear
{

using osgUtil::CullVisitor;

EffectCullVisitor::EffectCullVisitor() :
    _ignoreLOD(false),
    _collectLights(false)
{
}

EffectCullVisitor::EffectCullVisitor(bool ignoreLOD, bool collectLights) :
    _ignoreLOD(ignoreLOD),
    _collectLights(collectLights)
{
}

EffectCullVisitor::EffectCullVisitor(const EffectCullVisitor& rhs) :
    osg::Referenced(rhs),
    CullVisitor(rhs),
    _ignoreLOD(rhs._ignoreLOD),
    _collectLights(rhs._collectLights)
{
}

CullVisitor* EffectCullVisitor::clone() const
{
    return new EffectCullVisitor(*this);
}

void EffectCullVisitor::apply(osg::LOD& node)
{
    if (_ignoreLOD) {
        if (isCulled(node)) return;

        // push the culling mode.
        pushCurrentMask();

        // push the node's state.
        osg::StateSet* node_state = node.getStateSet();
        if (node_state) pushStateSet(node_state);

        if (_traversalMode==TRAVERSE_PARENTS) node.osg::Group::ascend(*this);
        else if (_traversalMode!=TRAVERSE_NONE) node.osg::Group::traverse(*this);
        // pop the node's state off the render graph stack.
        if (node_state) popStateSet();

        // pop the culling mode.
        popCurrentMask();
    }
    else
        CullVisitor::apply(node);
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
    if (_collectLights && ( eg->getNodeMask() & MODELLIGHT_BIT ) ) {
        _lightList.push_back( eg );
    }
    Effect* effect = eg->getEffect();
    Technique* technique = 0;
    if (!effect) {
        CullVisitor::apply(node);
        return;
    } else if (!(technique = effect->chooseTechnique(&getRenderInfo()))) {
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

void EffectCullVisitor::clearBufferList()
{
    _bufferList.clear();
}

void EffectCullVisitor::addBuffer(int i, osg::Texture2D* tex)
{
    _bufferList.insert(std::make_pair(i,tex));
}

osg::Texture2D* EffectCullVisitor::getBuffer(int i)
{
    return _bufferList[i];
}

}
