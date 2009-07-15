#include <simgear/scene/material/GLPredicate.hxx>

#include <algorithm>
#include <boost/bind.hpp>

#include <osg/GLExtensions>
#include <osg/Math>

namespace simgear
{
using namespace std;
using namespace osg;
using namespace boost;

bool GLPredicate::operator ()(unsigned int contextID)
{
    float versionNumber = getGLVersionNumber() * 10.0f;
    float required = (static_cast<float>(majorVersion) * 10.0f
                      + static_cast<float>(minorVersion));
    if (versionNumber < required
        && !osg::equivalent(versionNumber, required))
        return false;
    return (find_if(extensions.begin(), extensions.end(),
                    !bind(isGLExtensionSupported, contextID,
                          bind(&string::c_str, _1)))
            == extensions.end());
}
}
