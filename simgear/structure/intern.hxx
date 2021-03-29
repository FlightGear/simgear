#ifndef SIMGEAR_INTERN_HXX
#define SIMGEAR_INTERN_HXX 1

#include <string>

#include <typeinfo>
#ifndef _MSC_VER
#include <cxxabi.h>
#endif

namespace simgear
{

/* return the template name as a string */
template <typename T>
std::string getTypeName(void)
{
#ifdef _MSC_VER
  const char *type = typeid(T).name();
  const char *tn = strchr(type, ' ');
  std::string name = tn ? tn+1 : type;
#else // __linux__
  int error = 0;
  char *type = abi::__cxa_demangle(typeid(T).name(), 0, 0, &error);
  std::string name = type;
  free(type);
#endif

  return name;
}

/**
 * Return a pointer to a single string object for a given string.
 */
const std::string* intern(const std::string& str);
}
#endif
