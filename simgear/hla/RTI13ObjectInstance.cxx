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

#include "RTIObjectInstance.hxx"
#include "RTI13Ambassador.hxx"

namespace simgear {

RTI13ObjectInstance::RTI13ObjectInstance(const RTI::ObjectHandle& handle, HLAObjectInstance* hlaObjectInstance,
                                         const RTI13ObjectClass* objectClass, RTI13Ambassador* ambassador, bool owned) :
    RTIObjectInstance(hlaObjectInstance),
    _handle(handle),
    _objectClass(objectClass),
    _ambassador(ambassador),
    _attributeValuePairSet(RTI::AttributeSetFactory::create(objectClass->getNumAttributes()))
{
    updateAttributesFromClass(owned);
}

RTI13ObjectInstance::~RTI13ObjectInstance()
{
}

const RTIObjectClass*
RTI13ObjectInstance::getObjectClass() const
{
    return _objectClass.get();
}

const RTI13ObjectClass*
RTI13ObjectInstance::get13ObjectClass() const
{
    return _objectClass.get();
}

std::string
RTI13ObjectInstance::getName() const
{
    SGSharedPtr<RTI13Ambassador> ambassador = _ambassador.lock();
    if (!ambassador.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "Error: Ambassador is zero.");
        return std::string();
    }

    try {
        return ambassador->getObjectInstanceName(_handle);
    } catch (RTI::ObjectNotKnown& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not get object name: " << e._name << " " << e._reason);
        return std::string();
    } catch (RTI::FederateNotExecutionMember& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not get object name: " << e._name << " " << e._reason);
        return std::string();
    } catch (RTI::ConcurrentAccessAttempted& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not get object name: " << e._name << " " << e._reason);
        return std::string();
    } catch (RTI::RTIinternalError& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not get object name: " << e._name << " " << e._reason);
        return std::string();
    } catch (...) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not get object name.");
        return std::string();
    }
}

void
RTI13ObjectInstance::addToRequestQueue()
{
    SGSharedPtr<RTI13Ambassador> ambassador = _ambassador.lock();
    if (!ambassador.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "Error: Ambassador is zero.");
        return;
    }

    ambassador->addObjectInstanceForCallback(this);
}

void
RTI13ObjectInstance::deleteObjectInstance(const RTIData& tag)
{
    SGSharedPtr<RTI13Ambassador> ambassador = _ambassador.lock();
    if (!ambassador.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "Error: Ambassador is zero.");
        return;
    }

    try {
        ambassador->deleteObjectInstance(_handle, tag);
    } catch (RTI::ObjectNotKnown& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not delete object instance: " << e._name << " " << e._reason);
    } catch (RTI::DeletePrivilegeNotHeld& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not delete object instance: " << e._name << " " << e._reason);
    } catch (RTI::FederateNotExecutionMember& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not delete object instance: " << e._name << " " << e._reason);
    } catch (RTI::ConcurrentAccessAttempted& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not delete object instance: " << e._name << " " << e._reason);
    } catch (RTI::SaveInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not delete object instance: " << e._name << " " << e._reason);
    } catch (RTI::RestoreInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not delete object instance: " << e._name << " " << e._reason);
    } catch (RTI::RTIinternalError& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not delete object instance: " << e._name << " " << e._reason);
    }
}

void
RTI13ObjectInstance::deleteObjectInstance(const SGTimeStamp& timeStamp, const RTIData& tag)
{
    SGSharedPtr<RTI13Ambassador> ambassador = _ambassador.lock();
    if (!ambassador.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "Error: Ambassador is zero.");
        return;
    }

    try {
        ambassador->deleteObjectInstance(_handle, timeStamp, tag);
    } catch (RTI::ObjectNotKnown& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not delete object instance: " << e._name << " " << e._reason);
    } catch (RTI::DeletePrivilegeNotHeld& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not delete object instance: " << e._name << " " << e._reason);
    } catch (RTI::InvalidFederationTime& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not delete object instance: " << e._name << " " << e._reason);
    } catch (RTI::FederateNotExecutionMember& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not delete object instance: " << e._name << " " << e._reason);
    } catch (RTI::ConcurrentAccessAttempted& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not delete object instance: " << e._name << " " << e._reason);
    } catch (RTI::SaveInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not delete object instance: " << e._name << " " << e._reason);
    } catch (RTI::RestoreInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not delete object instance: " << e._name << " " << e._reason);
    } catch (RTI::RTIinternalError& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not delete object instance: " << e._name << " " << e._reason);
    }
}

void
RTI13ObjectInstance::localDeleteObjectInstance()
{
    SGSharedPtr<RTI13Ambassador> ambassador = _ambassador.lock();
    if (!ambassador.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "Error: Ambassador is zero.");
        return;
    }

    try {
        ambassador->localDeleteObjectInstance(_handle);
    } catch (RTI::ObjectNotKnown& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not delete object instance: " << e._name << " " << e._reason);
    } catch (RTI::FederateOwnsAttributes& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not delete object instance: " << e._name << " " << e._reason);
    } catch (RTI::FederateNotExecutionMember& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not delete object instance: " << e._name << " " << e._reason);
    } catch (RTI::ConcurrentAccessAttempted& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not delete object instance: " << e._name << " " << e._reason);
    } catch (RTI::SaveInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not delete object instance: " << e._name << " " << e._reason);
    } catch (RTI::RestoreInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not delete object instance: " << e._name << " " << e._reason);
    } catch (RTI::RTIinternalError& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not delete object instance: " << e._name << " " << e._reason);
    }
}

void
RTI13ObjectInstance::reflectAttributeValues(const RTI::AttributeHandleValuePairSet& attributeValuePairSet, const RTIData& tag)
{
    // Retrieve an empty update struct from the memory pool
    UpdateList updateList;
    getUpdateFromPool(updateList);

    RTI::ULong numAttribs = attributeValuePairSet.size();
    for (RTI::ULong i = 0; i < numAttribs; ++i) {
        unsigned index = getAttributeIndex(attributeValuePairSet.getHandle(i));
        // Get a RTIData from the data pool
        getDataFromPool(index, updateList.back()._indexDataPairList);
        RTI::ULong length = attributeValuePairSet.getValueLength(i);
        updateList.back()._indexDataPairList.back().second.resize(length);
        attributeValuePairSet.getValue(i, updateList.back()._indexDataPairList.back().second.data(), length);
        updateList.back()._tag = tag;
    }

    RTIObjectInstance::reflectAttributeValues(updateList.front()._indexDataPairList, tag);
    // Return the update data back to the pool
    putUpdateToPool(updateList);
}

void
RTI13ObjectInstance::reflectAttributeValues(const RTI::AttributeHandleValuePairSet& attributeValuePairSet, const SGTimeStamp& timeStamp, const RTIData& tag)
{
    // Retrieve an empty update struct from the memory pool
    UpdateList updateList;
    getUpdateFromPool(updateList);

    RTI::ULong numAttribs = attributeValuePairSet.size();
    for (RTI::ULong i = 0; i < numAttribs; ++i) {
        unsigned index = getAttributeIndex(attributeValuePairSet.getHandle(i));
        // Get a RTIData from the data pool
        getDataFromPool(index, updateList.back()._indexDataPairList);
        RTI::ULong length = attributeValuePairSet.getValueLength(i);
        updateList.back()._indexDataPairList.back().second.resize(length);
        attributeValuePairSet.getValue(i, updateList.back()._indexDataPairList.back().second.data(), length);
        updateList.back()._tag = tag;
    }

    scheduleUpdates(timeStamp, updateList);
}

void
RTI13ObjectInstance::requestObjectAttributeValueUpdate()
{
    SGSharedPtr<RTI13Ambassador> ambassador = _ambassador.lock();
    if (!ambassador.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "Error: Ambassador is zero.");
        return;
    }

    try {
        unsigned numAttributes = getNumAttributes();
        std::auto_ptr<RTI::AttributeHandleSet> attributeHandleSet(RTI::AttributeHandleSetFactory::create(numAttributes));
        for (unsigned i = 0; i < numAttributes; ++i) {
            if (!getRequestAttributeUpdate(i))
                continue;
            attributeHandleSet->add(getAttributeHandle(i));
        }
        if (!attributeHandleSet->size())
            return;

        ambassador->requestObjectAttributeValueUpdate(_handle, *attributeHandleSet);

        for (unsigned i = 0; i < numAttributes; ++i)
            setRequestAttributeUpdate(i, false);

        return;
    } catch (RTI::ObjectNotKnown& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not request attribute update for object instance: " << e._name << " " << e._reason);
        return;
    } catch (RTI::AttributeNotDefined& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not request attribute update for object instance: " << e._name << " " << e._reason);
        return;
    } catch (RTI::FederateNotExecutionMember& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not request attribute update for object instance: " << e._name << " " << e._reason);
        return;
    } catch (RTI::ConcurrentAccessAttempted& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not request attribute update for object instance: " << e._name << " " << e._reason);
        return;
    } catch (RTI::SaveInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not request attribute update for object instance: " << e._name << " " << e._reason);
        return;
    } catch (RTI::RestoreInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not request attribute update for object instance: " << e._name << " " << e._reason);
        return;
    } catch (RTI::RTIinternalError& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not request attribute update for object instance: " << e._name << " " << e._reason);
        return;
    } catch (...) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not request attribute update for object instance.");
        return;
    }
}

void
RTI13ObjectInstance::provideAttributeValueUpdate(const RTI::AttributeHandleSet& attributes)
{
    // Called from the ambassador. Just marks some instance attributes dirty so that they are sent with the next update
    RTI::ULong numAttribs = attributes.size();
    for (RTI::ULong i = 0; i < numAttribs; ++i) {
        unsigned index = getAttributeIndex(attributes.getHandle(i));
        setAttributeForceUpdate(index);
    }
}

void
RTI13ObjectInstance::updateAttributeValues(const RTIData& tag)
{
    SGSharedPtr<RTI13Ambassador> ambassador = _ambassador.lock();
    if (!ambassador.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "Error: Ambassador is zero.");
        return;
    }

    try {
        // That means clear()
        _attributeValuePairSet->empty();

        unsigned numAttributes = getNumAttributes();
        for (unsigned i = 0; i < numAttributes; ++i) {
            if (!getAttributeEffectiveUpdateEnabled(i))
                continue;
            const HLADataElement* dataElement = getDataElement(i);
            if (!dataElement)
                continue;
            // FIXME cache somewhere
            RTIData data;
            HLAEncodeStream stream(data);
            dataElement->encode(stream);
            _attributeValuePairSet->add(getAttributeHandle(i), data.data(), data.size());
        }

        if (!_attributeValuePairSet->size())
            return;

        ambassador->updateAttributeValues(_handle, *_attributeValuePairSet, tag);

        for (unsigned i = 0; i < numAttributes; ++i) {
            setAttributeUpdated(i);
        }

    } catch (RTI::ObjectNotKnown& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not delete object instance: " << e._name << " " << e._reason);
    } catch (RTI::AttributeNotDefined& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not delete object instance: " << e._name << " " << e._reason);
    } catch (RTI::AttributeNotOwned& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not delete object instance: " << e._name << " " << e._reason);
    } catch (RTI::FederateNotExecutionMember& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not delete object instance: " << e._name << " " << e._reason);
    } catch (RTI::ConcurrentAccessAttempted& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not delete object instance: " << e._name << " " << e._reason);
    } catch (RTI::SaveInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not delete object instance: " << e._name << " " << e._reason);
    } catch (RTI::RestoreInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not delete object instance: " << e._name << " " << e._reason);
    } catch (RTI::RTIinternalError& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not delete object instance: " << e._name << " " << e._reason);
    }

    // That means clear()
    _attributeValuePairSet->empty();
}

void
RTI13ObjectInstance::updateAttributeValues(const SGTimeStamp& timeStamp, const RTIData& tag)
{
    SGSharedPtr<RTI13Ambassador> ambassador = _ambassador.lock();
    if (!ambassador.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "Error: Ambassador is zero.");
        return;
    }

    try {
        // That means clear()
        _attributeValuePairSet->empty();

        unsigned numAttributes = getNumAttributes();
        for (unsigned i = 0; i < numAttributes; ++i) {
            if (!getAttributeEffectiveUpdateEnabled(i))
                continue;
            const HLADataElement* dataElement = getDataElement(i);
            if (!dataElement)
                continue;
            // FIXME cache somewhere
            RTIData data;
            HLAEncodeStream stream(data);
            dataElement->encode(stream);
            _attributeValuePairSet->add(getAttributeHandle(i), data.data(), data.size());
        }

        if (!_attributeValuePairSet->size())
            return;

        ambassador->updateAttributeValues(_handle, *_attributeValuePairSet, timeStamp, tag);

        for (unsigned i = 0; i < numAttributes; ++i) {
            setAttributeUpdated(i);
        }

    } catch (RTI::ObjectNotKnown& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not delete object instance: " << e._name << " " << e._reason);
    } catch (RTI::AttributeNotDefined& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not delete object instance: " << e._name << " " << e._reason);
    } catch (RTI::AttributeNotOwned& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not delete object instance: " << e._name << " " << e._reason);
    } catch (RTI::InvalidFederationTime& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not delete object instance: " << e._name << " " << e._reason);
    } catch (RTI::FederateNotExecutionMember& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not delete object instance: " << e._name << " " << e._reason);
    } catch (RTI::ConcurrentAccessAttempted& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not delete object instance: " << e._name << " " << e._reason);
    } catch (RTI::SaveInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not delete object instance: " << e._name << " " << e._reason);
    } catch (RTI::RestoreInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not delete object instance: " << e._name << " " << e._reason);
    } catch (RTI::RTIinternalError& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not delete object instance: " << e._name << " " << e._reason);
    }

    // That means clear()
    _attributeValuePairSet->empty();
}

void
RTI13ObjectInstance::attributesInScope(const RTI::AttributeHandleSet& attributeHandleSet)
{
    RTI::ULong numAttribs = attributeHandleSet.size();
    for (RTI::ULong i = 0; i < numAttribs; ++i) {
        RTI::AttributeHandle attributeHandle = attributeHandleSet.getHandle(i);
        setAttributeInScope(getAttributeIndex(attributeHandle), true);
    }
}

void
RTI13ObjectInstance::attributesOutOfScope(const RTI::AttributeHandleSet& attributeHandleSet)
{
    RTI::ULong numAttribs = attributeHandleSet.size();
    for (RTI::ULong i = 0; i < numAttribs; ++i) {
        RTI::AttributeHandle attributeHandle = attributeHandleSet.getHandle(i);
        setAttributeInScope(getAttributeIndex(attributeHandle), false);
    }
}

void
RTI13ObjectInstance::turnUpdatesOnForObjectInstance(const RTI::AttributeHandleSet& attributeHandleSet)
{
    RTI::ULong numAttribs = attributeHandleSet.size();
    for (RTI::ULong i = 0; i < numAttribs; ++i) {
        RTI::AttributeHandle attributeHandle = attributeHandleSet.getHandle(i);
        setAttributeUpdateEnabled(getAttributeIndex(attributeHandle), true);
    }
}

void
RTI13ObjectInstance::turnUpdatesOffForObjectInstance(const RTI::AttributeHandleSet& attributeHandleSet)
{
    RTI::ULong numAttribs = attributeHandleSet.size();
    for (RTI::ULong i = 0; i < numAttribs; ++i) {
        RTI::AttributeHandle attributeHandle = attributeHandleSet.getHandle(i);
        setAttributeUpdateEnabled(getAttributeIndex(attributeHandle), false);
    }
}

void
RTI13ObjectInstance::requestAttributeOwnershipAssumption(const RTI::AttributeHandleSet& attributes, const RTIData& tag)
{
}

void
RTI13ObjectInstance::attributeOwnershipDivestitureNotification(const RTI::AttributeHandleSet& attributes)
{
}

void
RTI13ObjectInstance::attributeOwnershipAcquisitionNotification(const RTI::AttributeHandleSet& attributes)
{
}

void
RTI13ObjectInstance::attributeOwnershipUnavailable(const RTI::AttributeHandleSet& attributes)
{
}

void
RTI13ObjectInstance::requestAttributeOwnershipRelease(const RTI::AttributeHandleSet& attributes, const RTIData& tag)
{
}

void
RTI13ObjectInstance::confirmAttributeOwnershipAcquisitionCancellation(const RTI::AttributeHandleSet& attributes)
{
}

void
RTI13ObjectInstance::informAttributeOwnership(RTI::AttributeHandle attributeHandle, RTI::FederateHandle federateHandle)
{
}

void
RTI13ObjectInstance::attributeIsNotOwned(RTI::AttributeHandle attributeHandle)
{
}

void
RTI13ObjectInstance::attributeOwnedByRTI(RTI::AttributeHandle attributeHandle)
{
}

}
