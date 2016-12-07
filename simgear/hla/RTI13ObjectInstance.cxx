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

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include <simgear/compiler.h>

#include "RTIObjectInstance.hxx"
#include "RTI13Ambassador.hxx"

namespace simgear {

RTI13ObjectInstance::RTI13ObjectInstance(const RTI::ObjectHandle& handle, HLAObjectInstance* objectInstance,
                                         const RTI13ObjectClass* objectClass, RTI13Ambassador* ambassador) :
    RTIObjectInstance(objectInstance),
    _handle(handle),
    _objectClass(objectClass),
    _ambassador(ambassador),
    _attributeValuePairSet(RTI::AttributeSetFactory::create(objectClass->getNumAttributes()))
{
    _setNumAttributes(getNumAttributes());
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
    if (!_ambassador.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "Error: Ambassador is zero.");
        return std::string();
    }

    try {
        return _ambassador->getObjectInstanceName(_handle);
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
RTI13ObjectInstance::deleteObjectInstance(const RTIData& tag)
{
    if (!_ambassador.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "Error: Ambassador is zero.");
        return;
    }

    SGSharedPtr<RTI13Federate> federate = _ambassador->_federate.lock();
    if (!federate.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "Error: Federate is zero.");
        return;
    }

    try {
        _ambassador->deleteObjectInstance(_handle, tag);
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
    if (!_ambassador.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "Error: Ambassador is zero.");
        return;
    }

    SGSharedPtr<RTI13Federate> federate = _ambassador->_federate.lock();
    if (!federate.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "Error: Federate is zero.");
        return;
    }

    try {
        _ambassador->deleteObjectInstance(_handle, timeStamp, tag);
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
    if (!_ambassador.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "Error: Ambassador is zero.");
        return;
    }

    SGSharedPtr<RTI13Federate> federate = _ambassador->_federate.lock();
    if (!federate.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "Error: Federate is zero.");
        return;
    }

    try {
        _ambassador->localDeleteObjectInstance(_handle);
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
RTI13ObjectInstance::reflectAttributeValues(RTI13AttributeHandleDataPairList& attributeHandleDataPairList,
                                            const RTIData& tag, HLAIndexList& indexPool)
{
    HLAIndexList reflectedIndices;
    for (RTI13AttributeHandleDataPairList::iterator i = attributeHandleDataPairList.begin();
         i != attributeHandleDataPairList.end(); ++i) {
        unsigned index = getAttributeIndex(i->first);
        _attributeData[index]._data.swap(i->second);

        if (indexPool.empty())
            reflectedIndices.push_back(index);
        else {
            reflectedIndices.splice(reflectedIndices.end(), indexPool, indexPool.begin());
            reflectedIndices.back() = index;
        }
    }

    RTIObjectInstance::reflectAttributeValues(reflectedIndices, tag);

    // Return the index list to the pool
    indexPool.splice(indexPool.end(), reflectedIndices);
}

void
RTI13ObjectInstance::reflectAttributeValues(RTI13AttributeHandleDataPairList& attributeHandleDataPairList,
                                            const SGTimeStamp& timeStamp, const RTIData& tag, HLAIndexList& indexPool)
{
    HLAIndexList reflectedIndices;
    for (RTI13AttributeHandleDataPairList::iterator i = attributeHandleDataPairList.begin();
         i != attributeHandleDataPairList.end(); ++i) {
        unsigned index = getAttributeIndex(i->first);
        _attributeData[index]._data.swap(i->second);

        if (indexPool.empty())
            reflectedIndices.push_back(index);
        else {
            reflectedIndices.splice(reflectedIndices.end(), indexPool, indexPool.begin());
            reflectedIndices.back() = index;
        }
    }

    RTIObjectInstance::reflectAttributeValues(reflectedIndices, timeStamp, tag);

    // Return the index list to the pool
    indexPool.splice(indexPool.end(), reflectedIndices);
}

void
RTI13ObjectInstance::requestObjectAttributeValueUpdate(const HLAIndexList& indexList)
{
    if (!_ambassador.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "Error: Ambassador is zero.");
        return;
    }

    if (indexList.empty())
        return;

    try {
        unsigned numAttributes = getNumAttributes();
        std::unique_ptr<RTI::AttributeHandleSet> attributeHandleSet(RTI::AttributeHandleSetFactory::create(numAttributes));
        for (HLAIndexList::const_iterator i = indexList.begin(); i != indexList.end(); ++i) {
            if (getAttributeOwned(*i)) {
                SG_LOG(SG_NETWORK, SG_WARN, "RTI13ObjectInstance::requestObjectAttributeValueUpdate(): "
                       "Invalid attribute index!");
                continue;
            }
            attributeHandleSet->add(getAttributeHandle(*i));
        }
        if (!attributeHandleSet->size())
            return;

        _ambassador->requestObjectAttributeValueUpdate(_handle, *attributeHandleSet);

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
RTI13ObjectInstance::provideAttributeValueUpdate(const std::vector<RTI::AttributeHandle>& attributeHandleSet)
{
    // Called from the ambassador. Just marks some instance attributes dirty so that they are sent with the next update
    size_t numAttribs = attributeHandleSet.size();
    for (RTI::ULong i = 0; i < numAttribs; ++i) {
        unsigned index = getAttributeIndex(attributeHandleSet[i]);
        _attributeData[index]._dirty = true;
    }
}

void
RTI13ObjectInstance::updateAttributeValues(const RTIData& tag)
{
    if (!_ambassador.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "Error: Ambassador is zero.");
        return;
    }

    try {
        // That means clear()
        _attributeValuePairSet->empty();

        unsigned numAttributes = getNumAttributes();
        for (unsigned i = 0; i < numAttributes; ++i) {
            if (!_attributeData[i]._dirty)
                continue;
            const RTIData& data = _attributeData[i]._data;
            _attributeValuePairSet->add(getAttributeHandle(i), data.data(), data.size());
        }

        if (!_attributeValuePairSet->size())
            return;

        _ambassador->updateAttributeValues(_handle, *_attributeValuePairSet, tag);

        for (unsigned i = 0; i < numAttributes; ++i) {
            _attributeData[i]._dirty = false;
        }

    } catch (RTI::ObjectNotKnown& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: could not update attribute values object instance: " << e._name << " " << e._reason);
    } catch (RTI::AttributeNotDefined& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: could not update attribute values object instance: " << e._name << " " << e._reason);
    } catch (RTI::AttributeNotOwned& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: could not update attribute values object instance: " << e._name << " " << e._reason);
    } catch (RTI::FederateNotExecutionMember& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: could not update attribute values object instance: " << e._name << " " << e._reason);
    } catch (RTI::ConcurrentAccessAttempted& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: could not update attribute values object instance: " << e._name << " " << e._reason);
    } catch (RTI::SaveInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: could not update attribute values object instance: " << e._name << " " << e._reason);
    } catch (RTI::RestoreInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: could not update attribute values object instance: " << e._name << " " << e._reason);
    } catch (RTI::RTIinternalError& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: could not update attribute values object instance: " << e._name << " " << e._reason);
    }

    // That means clear()
    _attributeValuePairSet->empty();
}

void
RTI13ObjectInstance::updateAttributeValues(const SGTimeStamp& timeStamp, const RTIData& tag)
{
    if (!_ambassador.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "Error: Ambassador is zero.");
        return;
    }

    try {
        // That means clear()
        _attributeValuePairSet->empty();

        unsigned numAttributes = getNumAttributes();
        for (unsigned i = 0; i < numAttributes; ++i) {
            if (!_attributeData[i]._dirty)
                continue;
            const RTIData& data = _attributeData[i]._data;
            _attributeValuePairSet->add(getAttributeHandle(i), data.data(), data.size());
        }

        if (!_attributeValuePairSet->size())
            return;

        _ambassador->updateAttributeValues(_handle, *_attributeValuePairSet, timeStamp, tag);

        for (unsigned i = 0; i < numAttributes; ++i) {
            _attributeData[i]._dirty = false;
        }

    } catch (RTI::ObjectNotKnown& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: could not update attribute values object instance: " << e._name << " " << e._reason);
    } catch (RTI::AttributeNotDefined& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: could not update attribute values object instance: " << e._name << " " << e._reason);
    } catch (RTI::AttributeNotOwned& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: could not update attribute values object instance: " << e._name << " " << e._reason);
    } catch (RTI::InvalidFederationTime& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: could not update attribute values object instance: " << e._name << " " << e._reason);
    } catch (RTI::FederateNotExecutionMember& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: could not update attribute values object instance: " << e._name << " " << e._reason);
    } catch (RTI::ConcurrentAccessAttempted& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: could not update attribute values object instance: " << e._name << " " << e._reason);
    } catch (RTI::SaveInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: could not update attribute values object instance: " << e._name << " " << e._reason);
    } catch (RTI::RestoreInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: could not update attribute values object instance: " << e._name << " " << e._reason);
    } catch (RTI::RTIinternalError& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: could not update attribute values object instance: " << e._name << " " << e._reason);
    }

    // That means clear()
    _attributeValuePairSet->empty();
}

void
RTI13ObjectInstance::attributesInScope(const std::vector<RTI::AttributeHandle>& attributeHandleSet)
{
    size_t numAttribs = attributeHandleSet.size();
    for (RTI::ULong i = 0; i < numAttribs; ++i) {
        RTI::AttributeHandle attributeHandle = attributeHandleSet[i];
        setAttributeInScope(getAttributeIndex(attributeHandle), true);
    }
}

void
RTI13ObjectInstance::attributesOutOfScope(const std::vector<RTI::AttributeHandle>& attributeHandleSet)
{
    size_t numAttribs = attributeHandleSet.size();
    for (RTI::ULong i = 0; i < numAttribs; ++i) {
        RTI::AttributeHandle attributeHandle = attributeHandleSet[i];
        setAttributeInScope(getAttributeIndex(attributeHandle), false);
    }
}

void
RTI13ObjectInstance::turnUpdatesOnForObjectInstance(const std::vector<RTI::AttributeHandle>& attributeHandleSet)
{
    size_t numAttribs = attributeHandleSet.size();
    for (RTI::ULong i = 0; i < numAttribs; ++i) {
        RTI::AttributeHandle attributeHandle = attributeHandleSet[i];
        setAttributeUpdateEnabled(getAttributeIndex(attributeHandle), true);
    }
}

void
RTI13ObjectInstance::turnUpdatesOffForObjectInstance(const std::vector<RTI::AttributeHandle>& attributeHandleSet)
{
    size_t numAttribs = attributeHandleSet.size();
    for (RTI::ULong i = 0; i < numAttribs; ++i) {
        RTI::AttributeHandle attributeHandle = attributeHandleSet[i];
        setAttributeUpdateEnabled(getAttributeIndex(attributeHandle), false);
    }
}

bool
RTI13ObjectInstance::isAttributeOwnedByFederate(unsigned index) const
{
    if (!_ambassador.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "Error: Ambassador is zero.");
        return false;
    }
    try {
        RTI::AttributeHandle attributeHandle = getAttributeHandle(index);
        return _ambassador->isAttributeOwnedByFederate(_handle, attributeHandle);
    } catch (RTI::ObjectNotKnown& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not query attribute ownership for index " << index << ": "
               << e._name << " " << e._reason);
        return false;
    } catch (RTI::AttributeNotDefined& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not query attribute ownership for index " << index << ": "
               << e._name << " " << e._reason);
        return false;
    } catch (RTI::FederateNotExecutionMember& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not query attribute ownership for index " << index << ": "
               << e._name << " " << e._reason);
        return false;
    } catch (RTI::ConcurrentAccessAttempted& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not query attribute ownership for index " << index << ": "
               << e._name << " " << e._reason);
        return false;
    } catch (RTI::SaveInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not query attribute ownership for index " << index << ": "
               << e._name << " " << e._reason);
        return false;
    } catch (RTI::RestoreInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not query attribute ownership for index " << index << ": "
               << e._name << " " << e._reason);
        return false;
    } catch (RTI::RTIinternalError& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not query attribute ownership for index " << index << ": "
               << e._name << " " << e._reason);
        return false;
    }
    return false;
 }

void
RTI13ObjectInstance::requestAttributeOwnershipAssumption(const std::vector<RTI::AttributeHandle>& attributes, const RTIData& tag)
{
}

void
RTI13ObjectInstance::attributeOwnershipDivestitureNotification(const std::vector<RTI::AttributeHandle>& attributes)
{
}

void
RTI13ObjectInstance::attributeOwnershipAcquisitionNotification(const std::vector<RTI::AttributeHandle>& attributes)
{
}

void
RTI13ObjectInstance::attributeOwnershipUnavailable(const std::vector<RTI::AttributeHandle>& attributes)
{
}

void
RTI13ObjectInstance::requestAttributeOwnershipRelease(const std::vector<RTI::AttributeHandle>& attributes, const RTIData& tag)
{
}

void
RTI13ObjectInstance::confirmAttributeOwnershipAcquisitionCancellation(const std::vector<RTI::AttributeHandle>& attributes)
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
