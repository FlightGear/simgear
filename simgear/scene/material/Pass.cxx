#include "Pass.hxx"

#include <osgDB/Registry>

namespace simgear
{

Pass::Pass(const Pass& rhs, const osg::CopyOp& copyop) :
    osg::StateSet(rhs, copyop)
{
}

namespace
{
osgDB::RegisterDotOsgWrapperProxy PassProxy
(
    new Pass,
    "simgear::Pass",
    "Object simgear::Pass StateSet ",
    0,
    0
    );
}
}
