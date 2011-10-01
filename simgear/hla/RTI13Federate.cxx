// Copyright (C) 2009 - 2011  Mathias Froehlich - Mathias.Froehlich@web.de
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

#include "RTI13Federate.hxx"

#include "RTI13Ambassador.hxx"

namespace simgear {

static std::string toStdString(const char* n)
{
    if (!n)
        return std::string();
    return std::string(n);
}

/// Just the interface class doing the callbacks into the parent class
struct RTI13Federate::FederateAmbassador : public RTI::FederateAmbassador {
    FederateAmbassador() :
        _timeRegulationEnabled(false),
        _timeConstrainedEnabled(false),
        _timeAdvancePending(false)
    {
    }
    virtual ~FederateAmbassador()
        throw (RTI::FederateInternalError)
    {
    }

    /// RTI federate ambassador callback functions.
    virtual void synchronizationPointRegistrationSucceeded(const char* label)
        throw (RTI::FederateInternalError)
    {
    }

    virtual void synchronizationPointRegistrationFailed(const char* label)
        throw (RTI::FederateInternalError)
    {
    }

    virtual void announceSynchronizationPoint(const char* label, const char* tag)
        throw (RTI::FederateInternalError)
    {
        _pendingSyncLabels.insert(toStdString(label));
    }

    virtual void federationSynchronized(const char* label)
        throw (RTI::FederateInternalError)
    {
        std::string s = toStdString(label);
        _pendingSyncLabels.erase(s);
        _syncronizedSyncLabels.insert(s);
    }

    virtual void initiateFederateSave(const char* label)
        throw (RTI::UnableToPerformSave,
               RTI::FederateInternalError)
    {
    }

    virtual void federationSaved()
        throw (RTI::FederateInternalError)
    {
    }

    virtual void federationNotSaved()
        throw (RTI::FederateInternalError)
    {
    }

    virtual void requestFederationRestoreSucceeded(const char* label)
        throw (RTI::FederateInternalError)
    {
    }

    virtual void requestFederationRestoreFailed(const char* label, const char* reason)
        throw (RTI::FederateInternalError)
    {
    }

    virtual void federationRestoreBegun()
        throw (RTI::FederateInternalError)
    {
    }

    virtual void initiateFederateRestore(const char* label, RTI::FederateHandle federateHandle)
        throw (RTI::SpecifiedSaveLabelDoesNotExist,
               RTI::CouldNotRestore,
               RTI::FederateInternalError)
    {
    }

    virtual void federationRestored()
        throw (RTI::FederateInternalError)
    {
    }

    virtual void federationNotRestored()
        throw (RTI::FederateInternalError)
    {
    }

    // Declaration Management
    virtual void startRegistrationForObjectClass(RTI::ObjectClassHandle objectClassHandle)
        throw (RTI::ObjectClassNotPublished,
               RTI::FederateInternalError)
    {
        ObjectClassMap::iterator i = _objectClassMap.find(objectClassHandle);
        if (i == _objectClassMap.end())
            return;
        if (!i->second.valid())
            return;
        i->second->startRegistration();
    }

    virtual void stopRegistrationForObjectClass(RTI::ObjectClassHandle objectClassHandle)
        throw (RTI::ObjectClassNotPublished,
               RTI::FederateInternalError)
    {
        ObjectClassMap::iterator i = _objectClassMap.find(objectClassHandle);
        if (i == _objectClassMap.end())
            return;
        if (!i->second.valid())
            return;
        i->second->stopRegistration();
    }

    virtual void turnInteractionsOn(RTI::InteractionClassHandle interactionClassHandle)
        throw (RTI::InteractionClassNotPublished,
               RTI::FederateInternalError)
    {
    }

    virtual void turnInteractionsOff(RTI::InteractionClassHandle interactionClassHandle)
        throw (RTI::InteractionClassNotPublished,
               RTI::FederateInternalError)
    {
    }


    // Object Management
    virtual void discoverObjectInstance(RTI::ObjectHandle objectHandle, RTI::ObjectClassHandle objectClassHandle, const char* tag)
        throw (RTI::CouldNotDiscover,
               RTI::ObjectClassNotKnown,
               RTI::FederateInternalError)
    {
        ObjectClassMap::iterator i = _objectClassMap.find(objectClassHandle);
        if (i == _objectClassMap.end())
            throw RTI::ObjectClassNotKnown("Federate: discoverObjectInstance()!");
        if (!i->second.valid())
            return;
        SGSharedPtr<RTI13ObjectInstance> objectInstance = new RTI13ObjectInstance(objectHandle, 0, i->second, _rtiAmbassador.get(), false);
        _objectInstanceMap[objectHandle] = objectInstance;
        _queueCallbackList.push_back(new DiscoverObjectCallback(i->second, objectInstance, tagToData(tag)));
    }

    virtual void reflectAttributeValues(RTI::ObjectHandle objectHandle, const RTI::AttributeHandleValuePairSet& attributeValuePairSet,
                                        const RTI::FedTime& fedTime, const char* tag, RTI::EventRetractionHandle eventRetractionHandle)
        throw (RTI::ObjectNotKnown,
               RTI::AttributeNotKnown,
               RTI::FederateOwnsAttributes,
               RTI::InvalidFederationTime,
               RTI::FederateInternalError)
    {
        ObjectInstanceMap::iterator i = _objectInstanceMap.find(objectHandle);
        if (i == _objectInstanceMap.end())
            throw RTI::ObjectNotKnown("Reflect attributes for unknown object!");
        if (!i->second.valid())
            return;
        i->second->reflectAttributeValues(attributeValuePairSet, RTI13Ambassador::toTimeStamp(fedTime), tagToData(tag));
    }

    virtual void reflectAttributeValues(RTI::ObjectHandle objectHandle, const RTI::AttributeHandleValuePairSet& attributeValuePairSet,
                                        const char* tag)
        throw (RTI::ObjectNotKnown,
               RTI::AttributeNotKnown,
               RTI::FederateOwnsAttributes,
               RTI::FederateInternalError)
    {
        ObjectInstanceMap::iterator i = _objectInstanceMap.find(objectHandle);
        if (i == _objectInstanceMap.end())
            throw RTI::ObjectNotKnown("Reflect attributes for unknown object!");
        if (!i->second.valid())
            return;
        i->second->reflectAttributeValues(attributeValuePairSet, tagToData(tag));
    }

    virtual void receiveInteraction(RTI::InteractionClassHandle interactionClassHandle, const RTI::ParameterHandleValuePairSet& parameters,
                                    const RTI::FedTime& fedTime, const char* tag, RTI::EventRetractionHandle eventRetractionHandle)
        throw (RTI::InteractionClassNotKnown,
               RTI::InteractionParameterNotKnown,
               RTI::InvalidFederationTime,
               RTI::FederateInternalError)
    {
    }

    virtual void receiveInteraction(RTI::InteractionClassHandle interactionClassHandle,
                                    const RTI::ParameterHandleValuePairSet& parameters, const char* tag)
        throw (RTI::InteractionClassNotKnown,
               RTI::InteractionParameterNotKnown,
               RTI::FederateInternalError)
    {
    }

    virtual void removeObjectInstance(RTI::ObjectHandle objectHandle, const RTI::FedTime& fedTime,
                                      const char* tag, RTI::EventRetractionHandle eventRetractionHandle)
        throw (RTI::ObjectNotKnown,
               RTI::InvalidFederationTime,
               RTI::FederateInternalError)
    {
        ObjectInstanceMap::iterator i = _objectInstanceMap.find(objectHandle);
        if (i == _objectInstanceMap.end())
            throw RTI::ObjectNotKnown("Federate: removeObjectInstance()!");
        if (i->second.valid())
            _queueCallbackList.push_back(new RemoveObjectCallback(i->second, tagToData(tag)));
        _objectInstanceMap.erase(i);
    }

    virtual void removeObjectInstance(RTI::ObjectHandle objectHandle, const char* tag)
        throw (RTI::ObjectNotKnown,
               RTI::FederateInternalError)
    {
        ObjectInstanceMap::iterator i = _objectInstanceMap.find(objectHandle);
        if (i == _objectInstanceMap.end())
            throw RTI::ObjectNotKnown("Federate: removeObjectInstance()!");
        if (i->second.valid())
            _queueCallbackList.push_back(new RemoveObjectCallback(i->second, tagToData(tag)));
        _objectInstanceMap.erase(i);
    }

    virtual void attributesInScope(RTI::ObjectHandle objectHandle, const RTI::AttributeHandleSet& attributes)
        throw (RTI::ObjectNotKnown,
               RTI::AttributeNotKnown,
               RTI::FederateInternalError)
    {
        ObjectInstanceMap::iterator i = _objectInstanceMap.find(objectHandle);
        if (i == _objectInstanceMap.end())
            throw RTI::ObjectNotKnown("Attributes in scope for unknown object!");
        if (!i->second.valid())
            return;
        i->second->attributesInScope(attributes);
    }

    virtual void attributesOutOfScope(RTI::ObjectHandle objectHandle, const RTI::AttributeHandleSet& attributes)
        throw (RTI::ObjectNotKnown,
               RTI::AttributeNotKnown,
               RTI::FederateInternalError)
    {
        ObjectInstanceMap::iterator i = _objectInstanceMap.find(objectHandle);
        if (i == _objectInstanceMap.end())
            throw RTI::ObjectNotKnown("Attributes in scope for unknown object!");
        if (!i->second.valid())
            return;
        i->second->attributesOutOfScope(attributes);
    }

    virtual void provideAttributeValueUpdate(RTI::ObjectHandle objectHandle, const RTI::AttributeHandleSet& attributes)
        throw (RTI::ObjectNotKnown,
               RTI::AttributeNotKnown,
               RTI::AttributeNotOwned,
               RTI::FederateInternalError)
    {
        ObjectInstanceMap::iterator i = _objectInstanceMap.find(objectHandle);
        if (i == _objectInstanceMap.end())
            throw RTI::ObjectNotKnown("Reflect attributes for unknown object!");
        if (!i->second.valid())
            return;
        i->second->provideAttributeValueUpdate(attributes);
    }

    virtual void turnUpdatesOnForObjectInstance(RTI::ObjectHandle objectHandle, const RTI::AttributeHandleSet& attributes)
        throw (RTI::ObjectNotKnown,
               RTI::AttributeNotOwned,
               RTI::FederateInternalError)
    {
        ObjectInstanceMap::iterator i = _objectInstanceMap.find(objectHandle);
        if (i == _objectInstanceMap.end())
            throw RTI::ObjectNotKnown("Turn on attributes for unknown object!");
        if (!i->second.valid())
            return;
        i->second->turnUpdatesOnForObjectInstance(attributes);
    }

    virtual void turnUpdatesOffForObjectInstance(RTI::ObjectHandle objectHandle, const RTI::AttributeHandleSet& attributes)
        throw (RTI::ObjectNotKnown,
               RTI::AttributeNotOwned,
               RTI::FederateInternalError)
    {
        ObjectInstanceMap::iterator i = _objectInstanceMap.find(objectHandle);
        if (i == _objectInstanceMap.end())
            throw RTI::ObjectNotKnown("Turn off attributes for unknown object!");
        if (!i->second.valid())
            return;
        i->second->turnUpdatesOffForObjectInstance(attributes);
    }

    // Ownership Management
    virtual void requestAttributeOwnershipAssumption(RTI::ObjectHandle objectHandle,
                                                     const RTI::AttributeHandleSet& attributes, const char* tag)
        throw (RTI::ObjectNotKnown,
               RTI::AttributeNotKnown,
               RTI::AttributeAlreadyOwned,
               RTI::AttributeNotPublished,
               RTI::FederateInternalError)
    {
        ObjectInstanceMap::iterator i = _objectInstanceMap.find(objectHandle);
        if (i == _objectInstanceMap.end())
            throw RTI::ObjectNotKnown("requestAttributeOwnershipAssumption for unknown object!");
        if (!i->second.valid())
            return;
        i->second->requestAttributeOwnershipAssumption(attributes, tagToData(tag));
    }

    virtual void attributeOwnershipDivestitureNotification(RTI::ObjectHandle objectHandle, const RTI::AttributeHandleSet& attributes)
        throw (RTI::ObjectNotKnown,
               RTI::AttributeNotKnown,
               RTI::AttributeNotOwned,
               RTI::AttributeDivestitureWasNotRequested,
               RTI::FederateInternalError)
    {
        ObjectInstanceMap::iterator i = _objectInstanceMap.find(objectHandle);
        if (i == _objectInstanceMap.end())
            throw RTI::ObjectNotKnown("attributeOwnershipDivestitureNotification for unknown object!");
        if (!i->second.valid())
            return;
        i->second->attributeOwnershipDivestitureNotification(attributes);
    }

    virtual void attributeOwnershipAcquisitionNotification(RTI::ObjectHandle objectHandle, const RTI::AttributeHandleSet& attributes)
        throw (RTI::ObjectNotKnown,
               RTI::AttributeNotKnown,
               RTI::AttributeAcquisitionWasNotRequested,
               RTI::AttributeAlreadyOwned,
               RTI::AttributeNotPublished,
               RTI::FederateInternalError)
    {
        ObjectInstanceMap::iterator i = _objectInstanceMap.find(objectHandle);
        if (i == _objectInstanceMap.end())
            throw RTI::ObjectNotKnown("attributeOwnershipAcquisitionNotification for unknown object!");
        if (!i->second.valid())
            return;
        i->second->attributeOwnershipAcquisitionNotification(attributes);
    }

    virtual void attributeOwnershipUnavailable(RTI::ObjectHandle objectHandle, const RTI::AttributeHandleSet& attributes)
        throw (RTI::ObjectNotKnown,
               RTI::AttributeNotKnown,
               RTI::AttributeAlreadyOwned,
               RTI::AttributeAcquisitionWasNotRequested,
               RTI::FederateInternalError)
    {
        ObjectInstanceMap::iterator i = _objectInstanceMap.find(objectHandle);
        if (i == _objectInstanceMap.end())
            throw RTI::ObjectNotKnown("attributeOwnershipUnavailable for unknown object!");
        if (!i->second.valid())
            return;
        i->second->attributeOwnershipUnavailable(attributes);
    }

    virtual void requestAttributeOwnershipRelease(RTI::ObjectHandle objectHandle,
                                                  const RTI::AttributeHandleSet& attributes, const char* tag)
        throw (RTI::ObjectNotKnown,
               RTI::AttributeNotKnown,
               RTI::AttributeNotOwned,
               RTI::FederateInternalError)
    {
        ObjectInstanceMap::iterator i = _objectInstanceMap.find(objectHandle);
        if (i == _objectInstanceMap.end())
            throw RTI::ObjectNotKnown("requestAttributeOwnershipRelease for unknown object!");
        if (!i->second.valid())
            return;
        i->second->requestAttributeOwnershipRelease(attributes, tagToData(tag));
    }

    virtual void confirmAttributeOwnershipAcquisitionCancellation(RTI::ObjectHandle objectHandle, const RTI::AttributeHandleSet& attributes)
        throw (RTI::ObjectNotKnown,
               RTI::AttributeNotKnown,
               RTI::AttributeAlreadyOwned,
               RTI::AttributeAcquisitionWasNotCanceled,
               RTI::FederateInternalError)
    {
        ObjectInstanceMap::iterator i = _objectInstanceMap.find(objectHandle);
        if (i == _objectInstanceMap.end())
            throw RTI::ObjectNotKnown("confirmAttributeOwnershipAcquisitionCancellation for unknown object!");
        if (!i->second.valid())
            return;
        i->second->confirmAttributeOwnershipAcquisitionCancellation(attributes);
    }

    virtual void informAttributeOwnership(RTI::ObjectHandle objectHandle, RTI::AttributeHandle attributeHandle,
                                          RTI::FederateHandle federateHandle)
        throw (RTI::ObjectNotKnown,
               RTI::AttributeNotKnown,
               RTI::FederateInternalError)
    {
        ObjectInstanceMap::iterator i = _objectInstanceMap.find(objectHandle);
        if (i == _objectInstanceMap.end())
            throw RTI::ObjectNotKnown("informAttributeOwnership for unknown object!");
        if (!i->second.valid())
            return;
        i->second->informAttributeOwnership(attributeHandle, federateHandle);
    }

    virtual void attributeIsNotOwned(RTI::ObjectHandle objectHandle, RTI::AttributeHandle attributeHandle)
        throw (RTI::ObjectNotKnown,
               RTI::AttributeNotKnown,
               RTI::FederateInternalError)
    {
        ObjectInstanceMap::iterator i = _objectInstanceMap.find(objectHandle);
        if (i == _objectInstanceMap.end())
            throw RTI::ObjectNotKnown("attributeIsNotOwned for unknown object!");
        if (!i->second.valid())
            return;
        i->second->attributeIsNotOwned(attributeHandle);
    }

    virtual void attributeOwnedByRTI(RTI::ObjectHandle objectHandle, RTI::AttributeHandle attributeHandle)
        throw (RTI::ObjectNotKnown,
               RTI::AttributeNotKnown,
               RTI::FederateInternalError)
    {
        ObjectInstanceMap::iterator i = _objectInstanceMap.find(objectHandle);
        if (i == _objectInstanceMap.end())
            throw RTI::ObjectNotKnown("attributeOwnedByRTI for unknown object!");
        if (!i->second.valid())
            return;
        i->second->attributeOwnedByRTI(attributeHandle);
    }

    // Time Management
    virtual void timeRegulationEnabled(const RTI::FedTime& fedTime)
        throw (RTI::InvalidFederationTime,
               RTI::EnableTimeRegulationWasNotPending,
               RTI::FederateInternalError)
    {
        _timeRegulationEnabled = true;
        _federateTime = RTI13Ambassador::toTimeStamp(fedTime);
        SG_LOG(SG_NETWORK, SG_INFO, "RTI: timeRegulationEnabled: " << _federateTime);
    }

    virtual void timeConstrainedEnabled(const RTI::FedTime& fedTime)
        throw (RTI::InvalidFederationTime,
               RTI::EnableTimeConstrainedWasNotPending,
               RTI::FederateInternalError)
    {
        _timeConstrainedEnabled = true;
        _federateTime = RTI13Ambassador::toTimeStamp(fedTime);
        SG_LOG(SG_NETWORK, SG_INFO, "RTI: timeConstrainedEnabled: " << _federateTime);
    }

    virtual void timeAdvanceGrant(const RTI::FedTime& fedTime)
        throw (RTI::InvalidFederationTime,
               RTI::TimeAdvanceWasNotInProgress,
               RTI::FederateInternalError)
    {
        _federateTime = RTI13Ambassador::toTimeStamp(fedTime);
        _timeAdvancePending = false;
        SG_LOG(SG_NETWORK, SG_INFO, "RTI: timeAdvanceGrant: " << _federateTime);
    }

    virtual void requestRetraction(RTI::EventRetractionHandle eventRetractionHandle)
        throw (RTI::EventNotKnown,
               RTI::FederateInternalError)
    {
        // No retraction concept yet
    }

    // processes the queues that filled up during the past
    void processQueues()
    {
        while (!_queueCallbackList.empty()) {
            (*_queueCallbackList.front())();
            _queueCallbackList.pop_front();
        }
    }

    bool getFederationSynchronizationPointAnnounced(const std::string& label)
    { return _pendingSyncLabels.find(label) != _pendingSyncLabels.end(); }
    bool getFederationSynchronized(const std::string& label)
    {
        std::set<std::string>::iterator i = _syncronizedSyncLabels.find(label);
        if (i == _syncronizedSyncLabels.end())
            return false;
        _syncronizedSyncLabels.erase(i);
        return true;
    }

    /// Generic callback to execute some notification on objects in a way that they are not prone to
    /// ConcurrentAccess exceptions.
    class QueueCallback : public SGReferenced {
    public:
        virtual ~QueueCallback() {}
        virtual void operator()() = 0;
    };

    class DiscoverObjectCallback : public QueueCallback {
    public:
        DiscoverObjectCallback(SGSharedPtr<RTIObjectClass> objectClass, SGSharedPtr<RTIObjectInstance> objectInstance, const RTIData& tag) :
            _objectClass(objectClass),
            _objectInstance(objectInstance),
            _tag(tag)
        { }
        virtual void operator()()
        {
            _objectClass->discoverInstance(_objectInstance.get(), _tag);
            _objectInstance->requestObjectAttributeValueUpdate();
        }
    private:
        SGSharedPtr<RTIObjectClass> _objectClass;
        SGSharedPtr<RTIObjectInstance> _objectInstance;
        RTIData _tag;
    };
    class RemoveObjectCallback : public QueueCallback {
    public:
        RemoveObjectCallback(SGSharedPtr<RTIObjectInstance> objectInstance, const RTIData& tag) :
            _objectInstance(objectInstance),
            _tag(tag)
        { }
        virtual void operator()()
        {
            _objectInstance->removeInstance(_tag);
        }
    private:
        SGSharedPtr<RTIObjectInstance> _objectInstance;
        RTIData _tag;
    };

    // The rtiambassador to issue requests
    SGSharedPtr<RTI13Ambassador> _rtiAmbassador;

    // All the sync labels we got an announcement for
    std::set<std::string> _pendingSyncLabels;
    std::set<std::string> _syncronizedSyncLabels;

    // All that calls back into user code is just queued.
    // That is to make sure we do not call recursively into the RTI
    typedef std::list<SGSharedPtr<QueueCallback> > QueueCallbackList;
    QueueCallbackList _queueCallbackList;

    // Top level information for dispatching federate object attribute updates
    typedef std::map<RTI::ObjectHandle, SGSharedPtr<RTI13ObjectInstance> > ObjectInstanceMap;
    // Map of all available objects
    ObjectInstanceMap _objectInstanceMap;

    // Top level information for dispatching creation of federate objects
    typedef std::map<RTI::ObjectClassHandle, SGSharedPtr<RTI13ObjectClass> > ObjectClassMap;
    ObjectClassMap _objectClassMap;

    bool _timeRegulationEnabled;
    bool _timeConstrainedEnabled;
    bool _timeAdvancePending;
    SGTimeStamp _federateTime;

private:
    const RTIData& tagToData(const char* tag)
    {
        if (tag)
            _cachedTag.setData(tag, std::strlen(tag) + 1);
        else
            _cachedTag.setData("", 1);
        return _cachedTag;
    }
    RTIData _cachedTag;
};

RTI13Federate::RTI13Federate(const std::list<std::string>& stringList) :
    _joined(false),
    _tickTimeout(10),
    _ambassador(new RTI13Ambassador),
    _federateAmbassador(new FederateAmbassador)
{
    _ambassador->_federate = this;
    _federateAmbassador->_rtiAmbassador = _ambassador;
    if (stringList.empty()) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Ignoring non empty connect arguments while connecting to an RTI13 federation!");
    }
}

RTI13Federate::~RTI13Federate()
{
    if (_joined)
        _ambassador->resignFederationExecution();
    delete _federateAmbassador;
}

bool
RTI13Federate::createFederationExecution(const std::string& federationName, const std::string& objectModel)
{
    try {
        _ambassador->createFederationExecution(federationName, objectModel);
        return true;
    } catch (RTI::FederationExecutionAlreadyExists& e) {
        return true;
    } catch (RTI::CouldNotOpenFED& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not create federation execution: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::ErrorReadingFED& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not create federation execution: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::ConcurrentAccessAttempted& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not create federation execution: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::RTIinternalError& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not create federation execution: " << e._name << " " << e._reason);
        return false;
    }
}

bool
RTI13Federate::destroyFederationExecution(const std::string& federation)
{
    try {
        _ambassador->destroyFederationExecution(federation);
        return true;
    } catch (RTI::FederatesCurrentlyJoined& e) {
        return true;
    } catch (RTI::FederationExecutionDoesNotExist& e) {
        return true;
    } catch (RTI::ConcurrentAccessAttempted& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not destroy federation execution: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::RTIinternalError& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not destroy federation execution: " << e._name << " " << e._reason);
        return false;
    }
}

bool
RTI13Federate::join(const std::string& federateType, const std::string& federationName)
{
    try {
        _federateHandle = _ambassador->joinFederationExecution(federateType, federationName, _federateAmbassador);
        SG_LOG(SG_NETWORK, SG_INFO, "RTI: Joined federation \""
               << federationName << "\" as \"" << federateType << "\"");
        setFederateType(federateType);
        setFederationName(federationName);
        _joined = true;
        return true;
    } catch (RTI::FederateAlreadyExecutionMember& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not join federation execution: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::FederationExecutionDoesNotExist& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not join federation execution: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::CouldNotOpenFED& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not join federation execution: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::ErrorReadingFED& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not join federation execution: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::ConcurrentAccessAttempted& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not join federation execution: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::SaveInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not join federation execution: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::RestoreInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not join federation execution: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::RTIinternalError& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not join federation execution: " << e._name << " " << e._reason);
        return false;
    }
}

bool
RTI13Federate::resign()
{
    try {
        _ambassador->resignFederationExecution();
        SG_LOG(SG_NETWORK, SG_INFO, "RTI: Resigned from federation.");
        _joined = false;
        _federateHandle = -1;
        return true;
    } catch (RTI::FederateOwnsAttributes& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not resign federation execution: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::FederateNotExecutionMember& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not resign federation execution: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::InvalidResignAction& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not resign federation execution: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::ConcurrentAccessAttempted& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not resign federation execution: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::RTIinternalError& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not resign federation execution: " << e._name << " " << e._reason);
        return false;
    }
}

bool
RTI13Federate::registerFederationSynchronizationPoint(const std::string& label, const RTIData& tag)
{
    try {
        _ambassador->registerFederationSynchronizationPoint(label, tag);
        SG_LOG(SG_NETWORK, SG_INFO, "RTI: registerFederationSynchronizationPoint(" << label << ", tag )");
        return true;
    } catch (RTI::FederateNotExecutionMember& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not register federation synchronization point: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::ConcurrentAccessAttempted& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not register federation synchronization point: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::SaveInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not register federation synchronization point: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::RestoreInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not register federation synchronization point: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::RTIinternalError& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not register federation synchronization point: " << e._name << " " << e._reason);
        return false;
    }
}

bool
RTI13Federate::waitForFederationSynchronizationPointAnnounced(const std::string& label)
{
    while (!_federateAmbassador->getFederationSynchronizationPointAnnounced(label)) {
        _ambassador->tick(_tickTimeout, 0);
        _federateAmbassador->processQueues();
    }
    return true;
}

bool
RTI13Federate::synchronizationPointAchieved(const std::string& label)
{
    try {
        _ambassador->synchronizationPointAchieved(label);
        SG_LOG(SG_NETWORK, SG_INFO, "RTI: synchronizationPointAchieved(" << label << ")");
        return true;
    } catch (RTI::SynchronizationPointLabelWasNotAnnounced& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not signal synchronization point: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::FederateNotExecutionMember& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not signal synchronization point: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::ConcurrentAccessAttempted& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not signal synchronization point: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::SaveInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not signal synchronization point: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::RestoreInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not signal synchronization point: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::RTIinternalError& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not signal synchronization point: " << e._name << " " << e._reason);
        return false;
    }
}

bool
RTI13Federate::waitForFederationSynchronized(const std::string& label)
{
    while (!_federateAmbassador->getFederationSynchronized(label)) {
        _ambassador->tick(_tickTimeout, 0);
        _federateAmbassador->processQueues();
    }
    return true;
}

bool
RTI13Federate::enableTimeConstrained()
{
    if (!_ambassador.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not enable time constrained at unconnected federate.");
        return false;
    }

    if (_federateAmbassador->_timeConstrainedEnabled) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Time constrained is already enabled.");
        return false;
    }

    try {
        _ambassador->enableTimeConstrained();
    } catch (RTI::TimeConstrainedAlreadyEnabled& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not resign federation execution: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::EnableTimeConstrainedPending& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not resign federation execution: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::TimeAdvanceAlreadyInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not resign federation execution: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::ConcurrentAccessAttempted& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not resign federation execution: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::FederateNotExecutionMember& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not resign federation execution: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::SaveInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not resign federation execution: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::RestoreInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not resign federation execution: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::RTIinternalError& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not resign federation execution: " << e._name << " " << e._reason);
        return false;
    }

    while (!_federateAmbassador->_timeConstrainedEnabled) {
        _ambassador->tick(_tickTimeout, 0);
        _federateAmbassador->processQueues();
    }

    return true;
}

bool
RTI13Federate::disableTimeConstrained()
{
    if (!_ambassador.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not disable time constrained at unconnected federate.");
        return false;
    }

    if (!_federateAmbassador->_timeConstrainedEnabled) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Time constrained is not enabled.");
        return false;
    }

    try {
        _ambassador->disableTimeConstrained();
        _federateAmbassador->_timeConstrainedEnabled = false;
    } catch (RTI::TimeConstrainedWasNotEnabled& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not resign federation execution: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::FederateNotExecutionMember& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not resign federation execution: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::ConcurrentAccessAttempted& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not resign federation execution: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::SaveInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not resign federation execution: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::RestoreInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not resign federation execution: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::RTIinternalError& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not resign federation execution: " << e._name << " " << e._reason);
        return false;
    }

    return true;
}

bool
RTI13Federate::enableTimeRegulation(const SGTimeStamp& lookahead)
{
    if (!_ambassador.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not enable time regulation at unconnected federate.");
        return false;
    }

    if (_federateAmbassador->_timeRegulationEnabled) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Time regulation already enabled.");
        return false;
    }

    try {
        _ambassador->enableTimeRegulation(lookahead);
    } catch (RTI::TimeRegulationAlreadyEnabled& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not resign federation execution: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::EnableTimeRegulationPending& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not resign federation execution: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::TimeAdvanceAlreadyInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not resign federation execution: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::InvalidFederationTime& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not resign federation execution: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::InvalidLookahead& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not resign federation execution: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::ConcurrentAccessAttempted& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not resign federation execution: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::FederateNotExecutionMember& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not resign federation execution: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::SaveInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not resign federation execution: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::RestoreInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not resign federation execution: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::RTIinternalError& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not resign federation execution: " << e._name << " " << e._reason);
        return false;
    }

    while (!_federateAmbassador->_timeRegulationEnabled) {
        _ambassador->tick(_tickTimeout, 0);
        _federateAmbassador->processQueues();
    }

    return true;
}

bool
RTI13Federate::disableTimeRegulation()
{
    if (!_ambassador.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not disable time regulation at unconnected federate.");
        return false;
    }

    if (!_federateAmbassador->_timeRegulationEnabled) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Time regulation is not enabled.");
        return false;
    }

    try {
        _ambassador->disableTimeRegulation();
        _federateAmbassador->_timeRegulationEnabled = false;
    } catch (RTI::TimeRegulationWasNotEnabled& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not resign federation execution: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::ConcurrentAccessAttempted& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not resign federation execution: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::FederateNotExecutionMember& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not resign federation execution: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::SaveInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not resign federation execution: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::RestoreInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not resign federation execution: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::RTIinternalError& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not resign federation execution: " << e._name << " " << e._reason);
        return false;
    }

    return true;
}

bool
RTI13Federate::timeAdvanceRequestBy(const SGTimeStamp& dt)
{
    if (!_ambassador.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not disable time regulation at unconnected federate.");
        return false;
    }

    SGTimeStamp fedTime = _federateAmbassador->_federateTime + dt;
    return timeAdvanceRequest(fedTime);
}

bool
RTI13Federate::timeAdvanceRequest(const SGTimeStamp& fedTime)
{
    if (!_ambassador.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not disable time regulation at unconnected federate.");
        return false;
    }

    try {
        _ambassador->timeAdvanceRequest(fedTime);
        _federateAmbassador->_timeAdvancePending = true;
    } catch (RTI::InvalidFederationTime& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not resign federation execution: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::FederationTimeAlreadyPassed& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not resign federation execution: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::TimeAdvanceAlreadyInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not resign federation execution: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::EnableTimeRegulationPending& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not resign federation execution: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::EnableTimeConstrainedPending& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not resign federation execution: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::FederateNotExecutionMember& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not resign federation execution: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::ConcurrentAccessAttempted& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not resign federation execution: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::SaveInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not resign federation execution: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::RestoreInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not resign federation execution: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::RTIinternalError& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not resign federation execution: " << e._name << " " << e._reason);
        return false;
    }

    while (_federateAmbassador->_timeAdvancePending) {
        _ambassador->tick(_tickTimeout, 0);
        _federateAmbassador->processQueues();
    }

    return true;
}

bool
RTI13Federate::queryFederateTime(SGTimeStamp& timeStamp)
{
    if (!_ambassador.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not query federate time.");
        return false;
    }

    try {
        _ambassador->queryFederateTime(timeStamp);
    } catch (RTI::FederateNotExecutionMember& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not query federate time: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::ConcurrentAccessAttempted& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not query federate time: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::SaveInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not query federate time: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::RestoreInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not query federate time: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::RTIinternalError& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not query federate time: " << e._name << " " << e._reason);
        return false;
    }
    return true;
}

bool
RTI13Federate::modifyLookahead(const SGTimeStamp& timeStamp)
{
    if (!_ambassador.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not modify lookahead.");
        return false;
    }
    try {
        _ambassador->modifyLookahead(timeStamp);
    } catch (RTI::InvalidLookahead& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not modify lookahead: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::FederateNotExecutionMember& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not modify lookahead: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::ConcurrentAccessAttempted& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not modify lookahead: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::SaveInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not modify lookahead: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::RestoreInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not modify lookahead: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::RTIinternalError& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not modify lookahead: " << e._name << " " << e._reason);
        return false;
    }
    return true;
}

bool
RTI13Federate::queryLookahead(SGTimeStamp& timeStamp)
{
    if (!_ambassador.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not query lookahead.");
        return false;
    }

    try {
        _ambassador->queryLookahead(timeStamp);
    } catch (RTI::FederateNotExecutionMember& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not query lookahead: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::ConcurrentAccessAttempted& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not query lookahead: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::SaveInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not query lookahead: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::RestoreInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not query lookahead: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::RTIinternalError& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not query lookahead: " << e._name << " " << e._reason);
        return false;
    }
    return true;
}

bool
RTI13Federate::queryGALT(SGTimeStamp& timeStamp)
{
    if (!_ambassador.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not query GALT.");
        return false;
    }

    try {
        return _ambassador->queryGALT(timeStamp);
    } catch (RTI::FederateNotExecutionMember& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not query GALT: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::ConcurrentAccessAttempted& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not query GALT: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::SaveInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not query GALT: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::RestoreInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not query GALT: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::RTIinternalError& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not query GALT: " << e._name << " " << e._reason);
        return false;
    }
    return true;
}

bool
RTI13Federate::queryLITS(SGTimeStamp& timeStamp)
{
    if (!_ambassador.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not query LITS.");
        return false;
    }

    try {
        return _ambassador->queryLITS(timeStamp);
    } catch (RTI::FederateNotExecutionMember& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not query LITS: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::ConcurrentAccessAttempted& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not query LITS: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::SaveInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not query LITS: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::RestoreInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not query LITS: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::RTIinternalError& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not query LITS: " << e._name << " " << e._reason);
        return false;
    }
    return true;
}

bool
RTI13Federate::tick()
{
    bool result = _ambassador->tick();
    _federateAmbassador->processQueues();
    return result;
}

bool
RTI13Federate::tick(const double& minimum, const double& maximum)
{
    bool result = _ambassador->tick(minimum, maximum);
    _federateAmbassador->processQueues();
    return result;
}

RTI13ObjectClass*
RTI13Federate::createObjectClass(const std::string& objectClassName, HLAObjectClass* hlaObjectClass)
{
    try {
        RTI::ObjectClassHandle objectClassHandle;
        objectClassHandle = _ambassador->getObjectClassHandle(objectClassName);
        if (_federateAmbassador->_objectClassMap.find(objectClassHandle) != _federateAmbassador->_objectClassMap.end()) {
            SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not create object class, object class already exists!");
            return 0;
        }
        RTI13ObjectClass* rtiObjectClass;
        rtiObjectClass = new RTI13ObjectClass(hlaObjectClass, objectClassHandle, _ambassador.get());
        _federateAmbassador->_objectClassMap[objectClassHandle] = rtiObjectClass;
        return rtiObjectClass;
    } catch (RTI::NameNotFound& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not get object class: " << e._name << " " << e._reason);
        return 0;
    } catch (RTI::FederateNotExecutionMember& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not get object class: " << e._name << " " << e._reason);
        return 0;
    } catch (RTI::ConcurrentAccessAttempted& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not get object class: " << e._name << " " << e._reason);
        return 0;
    } catch (RTI::RTIinternalError& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not get object class: " << e._name << " " << e._reason);
        return 0;
    }
}

RTI13ObjectInstance*
RTI13Federate::getObjectInstance(const std::string& objectInstanceName)
{
    try {
        RTI::ObjectHandle objectHandle;
        objectHandle = _ambassador->getObjectInstanceHandle(objectInstanceName);
        FederateAmbassador::ObjectInstanceMap::iterator i = _federateAmbassador->_objectInstanceMap.find(objectHandle);
        if (i == _federateAmbassador->_objectInstanceMap.end()) {
            SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not get object class: ObjectInstance not found.");
            return 0;
        }
        return i->second;
    } catch (RTI::ObjectNotKnown& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not get object class: " << e._name << " " << e._reason);
        return 0;
    } catch (RTI::FederateNotExecutionMember& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not get object class: " << e._name << " " << e._reason);
        return 0;
    } catch (RTI::ConcurrentAccessAttempted& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not get object class: " << e._name << " " << e._reason);
        return 0;
    } catch (RTI::RTIinternalError& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not get object class: " << e._name << " " << e._reason);
        return 0;
    }
}

void
RTI13Federate::insertObjectInstance(RTI13ObjectInstance* objectInstance)
{
    _federateAmbassador->_objectInstanceMap[objectInstance->getHandle()] = objectInstance;
}

}
