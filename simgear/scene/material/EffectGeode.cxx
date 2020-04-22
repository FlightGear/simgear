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

#include "EffectGeode.hxx"
#include "Effect.hxx"
#include "Technique.hxx"

#include <osg/Version>

#include <osgUtil/CullVisitor>
#include <osgUtil/TangentSpaceGenerator>

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

EffectGeode::EffectGeode(const EffectGeode& rhs, const osg::CopyOp& copyop) :
    Geode(rhs, copyop),
    _effect(static_cast<Effect*>(copyop(rhs._effect.get())))
{
}

void EffectGeode::setEffect(Effect* effect)
{
    _effect = effect;
    if (!_effect)
        return;
    addUpdateCallback(new Effect::InitializeCallback);
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

// Generates tangent space vectors or other data from geom, as defined by effect
void EffectGeode::runGenerators(osg::Geometry *geometry)
{
    if(geometry && _effect.valid()) {
        // Generate tangent vectors for the geometry
        osg::ref_ptr<osgUtil::TangentSpaceGenerator> tsg = new osgUtil::TangentSpaceGenerator;
        tsg->generate(geometry,0);  
        // Generating only tangent vector should be enough
        // since the binormal is a cross product of normal and tangent
        // This saves a bit of memory & memory bandwidth!
        int n = _effect->getGenerator(Effect::TANGENT);
        if (n != -1 && !geometry->getVertexAttribArray(n)) {
#if OSG_MIN_VERSION_REQUIRED(3,1,8)
            geometry->setVertexAttribArray(n, tsg->getTangentArray(), osg::Array::BIND_PER_VERTEX);
#else
            geometry->setVertexAttribData(n, osg::Geometry::ArrayData(tsg->getTangentArray(), osg::Geometry::BIND_PER_VERTEX,GL_FALSE));
#endif
        }
        n = _effect->getGenerator(Effect::BINORMAL);
        if (n != -1 && !geometry->getVertexAttribArray(n))
#if OSG_MIN_VERSION_REQUIRED(3,1,8)
            geometry->setVertexAttribArray(n, tsg->getBinormalArray(), osg::Array::BIND_PER_VERTEX);
#else
            geometry->setVertexAttribData(n, osg::Geometry::ArrayData(tsg->getBinormalArray(), osg::Geometry::BIND_PER_VERTEX,GL_FALSE));
#endif

        n = _effect->getGenerator(Effect::NORMAL);
        if (n != -1 && !geometry->getVertexAttribArray(n)) {
            // Provide the tsg-generated normals to the geometry
            Vec4Array * new_normals = tsg->getNormalArray();
            geometry->setNormalArray(new_normals,osg::Array::BIND_PER_VERTEX);
#if OSG_MIN_VERSION_REQUIRED(3,1,8)
            geometry->setVertexAttribArray(n, new_normals, osg::Array::BIND_PER_VERTEX);
#else
            geometry->setVertexAttribData(n, osg::Geometry::ArrayData(new_normals, osg::Geometry::BIND_PER_VERTEX,GL_FALSE));
#endif
        }
    }
}

bool EffectGeode_writeLocalData(const Object& obj, osgDB::Output& fw)
{
    const EffectGeode& eg = static_cast<const EffectGeode&>(obj);

    if (eg.getEffect()) {
        fw.indent() << "effect\n";
        fw.writeObject(*eg.getEffect());
    }

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
