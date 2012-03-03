#ifndef SIMGEAR_SINGLETON_HXX
#define SIMGEAR_SINGLETON_HXX 1

#include "singleton.hpp"

#include "SGReferenced.hxx"
#include "SGSharedPtr.hxx"

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

template <typename RefClass>
class SingletonRefPtr
{
public:
    SingletonRefPtr()
    {
        ptr = new RefClass;
    }
    static RefClass* instance()
    {
        SingletonRefPtr& singleton
            = boost::details::pool::singleton_default<SingletonRefPtr>::instance();
        return singleton.ptr.get();
    }
private:
    SGSharedPtr<RefClass> ptr;
};

template <typename RefClass>
class ReferencedSingleton : public virtual SGReferenced
{
public:
    static RefClass* instance()
    {
        return SingletonRefPtr<RefClass>::instance();
    }
};

}
#endif
