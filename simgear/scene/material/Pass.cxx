#include "Pass.hxx"

#include <simgear/structure/OSGUtils.hxx>

#include <osg/StateSet>
#include <osgDB/Registry>
#include <osgDB/Input>
#include <osgDB/ParameterOutput>

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

bool Pass_writeLocalData(const osg::Object& obj, osgDB::Output& fw)
{
    const Pass& pass = static_cast<const Pass&>(obj);

    fw.indent() << "stateSet\n";
    fw.writeObject(*pass.getStateSet());
    return true;
}

namespace
{
osgDB::RegisterDotOsgWrapperProxy passProxy
(
    new Pass,
    "simgear::Pass",
    "Object simgear::Pass",
    0,
    &Pass_writeLocalData
    );
}
}
