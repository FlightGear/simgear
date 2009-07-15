#include "Pass.hxx"

#include <simgear/structure/OSGUtils.hxx>

#include <osg/StateSet>

namespace simgear
{

Pass::Pass(const Pass& rhs, const osg::CopyOp& copyop) :
    _stateSet(clone_ref(rhs._stateSet, copyop))
{
}

void Pass::resizeGLObjectBuffers(unsigned int maxSize)
{
    if (_stateSet.valid())
        _stateSet->resizeGLObjectBuffers(maxSize);
}

void Pass::releaseGLObjects(osg::State* state) const
{
    if (_stateSet.valid())
        _stateSet->releaseGLObjects(state);
}

}
