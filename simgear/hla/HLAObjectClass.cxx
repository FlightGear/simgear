// Copyright (C) 2009 - 2012  Mathias Froehlich - Mathias.Froehlich@web.de
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//

#include "HLAObjectClass.hxx"

#include "simgear/debug/logstream.hxx"
#include "RTIFederate.hxx"
#include "RTIObjectClass.hxx"
#include "RTIObjectInstance.hxx"
#include "HLADataType.hxx"
#include "HLAFederate.hxx"
#include "HLAObjectInstance.hxx"

namespace simgear {

HLAObjectClass::InstanceCallback::~InstanceCallback()
{
}

void
HLAObjectClass::InstanceCallback::discoverInstance(const HLAObjectClass&, HLAObjectInstance& objectInstance, const RTIData& tag)
{
}

void
HLAObjectClass::InstanceCallback::removeInstance(const HLAObjectClass&, HLAObjectInstance& objectInstance, const RTIData& tag)
{
}

void
HLAObjectClass::InstanceCallback::registerInstance(const HLAObjectClass&, HLAObjectInstance& objectInstance)
{
}

void
HLAObjectClass::InstanceCallback::deleteInstance(const HLAObjectClass&, HLAObjectInstance& objectInstance)
{
}

HLAObjectClass::RegistrationCallback::~RegistrationCallback()
{
}

HLAObjectClass::HLAObjectClass(const std::string& name, HLAFederate* federate) :
    _federate(federate),
    _name(name)
{
    if (!federate) {
        SG_LOG(SG_NETWORK, SG_ALERT, "HLAObjectClass::HLAObjectClass(): "
               "No parent federate given for object class \"" << getName() << "\"!");
        return;
    }
    federate->_insertObjectClass(this);
}

HLAObjectClass::~HLAObjectClass()
{
    // HLAObjectClass objects only get deleted when the parent federate
    // dies. So we do not need to deregister there.

    _clearRTIObjectClass();
}

const std::string&
HLAObjectClass::getName() const
{
    return _name;
}

unsigned
HLAObjectClass::getNumAttributes() const
{
    return _attributeVector.size();
}

unsigned
HLAObjectClass::addAttribute(const std::string& name)
{
    unsigned index = _attributeVector.size();
    _nameIndexMap[name] = index;
    _attributeVector.push_back(Attribute(name));
    _resolveAttributeIndex(name, index);
    return index;
}

unsigned
HLAObjectClass::getAttributeIndex(const std::string& name) const
{
    NameIndexMap::const_iterator i = _nameIndexMap.find(name);
    if (i == _nameIndexMap.end())
        return ~0u;
    return i->second;
}

std::string
HLAObjectClass::getAttributeName(unsigned index) const
{
    if (_attributeVector.size() <= index)
        return std::string();
    return _attributeVector[index]._name;
}

const HLADataType*
HLAObjectClass::getAttributeDataType(unsigned index) const
{
    if (_attributeVector.size() <= index)
        return 0;
    return _attributeVector[index]._dataType.get();
}

void
HLAObjectClass::setAttributeDataType(unsigned index, const HLADataType* dataType)
{
    if (_attributeVector.size() <= index)
        return;
    _attributeVector[index]._dataType = dataType;
}

HLAUpdateType
HLAObjectClass::getAttributeUpdateType(unsigned index) const
{
    if (_attributeVector.size() <= index)
        return HLAUndefinedUpdate;
    return _attributeVector[index]._updateType;
}

void
HLAObjectClass::setAttributeUpdateType(unsigned index, HLAUpdateType updateType)
{
    if (_attributeVector.size() <= index)
        return;
    _attributeVector[index]._updateType = updateType;
}

HLASubscriptionType
HLAObjectClass::getAttributeSubscriptionType(unsigned index) const
{
    if (_attributeVector.size() <= index)
        return HLAUnsubscribed;
    return _attributeVector[index]._subscriptionType;
}

void
HLAObjectClass::setAttributeSubscriptionType(unsigned index, HLASubscriptionType subscriptionType)
{
    if (_attributeVector.size() <= index)
        return;
    _attributeVector[index]._subscriptionType = subscriptionType;
}

HLAPublicationType
HLAObjectClass::getAttributePublicationType(unsigned index) const
{
    if (_attributeVector.size() <= index)
        return HLAUnpublished;
    return _attributeVector[index]._publicationType;
}

void
HLAObjectClass::setAttributePublicationType(unsigned index, HLAPublicationType publicationType)
{
    if (_attributeVector.size() <= index)
        return;
    _attributeVector[index]._publicationType = publicationType;
}

HLADataElement::IndexPathPair
HLAObjectClass::getIndexPathPair(const HLADataElement::StringPathPair& stringPathPair) const
{
    unsigned index = getAttributeIndex(stringPathPair.first);
    if (getNumAttributes() <= index) {
        SG_LOG(SG_NETWORK, SG_ALERT, "HLAObjectClass::getIndexPathPair(\""
               << HLADataElement::toString(stringPathPair)
               << "\"): Could not resolve attribute \"" << stringPathPair.first
               << "\" for object class \"" << getName() << "\"!");
    }
    return HLADataElement::IndexPathPair(index, stringPathPair.second);
}

HLADataElement::IndexPathPair
HLAObjectClass::getIndexPathPair(const std::string& path) const
{
    return getIndexPathPair(HLADataElement::toStringPathPair(path));
}

bool
HLAObjectClass::subscribe()
{
    if (!_rtiObjectClass.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "HLAObjectClass::subscribe(): "
               "No RTIObject class for object class \"" << getName() << "\"!");
        return false;
    }

    HLAIndexList indexList;
    for (unsigned i = 1; i < getNumAttributes(); ++i) {
        if (_attributeVector[i]._subscriptionType != HLASubscribedActive)
            continue;
        indexList.push_back(i);
    }
    if (!indexList.empty()) {
        if (!_rtiObjectClass->subscribe(indexList, true))
            return false;
    }

    indexList.clear();
    for (unsigned i = 1; i < getNumAttributes(); ++i) {
        if (_attributeVector[i]._subscriptionType != HLASubscribedPassive)
            continue;
        indexList.push_back(i);
    }
    if (!indexList.empty()) {
        if (!_rtiObjectClass->subscribe(indexList, false))
            return false;
    }
    return true;
}

bool
HLAObjectClass::unsubscribe()
{
    if (!_rtiObjectClass.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "HLAObjectClass::unsubscribe(): "
               "No RTIObject class for object class \"" << getName() << "\"!");
        return false;
    }
    return _rtiObjectClass->unsubscribe();
}

bool
HLAObjectClass::publish()
{
    if (!_rtiObjectClass.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "HLAObjectClass::publish(): "
               "No RTIObject class for object class \"" << getName() << "\"!");
        return false;
    }

    HLAIndexList indexList;
    for (unsigned i = 1; i < getNumAttributes(); ++i) {
        if (_attributeVector[i]._publicationType == HLAUnpublished)
            continue;
        indexList.push_back(i);
    }
    if (indexList.empty())
        return true;
    if (!_rtiObjectClass->publish(indexList))
        return false;
    return true;
}

bool
HLAObjectClass::unpublish()
{
    if (!_rtiObjectClass.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "HLAObjectClass::unpublish(): "
               "No RTIObject class for object class \"" << getName() << "\"!");
        return false;
    }
    return _rtiObjectClass->unpublish();
}

void
HLAObjectClass::startRegistration() const
{
}

void
HLAObjectClass::stopRegistration() const
{
}

HLAObjectInstance*
HLAObjectClass::createObjectInstance(const std::string& name)
{
    HLAObjectInstance* objectInstance = createObjectInstance();
    if (objectInstance)
        return objectInstance;
    SGSharedPtr<HLAFederate> federate = _federate.lock();
    if (!federate.valid())
        return 0;
    return federate->createObjectInstance(this, name);
}

HLAObjectInstance*
HLAObjectClass::createObjectInstance()
{
    return 0;
}

void
HLAObjectClass::_setRTIObjectClass(RTIObjectClass* objectClass)
{
    if (_rtiObjectClass) {
        SG_LOG(SG_NETWORK, SG_ALERT, "HLAObjectClass: Setting RTIObjectClass twice for object class \"" << getName() << "\"!");
        return;
    }
    _rtiObjectClass = objectClass;
    if (_rtiObjectClass->_objectClass != this) {
        SG_LOG(SG_NETWORK, SG_ALERT, "HLAObjectClass: backward reference does not match!");
        return;
    }
    for (unsigned i = 0; i < _attributeVector.size(); ++i)
        _resolveAttributeIndex(_attributeVector[i]._name, i);
}

void
HLAObjectClass::_resolveAttributeIndex(const std::string& name, unsigned index)
{
    if (!_rtiObjectClass)
        return;
    if (!_rtiObjectClass->resolveAttributeIndex(name, index))
        SG_LOG(SG_NETWORK, SG_ALERT, "HLAObjectClass: Could not resolve attribute \""
               << name << "\" for object class \"" << getName() << "\"!");
}

void
HLAObjectClass::_clearRTIObjectClass()
{
    if (!_rtiObjectClass.valid())
        return;
    _rtiObjectClass->_objectClass = 0;
    _rtiObjectClass = 0;
}

void
HLAObjectClass::_discoverInstance(RTIObjectInstance* rtiObjectInstance, const RTIData& tag)
{
    SGSharedPtr<HLAFederate> federate = _federate.lock();
    if (!federate.valid()) {
        SG_LOG(SG_NETWORK, SG_ALERT, "RTI: could not find parent federate while discovering object instance");
        return;
    }

    SGSharedPtr<HLAObjectInstance> objectInstance = createObjectInstance(rtiObjectInstance->getName());
    if (!objectInstance.valid()) {
        SG_LOG(SG_NETWORK, SG_INFO, "RTI: could not create new object instance for discovered \""
               << rtiObjectInstance->getName() << "\" object");
        return;
    }
    SG_LOG(SG_NETWORK, SG_INFO, "RTI: create new object instance for discovered \""
           << rtiObjectInstance->getName() << "\" object");
    objectInstance->_setRTIObjectInstance(rtiObjectInstance);
    if (!federate->_insertObjectInstance(objectInstance)) {
        SG_LOG(SG_NETWORK, SG_ALERT, "RTI: could not insert new object instance for discovered \""
               << rtiObjectInstance->getName() << "\" object");
        return;
    }
    if (_instanceCallback.valid())
        _instanceCallback->discoverInstance(*this, *objectInstance, tag);
}

void
HLAObjectClass::_removeInstance(HLAObjectInstance& objectInstance, const RTIData& tag)
{
    SGSharedPtr<HLAFederate> federate = _federate.lock();
    if (!federate.valid()) {
        SG_LOG(SG_NETWORK, SG_ALERT, "RTI: could not find parent federate while removing object instance");
        return;
    }
    SG_LOG(SG_NETWORK, SG_INFO, "RTI: remove object instance \"" << objectInstance.getName() << "\"");
    if (_instanceCallback.valid())
        _instanceCallback->removeInstance(*this, objectInstance, tag);
    federate->_eraseObjectInstance(objectInstance.getName());
}

void
HLAObjectClass::_registerInstance(HLAObjectInstance* objectInstance)
{
    SGSharedPtr<HLAFederate> federate = _federate.lock();
    if (!federate.valid()) {
        SG_LOG(SG_NETWORK, SG_ALERT, "RTI: could not find parent federate while registering object instance");
        return;
    }
    if (!objectInstance)
        return;
    // We can only register object instances with a valid name at the rti.
    // So, we cannot do that at HLAObjectInstance creation time.
    if (!federate->_insertObjectInstance(objectInstance)) {
        SG_LOG(SG_NETWORK, SG_ALERT, "RTI: could not insert new object instance \""
               << objectInstance->getName() << "\" object");
        return;
    }
    if (_instanceCallback.valid())
        _instanceCallback->registerInstance(*this, *objectInstance);
}

void
HLAObjectClass::_deleteInstance(HLAObjectInstance& objectInstance)
{
    SGSharedPtr<HLAFederate> federate = _federate.lock();
    if (!federate.valid()) {
        SG_LOG(SG_NETWORK, SG_ALERT, "RTI: could not find parent federate while deleting object instance");
        return;
    }
    if (_instanceCallback.valid())
        _instanceCallback->deleteInstance(*this, objectInstance);
    federate->_eraseObjectInstance(objectInstance.getName());
}

void
HLAObjectClass::_startRegistration()
{
    if (_registrationCallback.valid())
        _registrationCallback->startRegistration(*this);
    else
        startRegistration();
}

void
HLAObjectClass::_stopRegistration()
{
    if (_registrationCallback.valid())
        _registrationCallback->stopRegistration(*this);
    else
        stopRegistration();
}

} // namespace simgear
