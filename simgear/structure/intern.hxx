#ifndef SIMGEAR_INTERN_HXX
#define SIMGEAR_INTERN_HXX 1

#include <string>

namespace simgear
{
/**
 * Return a pointer to a single string object for a given string.
 */

const std::string* intern(const std::string& str);
}
#endif
