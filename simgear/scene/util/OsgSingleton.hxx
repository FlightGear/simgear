#ifndef SIMGEAR_OSGSINGLETON_HXX
#define SIMGEAR_OSGSINGLETON_HXX 1

#include <simgear/structure/Singleton.hxx>

#include <osg/Referenced>
#include <osg/ref_ptr>

namespace simgear {

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
