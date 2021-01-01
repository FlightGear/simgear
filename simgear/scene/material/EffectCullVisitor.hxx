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

#ifndef SIMGEAR_EFFECT_CULL_VISITOR_HXX
#define SIMGEAR_EFFECT_CULL_VISITOR_HXX 1

#include <osgUtil/CullVisitor>

#include <simgear/scene/model/SGLight.hxx>

namespace osg
{
class Geode;
}

namespace simgear
{
class Effect;
class EffectGeode;
class EffectCullVisitor : public osgUtil::CullVisitor
{
public:
    EffectCullVisitor(bool collectLights = false, const std::string &effScheme = "");
    EffectCullVisitor(const EffectCullVisitor&);
    virtual osgUtil::CullVisitor* clone() const;
    using osgUtil::CullVisitor::apply;
    virtual void apply(osg::Node& node);
    virtual void apply(osg::Geode& node);
    virtual void reset();

    SGLightList getLightList() const { return _lightList; }

private:
    SGLightList _lightList;
    bool _collectLights;
    std::string _effScheme;
};
}
#endif
