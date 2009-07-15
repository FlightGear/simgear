#include "Pass.hxx"

namespace simgear
{

Pass::Pass(const Pass& rhs, const osg::CopyOp& copyop) :
    osg::StateSet(rhs, copyop)
{
}
}
