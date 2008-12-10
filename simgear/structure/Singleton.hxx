#ifndef SIMGEAR_SINGLETON_HXX
#define SIMGEAR_SINGLETON_HXX 1

#include <boost/pool/detail/singleton.hpp>

#include <osg/Referenced>
#include <osg/ref_ptr>

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
    osg::ref_ptr<RefClass> ptr;
};

template <typename RefClass>
class ReferencedSingleton : public virtual osg::Referenced
{
public:
    static RefClass* instance()
    {
        return SingletonRefPtr<RefClass>::instance();
    }
};
}
#endif
