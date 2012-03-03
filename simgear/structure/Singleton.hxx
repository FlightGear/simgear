#ifndef SIMGEAR_SINGLETON_HXX
#define SIMGEAR_SINGLETON_HXX 1

#include "singleton.hpp"

namespace simgear
{
/**
 * Class that supplies the address of a singleton instance. This class
 * can be inherited by its Class argument in order to support the
 * instance() method in that class.
 */
template <typename Class>
class Singleton
{
protected:
    Singleton() {}
public:
    static Class* instance()
    {
        Class& singleton
            = boost::details::pool::singleton_default<Class>::instance();
        return &singleton;
    }
};

}
#endif
