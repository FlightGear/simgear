// vectorPropTemplates.hxx -- Templates Requiring Vector Properties
//
// Separate header file for any templates requiring SGVecXX vector types.

#ifndef __VECTORPROPTEMPLATES_HXX
#define __VECTORPROPTEMPLATES_HXX

#include "props.hxx"

// The templates below depend on the full SGMath.hxx include. Forward
// declarations (SGMathFwd.hxx) would be insufficient here (at least with MSVC).
#include <simgear/math/SGMath.hxx>

namespace simgear
{

// Extended properties
template<>
std::istream& readFrom<SGVec3d>(std::istream& stream, SGVec3d& result);
template<>
std::istream& readFrom<SGVec4d>(std::istream& stream, SGVec4d& result);

namespace props
{

template<>
struct PropertyTraits<SGVec3d>
{
    static const Type type_tag = VEC3D;
    enum  { Internal = 0 };
};

template<>
struct PropertyTraits<SGVec4d>
{
    static const Type type_tag = VEC4D;
    enum  { Internal = 0 };
};
}
}

template<>
std::ostream& SGRawBase<SGVec3d>::printOn(std::ostream& stream) const;
template<>
std::ostream& SGRawBase<SGVec4d>::printOn(std::ostream& stream) const;

#endif // __VECTORPROPTEMPLATES_HXX

// end of vectorPropTemplates.hxx
