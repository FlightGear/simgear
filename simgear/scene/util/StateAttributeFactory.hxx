#ifndef SIMGEAR_STATEATTRIBUTEFACTORY_HXX
#define SIMGEAR_STATEATTRIBUTEFACTORY_HXX 1

#include <OpenThreads/Mutex>
#include <osg/ref_ptr>
#include <osg/AlphaFunc>
#include <osg/BlendFunc>
#include <osg/ShadeModel>
#include <osg/TexEnv>

// Return read-only instances of common OSG state attributes.
namespace simgear
{
class StateAttributeFactory : public osg::Referenced {
public:
    // Alpha test > .01
    osg::AlphaFunc* getStandardAlphaFunc() { return _standardAlphaFunc.get(); }
    // alpha source, 1 - alpha destination
    osg::BlendFunc* getStandardBlendFunc() { return _standardBlendFunc.get(); }
    // modulate
    osg::TexEnv* getStandardTexEnv() { return _standardTexEnv.get(); }
    osg::ShadeModel* getSmoothShadeModel() { return _smooth.get(); }
    osg::ShadeModel* getFlatShadeModel() { return _flat.get(); }
    static StateAttributeFactory* instance();
protected:
    StateAttributeFactory();
    osg::ref_ptr<osg::AlphaFunc> _standardAlphaFunc;
    osg::ref_ptr<osg::ShadeModel> _smooth;
    osg::ref_ptr<osg::ShadeModel> _flat;
    osg::ref_ptr<osg::BlendFunc> _standardBlendFunc;
    osg::ref_ptr<osg::TexEnv> _standardTexEnv;
    static osg::ref_ptr<StateAttributeFactory> _theInstance;
    static OpenThreads::Mutex _instanceMutex;
};
}
#endif
