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

#include <map>

namespace osg
{
class Geode;
class Texture2D;
}

namespace simgear
{
class EffectGeode;
class EffectCullVisitor : public osgUtil::CullVisitor
{
public:
    EffectCullVisitor(bool collectLights = false);
    EffectCullVisitor(const EffectCullVisitor&);
    virtual osgUtil::CullVisitor* clone() const;
    using osgUtil::CullVisitor::apply;
    virtual void apply(osg::Geode& node);
    virtual void reset();

    void clearBufferList();
    void addBuffer(std::string b, osg::Texture2D* tex);
    osg::Texture2D* getBuffer(std::string b);

private:
    std::map<std::string,osg::ref_ptr<osg::Texture2D> > _bufferList;
    std::vector<osg::ref_ptr<EffectGeode> > _lightList;
    bool _collectLights;
};
}
#endif
