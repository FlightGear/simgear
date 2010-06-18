// Copyright (C) 2009 - 2010  Mathias Froehlich - Mathias.Froehlich@web.de
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

HLAObjectClass::HLAObjectClass(const std::string& name, HLAFederate& federate) :
    _name(name)
{
    _rtiObjectClass = federate._rtiFederate->createObjectClass(name, this);
    if (!_rtiObjectClass.valid())
        SG_LOG(SG_NETWORK, SG_WARN, "HLAObjectClass::HLAObjectClass(): No RTIObjectClass found for \"" << name << "\"!");
}

HLAObjectClass::~HLAObjectClass()
{
}

unsigned
HLAObjectClass::getNumAttributes() const
{
    if (!_rtiObjectClass.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "HLAObjectClass::getAttributeIndex(): No RTIObject class for object class \"" << getName() << "\"!");
        return 0;
    }
    return _rtiObjectClass->getNumAttributes();
}

unsigned
HLAObjectClass::getAttributeIndex(const std::string& name) const
{
    if (!_rtiObjectClass.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "HLAObjectClass::getAttributeIndex(): No RTIObject class for object class \"" << getName() << "\"!");
        return ~0u;
    }
    return _rtiObjectClass->getOrCreateAttributeIndex(name);
}

std::string
HLAObjectClass::getAttributeName(unsigned index) const
{
    if (!_rtiObjectClass.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "HLAObjectClass::getAttributeIndex(): No RTIObject class for object class \"" << getName() << "\"!");
        return 0;
    }
    return _rtiObjectClass->getAttributeName(index);
}

const HLADataType*
HLAObjectClass::getAttributeDataType(unsigned index) const
{
    if (!_rtiObjectClass.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "HLAObjectClass::getAttributeDataType(): No RTIObject class for object class \"" << getName() << "\"!");
        return 0;
    }
    return _rtiObjectClass->getAttributeDataType(index);
}

void
HLAObjectClass::setAttributeDataType(unsigned index, const HLADataType* dataType)
{
    if (!_rtiObjectClass.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "HLAObjectClass::setAttributeDataType(): No RTIObject class for object class \"" << getName() << "\"!");
        return;
    }
    _rtiObjectClass->setAttributeDataType(index, dataType);
}

HLAUpdateType
HLAObjectClass::getAttributeUpdateType(unsigned index) const
{
    if (!_rtiObjectClass.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "HLAObjectClass::getAttributeUpdateType(): No RTIObject class for object class \"" << getName() << "\"!");
        return HLAUndefinedUpdate;
    }
    return _rtiObjectClass->getAttributeUpdateType(index);
}

void
HLAObjectClass::setAttributeUpdateType(unsigned index, HLAUpdateType updateType)
{
    if (!_rtiObjectClass.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "HLAObjectClass::setAttributeUpdateType(): "
               "No RTIObject class for object class \"" << getName() << "\"!");
        return;
    }
    _rtiObjectClass->setAttributeUpdateType(index, updateType);
}

HLADataElement::IndexPathPair
HLAObjectClass::getIndexPathPair(const HLADataElement::AttributePathPair& attributePathPair) const
{
    unsigned index = getAttributeIndex(attributePathPair.first);
    if (getNumAttributes() <= index) {
        SG_LOG(SG_NETWORK, SG_ALERT, "HLAObjectClass::getIndexPathPair(\""
               << HLADataElement::toString(attributePathPair)
               << "\"): Could not resolve attribute \"" << attributePathPair.first
               << "\" for object class \"" << getName() << "\"!");
    }
    return HLADataElement::IndexPathPair(index, attributePathPair.second);
}

HLADataElement::IndexPathPair
HLAObjectClass::getIndexPathPair(const std::string& path) const
{
    return getIndexPathPair(HLADataElement::toAttributePathPair(path));
}

bool
HLAObjectClass::subscribe(const std::set<unsigned>& indexSet, bool active)
{
    if (!_rtiObjectClass.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "HLAObjectClass::subscribe(): No RTIObject class for object class \"" << getName() << "\"!");
        return false;
    }
    return _rtiObjectClass->subscribe(indexSet, active);
}

bool
HLAObjectClass::unsubscribe()
{
    if (!_rtiObjectClass.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "HLAObjectClass::unsubscribe(): No RTIObject class for object class \"" << getName() << "\"!");
        return false;
    }
    return _rtiObjectClass->unsubscribe();
}

bool
HLAObjectClass::publish(const std::set<unsigned>& indexSet)
{
    if (!_rtiObjectClass.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "HLAObjectClass::publish(): No RTIObject class for object class \"" << getName() << "\"!");
        return false;
    }
    return _rtiObjectClass->publish(indexSet);
}

bool
HLAObjectClass::unpublish()
{
    if (!_rtiObjectClass.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "HLAObjectClass::unpublish(): No RTIObject class for object class \"" << getName() << "\"!");
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
HLAObjectClass::createObjectInstance(RTIObjectInstance* rtiObjectInstance)
{
    return new HLAObjectInstance(this, rtiObjectInstance);
}

void
HLAObjectClass::discoverInstance(RTIObjectInstance* objectInstance, const RTIData& tag)
{
    SGSharedPtr<HLAObjectInstance> hlaObjectInstance = createObjectInstance(objectInstance);
    if (hlaObjectInstance.valid()) {
        SG_LOG(SG_NETWORK, SG_INFO, "RTI: create new object instance for discovered \""
               << hlaObjectInstance->getName() << "\" object");
        _objectInstanceSet.insert(hlaObjectInstance);
        discoverInstanceCallback(*hlaObjectInstance, tag);
    } else {
        SG_LOG(SG_NETWORK, SG_INFO, "RTI: local delete of \"" << objectInstance->getName() << "\"");
        objectInstance->localDeleteObjectInstance();
    }
}

void
HLAObjectClass::removeInstance(HLAObjectInstance& hlaObjectInstance, const RTIData& tag)
{
    SG_LOG(SG_NETWORK, SG_INFO, "RTI: remove object instance \"" << hlaObjectInstance.getName() << "\"");
    removeInstanceCallback(hlaObjectInstance, tag);
    _objectInstanceSet.erase(&hlaObjectInstance);
}

void
HLAObjectClass::registerInstance(HLAObjectInstance& objectInstance)
{
    _objectInstanceSet.insert(&objectInstance);
    registerInstanceCallback(objectInstance);
}

void
HLAObjectClass::deleteInstance(HLAObjectInstance& objectInstance)
{
    deleteInstanceCallback(objectInstance);
    _objectInstanceSet.erase(&objectInstance);
}

void
HLAObjectClass::discoverInstanceCallback(HLAObjectInstance& objectInstance, const RTIData& tag) const
{
    if (!_instanceCallback.valid())
        return;
    _instanceCallback->discoverInstance(*this, objectInstance, tag);
}

void
HLAObjectClass::removeInstanceCallback(HLAObjectInstance& objectInstance, const RTIData& tag) const
{
    if (!_instanceCallback.valid())
        return;
    _instanceCallback->removeInstance(*this, objectInstance, tag);
}

void
HLAObjectClass::registerInstanceCallback(HLAObjectInstance& objectInstance) const
{
    if (!_instanceCallback.valid())
        return;
    _instanceCallback->registerInstance(*this, objectInstance);
}

void
HLAObjectClass::deleteInstanceCallback(HLAObjectInstance& objectInstance) const
{
    if (!_instanceCallback.valid())
        return;
    _instanceCallback->deleteInstance(*this, objectInstance);
}

void
HLAObjectClass::startRegistrationCallback()
{
    if (_registrationCallback.valid())
        _registrationCallback->startRegistration(*this);
    else
        startRegistration();
}

void
HLAObjectClass::stopRegistrationCallback()
{
    if (_registrationCallback.valid())
        _registrationCallback->stopRegistration(*this);
    else
        stopRegistration();
}

} // namespace simgear
