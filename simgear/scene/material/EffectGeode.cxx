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

#include "EffectGeode.hxx"
#include "Effect.hxx"
#include "Technique.hxx"

#include <osgUtil/CullVisitor>

#include <osgDB/Registry>
#include <osgDB/Input>
#include <osgDB/ParameterOutput>

namespace simgear
{

using namespace osg;
using namespace osgUtil;

EffectGeode::EffectGeode()
{
}

EffectGeode::EffectGeode(const EffectGeode& rhs, const CopyOp& copyop) :
    Geode(rhs, copyop)
{
    _effect = static_cast<Effect*>(rhs._effect->clone(copyop));
}

void EffectGeode::resizeGLObjectBuffers(unsigned int maxSize)
{
    if (_effect.valid())
        _effect->resizeGLObjectBuffers(maxSize);
    Geode::resizeGLObjectBuffers(maxSize);
}

void EffectGeode::releaseGLObjects(osg::State* state) const
{
    if (_effect.valid())
        _effect->releaseGLObjects(state);
    Geode::releaseGLObjects(state);
}

bool EffectGeode_writeLocalData(const Object& obj, osgDB::Output& fw)
{
    const EffectGeode& eg = static_cast<const EffectGeode&>(obj);

    fw.indent() << "effect\n";
    fw.writeObject(*eg.getEffect());
    return true;
}

namespace
{
osgDB::RegisterDotOsgWrapperProxy effectGeodeProxy
(
    new EffectGeode,
    "simgear::EffectGeode",
    "Object Node Geode simgear::EffectGeode",
    0,
    &EffectGeode_writeLocalData
    );
}
}
