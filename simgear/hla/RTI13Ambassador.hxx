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

#ifndef RTIAmbassador_hxx
#define RTIAmbassador_hxx

#include <cstdlib>
#include <list>
#include <memory>
#include <vector>
#include <map>
#include <set>

#ifndef RTI_USES_STD_FSTREAM
#define RTI_USES_STD_FSTREAM
#endif

#include <RTI.hh>
#include <fedtime.hh>

#include <simgear/debug/logstream.hxx>
#include <simgear/structure/SGWeakReferenced.hxx>
#include <simgear/structure/SGSharedPtr.hxx>
#include <simgear/structure/SGWeakPtr.hxx>
#include <simgear/timing/timestamp.hxx>

#include "RTIObjectClass.hxx"
#include "RTIData.hxx"
#include "RTI13Federate.hxx"
#include "RTI13ObjectInstance.hxx"

namespace simgear {

class RTI13Ambassador : public SGWeakReferenced {
public:
    RTI13Ambassador() :
        _federateAmbassador(*this),
        _timeRegulationEnabled(false),
        _timeConstrainedEnabled(false),
        _timeAdvancePending(false)
    { }
    virtual ~RTI13Ambassador()
    { }

    // processes the queues that filled up during the past
    void processQueues()
    {
        while (!_queueCallbackList.empty()) {
            (*_queueCallbackList.front())();
            _queueCallbackList.pop_front();
        }

        while (!_objectInstancePendingCallbackList.empty()) {
            (*_objectInstancePendingCallbackList.begin())->flushPendingRequests();
            _objectInstancePendingCallbackList.erase(_objectInstancePendingCallbackList.begin());
        }
    }

    bool getTimeRegulationEnabled() const
    { return _timeRegulationEnabled; }
    bool getTimeConstrainedEnabled() const
    { return _timeConstrainedEnabled; }
    bool getTimeAdvancePending() const
    { return _timeAdvancePending; }
    const SGTimeStamp& getCurrentLogicalTime() const
    { return _federateTime; }

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

    void createFederationExecution(const std::string& name, const std::string& objectModel)
    { _rtiAmbassador.createFederationExecution(name.c_str(), objectModel.c_str()); }
    void destroyFederationExecution(const std::string& name)
    { _rtiAmbassador.destroyFederationExecution(name.c_str()); }

    RTI::FederateHandle joinFederationExecution(const std::string& federate, const std::string& federation)
    { return _rtiAmbassador.joinFederationExecution(federate.c_str(), federation.c_str(), &_federateAmbassador); }
    void resignFederationExecution()
    { _rtiAmbassador.resignFederationExecution(RTI::DELETE_OBJECTS_AND_RELEASE_ATTRIBUTES); }

    void registerFederationSynchronizationPoint(const std::string& label, const RTIData& tag)
    { _rtiAmbassador.registerFederationSynchronizationPoint(label.c_str(), tag.data()); }
    void synchronizationPointAchieved(const std::string& label)
    { _rtiAmbassador.synchronizationPointAchieved(label.c_str()); }

    void publishObjectClass(const RTI::ObjectClassHandle& handle, const RTI::AttributeHandleSet& attributeHandleSet)
    { _rtiAmbassador.publishObjectClass(handle, attributeHandleSet); }
    void unpublishObjectClass(const RTI::ObjectClassHandle& handle)
    { _rtiAmbassador.unpublishObjectClass(handle); }
    void subscribeObjectClassAttributes(const RTI::ObjectClassHandle& handle, const RTI::AttributeHandleSet& attributeHandleSet, bool active)
    { _rtiAmbassador.subscribeObjectClassAttributes(handle, attributeHandleSet, active ? RTI::RTI_TRUE : RTI::RTI_FALSE); }
    void unsubscribeObjectClass(const RTI::ObjectClassHandle& handle)
    { _rtiAmbassador.unsubscribeObjectClass(handle); }

    RTI13ObjectInstance* registerObjectInstance(const RTI13ObjectClass* objectClass, HLAObjectInstance* hlaObjectInstance)
    {
        RTI::ObjectHandle objectHandle = _rtiAmbassador.registerObjectInstance(objectClass->getHandle());
        RTI13ObjectInstance* objectInstance = new RTI13ObjectInstance(objectHandle, hlaObjectInstance, objectClass, this, true);
        _objectInstanceMap[objectHandle] = objectInstance;
        return objectInstance;
    }
    void updateAttributeValues(const RTI::ObjectHandle& objectHandle, const RTI::AttributeHandleValuePairSet& attributeValues,
                               const SGTimeStamp& timeStamp, const RTIData& tag)
    { _rtiAmbassador.updateAttributeValues(objectHandle, attributeValues, toFedTime(timeStamp), tag.data()); }
    void updateAttributeValues(const RTI::ObjectHandle& objectHandle, const RTI::AttributeHandleValuePairSet& attributeValues, const RTIData& tag)
    { _rtiAmbassador.updateAttributeValues(objectHandle, attributeValues, tag.data()); }

    // RTI::EventRetractionHandle sendInteraction(RTI::InteractionClassHandle interactionClassHandle, const RTI::ParameterHandleValuePairSet& parameters, const RTI::FedTime& fedTime, const RTIData& tag)
    // { return _rtiAmbassador.sendInteraction(interactionClassHandle, parameters, fedTime, tag.data()); }
    // void sendInteraction(RTI::InteractionClassHandle interactionClassHandle, const RTI::ParameterHandleValuePairSet& parameters, const RTIData& tag)
    // { _rtiAmbassador.sendInteraction(interactionClassHandle, parameters, tag.data()); }

    void deleteObjectInstance(const RTI::ObjectHandle& objectHandle, const SGTimeStamp& timeStamp, const RTIData& tag)
    {
        RTI::EventRetractionHandle h = _rtiAmbassador.deleteObjectInstance(objectHandle, toFedTime(timeStamp), tag.data());
        ObjectInstanceMap::iterator i = _objectInstanceMap.find(objectHandle);
        if (i == _objectInstanceMap.end()) {
            SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not get object class: ObjectInstance not found.");
            return;
        }
        _objectInstancePendingCallbackList.erase(i->second);
        _objectInstanceMap.erase(i);
    }
    void deleteObjectInstance(const RTI::ObjectHandle& objectHandle, const RTIData& tag)
    {
        _rtiAmbassador.deleteObjectInstance(objectHandle, tag.data());
        ObjectInstanceMap::iterator i = _objectInstanceMap.find(objectHandle);
        if (i == _objectInstanceMap.end()) {
            SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not get object class: ObjectInstance not found.");
            return;
        }
        _objectInstancePendingCallbackList.erase(i->second);
        _objectInstanceMap.erase(i);
    }
    void localDeleteObjectInstance(const RTI::ObjectHandle& objectHandle)
    {
        _rtiAmbassador.localDeleteObjectInstance(objectHandle);
        ObjectInstanceMap::iterator i = _objectInstanceMap.find(objectHandle);
        if (i == _objectInstanceMap.end()) {
            SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not get object class: ObjectInstance not found.");
            return;
        }
        _objectInstancePendingCallbackList.erase(i->second);
        _objectInstanceMap.erase(i);
    }

    void requestObjectAttributeValueUpdate(const RTI::ObjectHandle& handle, const RTI::AttributeHandleSet& attributeHandleSet)
    { _rtiAmbassador.requestObjectAttributeValueUpdate(handle, attributeHandleSet); }
    void requestClassAttributeValueUpdate(const RTI::ObjectClassHandle& handle, const RTI::AttributeHandleSet& attributeHandleSet)
    { _rtiAmbassador.requestClassAttributeValueUpdate(handle, attributeHandleSet); }

    // Ownership Management -------------------

    // bool unconditionalAttributeOwnershipDivestiture(const RTIHandle& objectHandle, const RTIHandleSet& attributeHandles)
    // {
    //     try {
    //         std::auto_ptr<RTI::AttributeHandleSet> attributeHandleSet(RTI::AttributeHandleSetFactory::create(attributeHandles.size()));
    //         for (RTIHandleSet::const_iterator i = attributeHandles.begin(); i != attributeHandles.end(); ++i)
    //             attributeHandleSet->add(*i);
    //         _rtiAmbassador.unconditionalAttributeOwnershipDivestiture(objectHandle, *attributeHandleSet);
    //         return true;
    //     } catch (RTI::ObjectNotKnown& e) {
    //     } catch (RTI::AttributeNotDefined& e) {
    //     } catch (RTI::AttributeNotOwned& e) {
    //     } catch (RTI::FederateNotExecutionMember& e) {
    //     } catch (RTI::ConcurrentAccessAttempted& e) {
    //     } catch (RTI::SaveInProgress& e) {
    //     } catch (RTI::RestoreInProgress& e) {
    //     } catch (RTI::RTIinternalError& e) {
    //     }
    //     return false;
    // }
    // bool negotiatedAttributeOwnershipDivestiture(const RTIHandle& objectHandle, const RTIHandleSet& attributeHandles, const RTIData& tag)
    // {
    //     try {
    //         std::auto_ptr<RTI::AttributeHandleSet> attributeHandleSet(RTI::AttributeHandleSetFactory::create(attributeHandles.size()));
    //         for (RTIHandleSet::const_iterator i = attributeHandles.begin(); i != attributeHandles.end(); ++i)
    //             attributeHandleSet->add(*i);
    //         _rtiAmbassador.negotiatedAttributeOwnershipDivestiture(objectHandle, *attributeHandleSet, tag.data());
    //         return true;
    //     } catch (RTI::ObjectNotKnown& e) {
    //     } catch (RTI::AttributeNotDefined& e) {
    //     } catch (RTI::AttributeNotOwned& e) {
    //     } catch (RTI::AttributeAlreadyBeingDivested& e) {
    //     } catch (RTI::FederateNotExecutionMember& e) {
    //     } catch (RTI::ConcurrentAccessAttempted& e) {
    //     } catch (RTI::SaveInProgress& e) {
    //     } catch (RTI::RestoreInProgress& e) {
    //     } catch (RTI::RTIinternalError& e) {
    //     }
    //     return false;
    // }
    // bool attributeOwnershipAcquisition(const RTIHandle& objectHandle, const RTIHandleSet& attributeHandles, const RTIData& tag)
    // {
    //     try {
    //         std::auto_ptr<RTI::AttributeHandleSet> attributeHandleSet(RTI::AttributeHandleSetFactory::create(attributeHandles.size()));
    //         for (RTIHandleSet::const_iterator i = attributeHandles.begin(); i != attributeHandles.end(); ++i)
    //             attributeHandleSet->add(*i);
    //         _rtiAmbassador.attributeOwnershipAcquisition(objectHandle, *attributeHandleSet, tag.data());
    //         return true;
    //     } catch (RTI::ObjectNotKnown& e) {
    //     } catch (RTI::ObjectClassNotPublished& e) {
    //     } catch (RTI::AttributeNotDefined& e) {
    //     } catch (RTI::AttributeNotPublished& e) {
    //     } catch (RTI::FederateOwnsAttributes& e) {
    //     } catch (RTI::FederateNotExecutionMember& e) {
    //     } catch (RTI::ConcurrentAccessAttempted& e) {
    //     } catch (RTI::SaveInProgress& e) {
    //     } catch (RTI::RestoreInProgress& e) {
    //     } catch (RTI::RTIinternalError& e) {
    //     }
    //     return false;
    // }
    // bool attributeOwnershipAcquisitionIfAvailable(const RTIHandle& objectHandle, const RTIHandleSet& attributeHandles)
    // {
    //     try {
    //         std::auto_ptr<RTI::AttributeHandleSet> attributeHandleSet(RTI::AttributeHandleSetFactory::create(attributeHandles.size()));
    //         for (RTIHandleSet::const_iterator i = attributeHandles.begin(); i != attributeHandles.end(); ++i)
    //             attributeHandleSet->add(*i);
    //         _rtiAmbassador.attributeOwnershipAcquisitionIfAvailable(objectHandle, *attributeHandleSet);
    //         return true;
    //     } catch (RTI::ObjectNotKnown& e) {
    //     } catch (RTI::ObjectClassNotPublished& e) {
    //     } catch (RTI::AttributeNotDefined& e) {
    //     } catch (RTI::AttributeNotPublished& e) {
    //     } catch (RTI::FederateOwnsAttributes& e) {
    //     } catch (RTI::AttributeAlreadyBeingAcquired& e) {
    //     } catch (RTI::FederateNotExecutionMember& e) {
    //     } catch (RTI::ConcurrentAccessAttempted& e) {
    //     } catch (RTI::SaveInProgress& e) {
    //     } catch (RTI::RestoreInProgress& e) {
    //     } catch (RTI::RTIinternalError& e) {
    //     }
    //     return false;
    // }
    // RTIHandleSet attributeOwnershipReleaseResponse(const RTIHandle& objectHandle, const RTIHandleSet& attributeHandles)
    // {
    //     try {
    //         std::auto_ptr<RTI::AttributeHandleSet> attributeHandleSet(RTI::AttributeHandleSetFactory::create(attributeHandles.size()));
    //         for (RTIHandleSet::const_iterator i = attributeHandles.begin(); i != attributeHandles.end(); ++i)
    //             attributeHandleSet->add(*i);
    //         attributeHandleSet.reset(_rtiAmbassador.attributeOwnershipReleaseResponse(objectHandle, *attributeHandleSet));
    //         RTIHandleSet handleSet;
    //         RTI::ULong numAttribs = attributeHandleSet->size();
    //         for (RTI::ULong i = 0; i < numAttribs; ++i)
    //             handleSet.insert(attributeHandleSet->getHandle(i));
    //         return handleSet;
    //     } catch (RTI::ObjectNotKnown& e) {
    //     } catch (RTI::AttributeNotDefined& e) {
    //     } catch (RTI::AttributeNotOwned& e) {
    //     } catch (RTI::FederateWasNotAskedToReleaseAttribute& e) {
    //     } catch (RTI::FederateNotExecutionMember& e) {
    //     } catch (RTI::ConcurrentAccessAttempted& e) {
    //     } catch (RTI::SaveInProgress& e) {
    //     } catch (RTI::RestoreInProgress& e) {
    //     } catch (RTI::RTIinternalError& e) {
    //     }
    //     return RTIHandleSet();
    // }
    // bool cancelNegotiatedAttributeOwnershipDivestiture(const RTIHandle& objectHandle, const RTIHandleSet& attributeHandles)
    // {
    //     try {
    //         std::auto_ptr<RTI::AttributeHandleSet> attributeHandleSet(RTI::AttributeHandleSetFactory::create(attributeHandles.size()));
    //         for (RTIHandleSet::const_iterator i = attributeHandles.begin(); i != attributeHandles.end(); ++i)
    //             attributeHandleSet->add(*i);
    //         _rtiAmbassador.cancelNegotiatedAttributeOwnershipDivestiture(objectHandle, *attributeHandleSet);
    //         return true;
    //     } catch (RTI::ObjectNotKnown& e) {
    //     } catch (RTI::AttributeNotDefined& e) {
    //     } catch (RTI::AttributeNotOwned& e) {
    //     } catch (RTI::AttributeDivestitureWasNotRequested& e) {
    //     } catch (RTI::FederateNotExecutionMember& e) {
    //     } catch (RTI::ConcurrentAccessAttempted& e) {
    //     } catch (RTI::SaveInProgress& e) {
    //     } catch (RTI::RestoreInProgress& e) {
    //     } catch (RTI::RTIinternalError& e) {
    //     }
    //     return false;
    // }
    // bool cancelAttributeOwnershipAcquisition(const RTIHandle& objectHandle, const RTIHandleSet& attributeHandles)
    // {
    //     try {
    //         std::auto_ptr<RTI::AttributeHandleSet> attributeHandleSet(RTI::AttributeHandleSetFactory::create(attributeHandles.size()));
    //         for (RTIHandleSet::const_iterator i = attributeHandles.begin(); i != attributeHandles.end(); ++i)
    //             attributeHandleSet->add(*i);
    //         _rtiAmbassador.cancelAttributeOwnershipAcquisition(objectHandle, *attributeHandleSet);
    //         return true;
    //     } catch (RTI::ObjectNotKnown& e) {
    //     } catch (RTI::AttributeNotDefined& e) {
    //     } catch (RTI::AttributeAlreadyOwned& e) {
    //     } catch (RTI::AttributeAcquisitionWasNotRequested& e) {
    //     } catch (RTI::FederateNotExecutionMember& e) {
    //     } catch (RTI::ConcurrentAccessAttempted& e) {
    //     } catch (RTI::SaveInProgress& e) {
    //     } catch (RTI::RestoreInProgress& e) {
    //     } catch (RTI::RTIinternalError& e) {
    //     }
    //     return false;
    // }
    // bool queryAttributeOwnership(const RTIHandle& objectHandle, const RTIHandle& attributeHandle)
    // {
    //     try {
    //         _rtiAmbassador.queryAttributeOwnership(objectHandle, attributeHandle);
    //         return true;
    //     } catch (RTI::ObjectNotKnown& e) {
    //     } catch (RTI::AttributeNotDefined& e) {
    //     } catch (RTI::FederateNotExecutionMember& e) {
    //     } catch (RTI::ConcurrentAccessAttempted& e) {
    //     } catch (RTI::SaveInProgress& e) {
    //     } catch (RTI::RestoreInProgress& e) {
    //     } catch (RTI::RTIinternalError& e) {
    //     }
    //     return false;
    // }
    // bool isAttributeOwnedByFederate(const RTIHandle& objectHandle, const RTIHandle& attributeHandle)
    // {
    //     try {
    //         return _rtiAmbassador.isAttributeOwnedByFederate(objectHandle, attributeHandle);
    //     } catch (RTI::ObjectNotKnown& e) {
    //     } catch (RTI::AttributeNotDefined& e) {
    //     } catch (RTI::FederateNotExecutionMember& e) {
    //     } catch (RTI::ConcurrentAccessAttempted& e) {
    //     } catch (RTI::SaveInProgress& e) {
    //     } catch (RTI::RestoreInProgress& e) {
    //     } catch (RTI::RTIinternalError& e) {
    //     }
    //     return false;
    // }

    /// Time Management

    void enableTimeRegulation(const SGTimeStamp& federateTime, const SGTimeStamp& lookahead)
    { _rtiAmbassador.enableTimeRegulation(toFedTime(federateTime), toFedTime(lookahead)); }
    void disableTimeRegulation()
    { _rtiAmbassador.disableTimeRegulation(); _timeRegulationEnabled = false; }

    void enableTimeConstrained()
    { _rtiAmbassador.enableTimeConstrained(); }
    void disableTimeConstrained()
    { _rtiAmbassador.disableTimeConstrained(); _timeConstrainedEnabled = false; }

    void timeAdvanceRequest(const SGTimeStamp& time)
    { _rtiAmbassador.timeAdvanceRequest(toFedTime(time)); _timeAdvancePending = true; }
    void timeAdvanceRequestAvailable(const SGTimeStamp& time)
    { _rtiAmbassador.timeAdvanceRequestAvailable(toFedTime(time)); _timeAdvancePending = true; }

    // bool queryLBTS(double& time)
    // {
    //     try {
    //         RTIfedTime fedTime;
    //         _rtiAmbassador.queryLBTS(fedTime);
    //         time = fedTime.getTime();
    //         return true;
    //     } catch (RTI::FederateNotExecutionMember& e) {
    //     } catch (RTI::ConcurrentAccessAttempted& e) {
    //     } catch (RTI::SaveInProgress& e) {
    //     } catch (RTI::RestoreInProgress& e) {
    //     } catch (RTI::RTIinternalError& e) {
    //     }
    //     return false;
    // }
    void queryFederateTime(SGTimeStamp& timeStamp)
    {
        RTIfedTime fedTime;
        _rtiAmbassador.queryFederateTime(fedTime);
        timeStamp = toTimeStamp(fedTime);
    }
    void queryLookahead(SGTimeStamp& timeStamp)
    {
        RTIfedTime fedTime;
        _rtiAmbassador.queryLookahead(fedTime);
        timeStamp = toTimeStamp(fedTime);
    }

    RTI13ObjectClass* createObjectClass(const std::string& name, HLAObjectClass* hlaObjectClass)
    {
        RTI::ObjectClassHandle objectClassHandle;
        objectClassHandle = getObjectClassHandle(name);
        if (_objectClassMap.find(objectClassHandle) != _objectClassMap.end()) {
            SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not create object class, object class already exists!");
            return 0;
        }
        RTI13ObjectClass* rtiObjectClass;
        rtiObjectClass = new RTI13ObjectClass(hlaObjectClass, objectClassHandle, this);
        _objectClassMap[objectClassHandle] = rtiObjectClass;
        return rtiObjectClass;
    }
    RTI::ObjectClassHandle getObjectClassHandle(const std::string& name)
    { return _rtiAmbassador.getObjectClassHandle(name.c_str()); }
    std::string getObjectClassName(const RTI::ObjectClassHandle& handle)
    { return rtiToStdString(_rtiAmbassador.getObjectClassName(handle)); }

    RTI::AttributeHandle getAttributeHandle(const std::string& attributeName, const RTI::ObjectClassHandle& objectClassHandle)
    { return _rtiAmbassador.getAttributeHandle(attributeName.c_str(), objectClassHandle); }
    std::string getAttributeName(const RTI::AttributeHandle& attributeHandle, const RTI::ObjectClassHandle& objectClassHandle)
    { return rtiToStdString(_rtiAmbassador.getAttributeName(attributeHandle, objectClassHandle)); }

    // RTIHandle getInteractionClassHandle(const std::string& name)
    // {
    //     try {
    //         return _rtiAmbassador.getInteractionClassHandle(name.c_str());
    //     } catch (RTI::NameNotFound& e) {
    //     } catch (RTI::FederateNotExecutionMember& e) {
    //     } catch (RTI::ConcurrentAccessAttempted& e) {
    //     } catch (RTI::RTIinternalError& e) {
    //     }
    //     return RTIHandle(-1);
    // }
    // std::string getInteractionClassName(const RTIHandle& handle)
    // {
    //     std::string name;
    //     try {
    //         rtiToStdString(name, _rtiAmbassador.getInteractionClassName(handle));
    //     } catch (RTI::InteractionClassNotDefined& e) {
    //     } catch (RTI::FederateNotExecutionMember& e) {
    //     } catch (RTI::ConcurrentAccessAttempted& e) {
    //     } catch (RTI::RTIinternalError& e) {
    //     }
    //     return name;
    // }

    // RTIHandle getParameterHandle(const std::string& parameterName, const RTIHandle& interactionClassHandle)
    // {
    //     try {
    //         return _rtiAmbassador.getParameterHandle(parameterName.c_str(), interactionClassHandle);
    //     } catch (RTI::InteractionClassNotDefined& e) {
    //     } catch (RTI::NameNotFound& e) {
    //     } catch (RTI::FederateNotExecutionMember& e) {
    //     } catch (RTI::ConcurrentAccessAttempted& e) {
    //     } catch (RTI::RTIinternalError& e) {
    //     }
    //     return RTIHandle(-1);
    // }
    // std::string getParameterName(const RTIHandle& parameterHandle, const RTIHandle& interactionClassHandle)
    // {
    //     std::string parameterName;
    //     try {
    //         rtiToStdString(parameterName, _rtiAmbassador.getParameterName(parameterHandle, interactionClassHandle));
    //     } catch (RTI::InteractionClassNotDefined& e) {
    //     } catch (RTI::InteractionParameterNotDefined& e) {
    //     } catch (RTI::FederateNotExecutionMember& e) {
    //     } catch (RTI::ConcurrentAccessAttempted& e) {
    //     } catch (RTI::RTIinternalError& e) {
    //     }
    //     return parameterName;
    // }

    RTI13ObjectInstance* getObjectInstance(const std::string& name)
    {
        RTI::ObjectHandle objectHandle;
        objectHandle = getObjectInstanceHandle(name);
        ObjectInstanceMap::iterator i = _objectInstanceMap.find(objectHandle);
        if (i == _objectInstanceMap.end()) {
            SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not get object class: ObjectInstance not found.");
            return 0;
        }
        return i->second;
    }
    RTI::ObjectHandle getObjectInstanceHandle(const std::string& name)
    { return _rtiAmbassador.getObjectInstanceHandle(name.c_str()); }
    std::string getObjectInstanceName(const RTI::ObjectHandle& objectHandle)
    { return rtiToStdString(_rtiAmbassador.getObjectInstanceName(objectHandle)); }

    // RTIHandle getRoutingSpaceHandle(const std::string& routingSpaceName)
    // {
    //     try {
    //         return _rtiAmbassador.getRoutingSpaceHandle(routingSpaceName.c_str());
    //     } catch (RTI::NameNotFound& e) {
    //     } catch (RTI::FederateNotExecutionMember& e) {
    //     } catch (RTI::ConcurrentAccessAttempted& e) {
    //     } catch (RTI::RTIinternalError& e) {
    //     }
    //     return RTIHandle(-1);
    // }
    // std::string getRoutingSpaceName(const RTIHandle& routingSpaceHandle)
    // {
    //     std::string routingSpaceName;
    //     try {
    //         rtiToStdString(routingSpaceName, _rtiAmbassador.wgetRoutingSpaceName(routingSpaceHandle));
    //     } catch (RTI::SpaceNotDefined& e) {
    //     } catch (RTI::FederateNotExecutionMember& e) {
    //     } catch (RTI::ConcurrentAccessAttempted& e) {
    //     } catch (RTI::RTIinternalError& e) {
    //     }
    //     return routingSpaceName;
    // }

    void enableClassRelevanceAdvisorySwitch()
    { _rtiAmbassador.enableClassRelevanceAdvisorySwitch(); }
    void disableClassRelevanceAdvisorySwitch()
    { _rtiAmbassador.disableClassRelevanceAdvisorySwitch(); }

    void enableAttributeRelevanceAdvisorySwitch()
    { _rtiAmbassador.enableAttributeRelevanceAdvisorySwitch(); }
    void disableAttributeRelevanceAdvisorySwitch()
    { _rtiAmbassador.disableAttributeRelevanceAdvisorySwitch(); }

    void enableAttributeScopeAdvisorySwitch()
    { _rtiAmbassador.enableAttributeScopeAdvisorySwitch(); }
    void disableAttributeScopeAdvisorySwitch()
    { _rtiAmbassador.disableAttributeScopeAdvisorySwitch(); }

    void enableInteractionRelevanceAdvisorySwitch()
    { _rtiAmbassador.enableInteractionRelevanceAdvisorySwitch(); }
    void disableInteractionRelevanceAdvisorySwitch()
    { _rtiAmbassador.disableInteractionRelevanceAdvisorySwitch(); }


    bool tick()
    { return _rtiAmbassador.tick(); }
    bool tick(double minimum, double maximum)
    { return _rtiAmbassador.tick(minimum, maximum); }

    void addObjectInstanceForCallback(RTIObjectInstance* objectIntance)
    { _objectInstancePendingCallbackList.insert(objectIntance); }

private:
    /// Generic callback to execute some notification on objects in a way that they are not prone to
    /// ConcurrentAccess exceptions.
    class QueueCallback : public SGReferenced {
    public:
        virtual ~QueueCallback() {}
        virtual void operator()() = 0;
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

    /// Just the interface class doing the callbacks into the parent class
    struct FederateAmbassador : public RTI::FederateAmbassador {
        FederateAmbassador(RTI13Ambassador& rtiAmbassador) :
            _rtiAmbassador(rtiAmbassador)
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
            _rtiAmbassador._pendingSyncLabels.insert(toStdString(label));
        }

        virtual void federationSynchronized(const char* label)
            throw (RTI::FederateInternalError)
        {
            std::string s = toStdString(label);
            _rtiAmbassador._pendingSyncLabels.erase(s);
            _rtiAmbassador._syncronizedSyncLabels.insert(s);
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
            ObjectClassMap::iterator i = _rtiAmbassador._objectClassMap.find(objectClassHandle);
            if (i == _rtiAmbassador._objectClassMap.end())
                return;
            if (!i->second.valid())
                return;
            i->second->startRegistration();
        }

        virtual void stopRegistrationForObjectClass(RTI::ObjectClassHandle objectClassHandle)
            throw (RTI::ObjectClassNotPublished,
                   RTI::FederateInternalError)
        {
            ObjectClassMap::iterator i = _rtiAmbassador._objectClassMap.find(objectClassHandle);
            if (i == _rtiAmbassador._objectClassMap.end())
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
            ObjectClassMap::iterator i = _rtiAmbassador._objectClassMap.find(objectClassHandle);
            if (i == _rtiAmbassador._objectClassMap.end())
                throw RTI::ObjectClassNotKnown("Federate: discoverObjectInstance()!");
            if (!i->second.valid())
                return;
            SGSharedPtr<RTI13ObjectInstance> objectInstance = new RTI13ObjectInstance(objectHandle, 0, i->second, &_rtiAmbassador, false);
            _rtiAmbassador._objectInstanceMap[objectHandle] = objectInstance;
            _rtiAmbassador._objectInstancePendingCallbackList.insert(objectInstance);
            i->second->discoverInstance(objectInstance.get(), tagToData(tag));
        }

        virtual void reflectAttributeValues(RTI::ObjectHandle objectHandle, const RTI::AttributeHandleValuePairSet& attributeValuePairSet,
                                            const RTI::FedTime& fedTime, const char* tag, RTI::EventRetractionHandle eventRetractionHandle)
            throw (RTI::ObjectNotKnown,
                   RTI::AttributeNotKnown,
                   RTI::FederateOwnsAttributes,
                   RTI::InvalidFederationTime,
                   RTI::FederateInternalError)
        {
            ObjectInstanceMap::iterator i = _rtiAmbassador._objectInstanceMap.find(objectHandle);
            if (i == _rtiAmbassador._objectInstanceMap.end())
                throw RTI::ObjectNotKnown("Reflect attributes for unknown object!");
            if (!i->second.valid())
                return;
            i->second->reflectAttributeValues(attributeValuePairSet, toTimeStamp(fedTime), tagToData(tag));
        }

        virtual void reflectAttributeValues(RTI::ObjectHandle objectHandle, const RTI::AttributeHandleValuePairSet& attributeValuePairSet,
                                            const char* tag)
            throw (RTI::ObjectNotKnown,
                   RTI::AttributeNotKnown,
                   RTI::FederateOwnsAttributes,
                   RTI::FederateInternalError)
        {
            ObjectInstanceMap::iterator i = _rtiAmbassador._objectInstanceMap.find(objectHandle);
            if (i == _rtiAmbassador._objectInstanceMap.end())
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
            ObjectInstanceMap::iterator i = _rtiAmbassador._objectInstanceMap.find(objectHandle);
            if (i == _rtiAmbassador._objectInstanceMap.end())
                throw RTI::ObjectNotKnown("Federate: removeObjectInstance()!");
            if (i->second.valid())
                _rtiAmbassador._queueCallbackList.push_back(new RemoveObjectCallback(i->second, tagToData(tag)));
            _rtiAmbassador._objectInstancePendingCallbackList.erase(i->second);
            _rtiAmbassador._objectInstanceMap.erase(i);
        }

        virtual void removeObjectInstance(RTI::ObjectHandle objectHandle, const char* tag)
            throw (RTI::ObjectNotKnown,
                   RTI::FederateInternalError)
        {
            ObjectInstanceMap::iterator i = _rtiAmbassador._objectInstanceMap.find(objectHandle);
            if (i == _rtiAmbassador._objectInstanceMap.end())
                throw RTI::ObjectNotKnown("Federate: removeObjectInstance()!");
            if (i->second.valid())
                _rtiAmbassador._queueCallbackList.push_back(new RemoveObjectCallback(i->second, tagToData(tag)));
            _rtiAmbassador._objectInstancePendingCallbackList.erase(i->second);
            _rtiAmbassador._objectInstanceMap.erase(i);
        }

        virtual void attributesInScope(RTI::ObjectHandle objectHandle, const RTI::AttributeHandleSet& attributes)
            throw (RTI::ObjectNotKnown,
                   RTI::AttributeNotKnown,
                   RTI::FederateInternalError)
        {
            ObjectInstanceMap::iterator i = _rtiAmbassador._objectInstanceMap.find(objectHandle);
            if (i == _rtiAmbassador._objectInstanceMap.end())
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
            ObjectInstanceMap::iterator i = _rtiAmbassador._objectInstanceMap.find(objectHandle);
            if (i == _rtiAmbassador._objectInstanceMap.end())
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
            ObjectInstanceMap::iterator i = _rtiAmbassador._objectInstanceMap.find(objectHandle);
            if (i == _rtiAmbassador._objectInstanceMap.end())
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
            ObjectInstanceMap::iterator i = _rtiAmbassador._objectInstanceMap.find(objectHandle);
            if (i == _rtiAmbassador._objectInstanceMap.end())
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
            ObjectInstanceMap::iterator i = _rtiAmbassador._objectInstanceMap.find(objectHandle);
            if (i == _rtiAmbassador._objectInstanceMap.end())
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
            ObjectInstanceMap::iterator i = _rtiAmbassador._objectInstanceMap.find(objectHandle);
            if (i == _rtiAmbassador._objectInstanceMap.end())
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
            ObjectInstanceMap::iterator i = _rtiAmbassador._objectInstanceMap.find(objectHandle);
            if (i == _rtiAmbassador._objectInstanceMap.end())
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
            ObjectInstanceMap::iterator i = _rtiAmbassador._objectInstanceMap.find(objectHandle);
            if (i == _rtiAmbassador._objectInstanceMap.end())
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
            ObjectInstanceMap::iterator i = _rtiAmbassador._objectInstanceMap.find(objectHandle);
            if (i == _rtiAmbassador._objectInstanceMap.end())
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
            ObjectInstanceMap::iterator i = _rtiAmbassador._objectInstanceMap.find(objectHandle);
            if (i == _rtiAmbassador._objectInstanceMap.end())
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
            ObjectInstanceMap::iterator i = _rtiAmbassador._objectInstanceMap.find(objectHandle);
            if (i == _rtiAmbassador._objectInstanceMap.end())
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
            ObjectInstanceMap::iterator i = _rtiAmbassador._objectInstanceMap.find(objectHandle);
            if (i == _rtiAmbassador._objectInstanceMap.end())
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
            ObjectInstanceMap::iterator i = _rtiAmbassador._objectInstanceMap.find(objectHandle);
            if (i == _rtiAmbassador._objectInstanceMap.end())
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
            ObjectInstanceMap::iterator i = _rtiAmbassador._objectInstanceMap.find(objectHandle);
            if (i == _rtiAmbassador._objectInstanceMap.end())
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
            _rtiAmbassador._timeRegulationEnabled = true;
            _rtiAmbassador._federateTime = toTimeStamp(fedTime);
            SG_LOG(SG_NETWORK, SG_INFO, "RTI: timeRegulationEnabled: " << _rtiAmbassador._federateTime);
        }

        virtual void timeConstrainedEnabled(const RTI::FedTime& fedTime)
            throw (RTI::InvalidFederationTime,
                   RTI::EnableTimeConstrainedWasNotPending,
                   RTI::FederateInternalError)
        {
            _rtiAmbassador._timeConstrainedEnabled = true;
            _rtiAmbassador._federateTime = toTimeStamp(fedTime);
            SG_LOG(SG_NETWORK, SG_INFO, "RTI: timeConstrainedEnabled: " << _rtiAmbassador._federateTime);
        }

        virtual void timeAdvanceGrant(const RTI::FedTime& fedTime)
            throw (RTI::InvalidFederationTime,
                   RTI::TimeAdvanceWasNotInProgress,
                   RTI::FederateInternalError)
        {
            _rtiAmbassador._federateTime = toTimeStamp(fedTime);
            _rtiAmbassador._timeAdvancePending = false;
            SG_LOG(SG_NETWORK, SG_INFO, "RTI: timeAdvanceGrant: " << _rtiAmbassador._federateTime);
        }

        virtual void requestRetraction(RTI::EventRetractionHandle eventRetractionHandle)
            throw (RTI::EventNotKnown,
                   RTI::FederateInternalError)
        {
            // No retraction concept yet
        }

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

        RTI13Ambassador& _rtiAmbassador;
    };

    static SGTimeStamp toTimeStamp(const RTI::FedTime& fedTime)
    {
        RTIfedTime referenceTime(fedTime);
        return SGTimeStamp::fromSec(referenceTime.getTime() + 0.5e-9);
    }

    static RTIfedTime toFedTime(const SGTimeStamp& timeStamp)
    {
        RTIfedTime referenceTime;
        referenceTime.setZero();
        referenceTime += timeStamp.toSecs();
        return referenceTime;
    }

    static std::string rtiToStdString(char* n)
    {
        if (!n)
            return std::string();
        std::string s;
        s.assign(n);
        delete[] n;
        return s;
    }

    static std::string toStdString(const char* n)
    {
        if (!n)
            return std::string();
        return std::string(n);
    }

    // The connection class
    RTI::RTIambassador _rtiAmbassador;

    // The class with all the callbacks.
    FederateAmbassador _federateAmbassador;

    // All the sync labels we got an announcement for
    std::set<std::string> _pendingSyncLabels;
    std::set<std::string> _syncronizedSyncLabels;

    // All that calls back into user code is just queued.
    // That is to make sure we do not call recursively into the RTI
    typedef std::list<SGSharedPtr<QueueCallback> > QueueCallbackList;
    QueueCallbackList _queueCallbackList;
    // All object instances that need to be called due to some event are noted here
    // That is to make sure we do not call recursively into the RTI
    typedef std::set<SGSharedPtr<RTIObjectInstance> > ObjectInstanceSet;
    ObjectInstanceSet _objectInstancePendingCallbackList;

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
};

} // namespace simgear

#endif
