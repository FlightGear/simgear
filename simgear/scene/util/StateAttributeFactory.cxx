#include <OpenThreads/ScopedLock>

#include "StateAttributeFactory.hxx"

using namespace osg;

namespace simgear
{
StateAttributeFactory::StateAttributeFactory()
{
    _standardAlphaFunc = new AlphaFunc;
    _standardAlphaFunc->setFunction(osg::AlphaFunc::GREATER);
    _standardAlphaFunc->setReferenceValue(0.01);
    _standardAlphaFunc->setDataVariance(Object::STATIC);
    _smooth = new ShadeModel;
    _smooth->setMode(ShadeModel::SMOOTH);
    _smooth->setDataVariance(Object::STATIC);
    _flat = new ShadeModel(ShadeModel::FLAT);
    _flat->setDataVariance(Object::STATIC);
    _standardBlendFunc = new BlendFunc;
    _standardBlendFunc->setSource(BlendFunc::SRC_ALPHA);
    _standardBlendFunc->setDestination(BlendFunc::ONE_MINUS_SRC_ALPHA);
    _standardBlendFunc->setDataVariance(Object::STATIC);
    _standardTexEnv = new TexEnv;
    _standardTexEnv->setMode(TexEnv::MODULATE);
    _standardTexEnv->setDataVariance(Object::STATIC);
}

osg::ref_ptr<StateAttributeFactory> StateAttributeFactory::_theInstance;
OpenThreads::Mutex StateAttributeFactory::_instanceMutex;

StateAttributeFactory* StateAttributeFactory::instance()
{
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_instanceMutex);
    if (!_theInstance.valid()) {
        _theInstance = new StateAttributeFactory;
    }
    return _theInstance.get();
}
}
