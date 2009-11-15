// Copyright (C) 2008 - 2009  Tim Moore timoore@redhat.com
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

#ifndef SIMGEAR_EFFECT_HXX
#define SIMGEAR_EFFECT_HXX 1

#include <vector>
#include <string>

#include <osg/Object>
#include <osgDB/ReaderWriter>

#include <simgear/props/props.hxx>

namespace osg
{
class Drawable;
class StateSet;
class RenderInfo;
}

namespace osgUtil
{
class CullVisitor;
}

namespace simgear
{
class Technique;

class Effect : public osg::Object
{
public:
    META_Object(simgear,Effect)
    Effect();
    Effect(const Effect& rhs,
           const osg::CopyOp& copyop = osg::CopyOp::SHALLOW_COPY);
    osg::StateSet* getDefaultStateSet();
    std::vector<osg::ref_ptr<Technique> > techniques;
    SGPropertyNode_ptr root;
    // Pointer to the parameters node, if it exists
    SGPropertyNode_ptr parametersProp;
    Technique* chooseTechnique(osg::RenderInfo* renderInfo);
    virtual void resizeGLObjectBuffers(unsigned int maxSize);
    virtual void releaseGLObjects(osg::State* state = 0) const;
    /*
     * Build the techniques from the effect properties.
     */
    bool realizeTechniques(const osgDB::ReaderWriter::Options* options = 0);
    
protected:
    ~Effect();
};
Effect* makeEffect(const std::string& name,
                   bool realizeTechniques,
                   const osgDB::ReaderWriter::Options* options = 0);

Effect* makeEffect(SGPropertyNode* prop,
                   bool realizeTechniques,
                   const osgDB::ReaderWriter::Options* options = 0);

bool makeParametersFromStateSet(SGPropertyNode* paramRoot,
                                const osg::StateSet* ss);
}
#endif
