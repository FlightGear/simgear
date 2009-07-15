#ifndef SIMGEAR_GLPREDICATE_HXX
#define SIMGEAR_GLPREDICATE_HXX 1

#include <vector>
#include <string>

namespace simgear
{

struct GLPredicate
{
    GLPredicate() : majorVersion(0),minorVersion(0) {}
    GLPredicate(int majorVersion_, int minorVersion_) :
        majorVersion(majorVersion_), minorVersion(minorVersion_)
    {
    }
    /** Does OpenGL support the required version and extensions?
     */
    bool operator ()(unsigned int contextID);
    int majorVersion;
    int minorVersion;
    std::vector<const std::string *> extensions;
};
}
#endif
