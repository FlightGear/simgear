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

#ifndef RTI13Ambassador_hxx
#define RTI13Ambassador_hxx

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

class RTI13Federate;

class RTI13Ambassador : public SGReferenced {
public:
    ~RTI13Ambassador()
    { }

    void createFederationExecution(const std::string& name, const std::string& objectModel)
    { _rtiAmbassador.createFederationExecution(name.c_str(), objectModel.c_str()); }
    void destroyFederationExecution(const std::string& name)
    { _rtiAmbassador.destroyFederationExecution(name.c_str()); }

    RTI::FederateHandle joinFederationExecution(const std::string& federate, const std::string& federation, RTI::FederateAmbassador* federateAmbassador)
    { return _rtiAmbassador.joinFederationExecution(federate.c_str(), federation.c_str(), federateAmbassador); }
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

    void publishInteractionClass(const RTI::InteractionClassHandle& handle)
    { _rtiAmbassador.publishInteractionClass(handle); }
    void unpublishInteractionClass(const RTI::InteractionClassHandle& handle)
    { _rtiAmbassador.unpublishInteractionClass(handle); }

    void subscribeObjectClassAttributes(const RTI::ObjectClassHandle& handle, const RTI::AttributeHandleSet& attributeHandleSet, bool active)
    { _rtiAmbassador.subscribeObjectClassAttributes(handle, attributeHandleSet, active ? RTI::RTI_TRUE : RTI::RTI_FALSE); }
    void unsubscribeObjectClass(const RTI::ObjectClassHandle& handle)
    { _rtiAmbassador.unsubscribeObjectClass(handle); }

    void subscribeInteractionClass(const RTI::InteractionClassHandle& handle, bool active)
    { _rtiAmbassador.subscribeInteractionClass(handle, active ? RTI::RTI_TRUE : RTI::RTI_FALSE); }
    void unsubscribeInteractionClass(const RTI::InteractionClassHandle& handle)
    { _rtiAmbassador.unsubscribeInteractionClass(handle); }

    RTI::ObjectHandle registerObjectInstance(const RTI::ObjectClassHandle& handle)
    { return _rtiAmbassador.registerObjectInstance(handle); }
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
    { /* RTI::EventRetractionHandle h = */ _rtiAmbassador.deleteObjectInstance(objectHandle, toFedTime(timeStamp), tag.data()); }
    void deleteObjectInstance(const RTI::ObjectHandle& objectHandle, const RTIData& tag)
    { _rtiAmbassador.deleteObjectInstance(objectHandle, tag.data()); }
    void localDeleteObjectInstance(const RTI::ObjectHandle& objectHandle)
    { _rtiAmbassador.localDeleteObjectInstance(objectHandle); }

    void requestObjectAttributeValueUpdate(const RTI::ObjectHandle& handle, const RTI::AttributeHandleSet& attributeHandleSet)
    { _rtiAmbassador.requestObjectAttributeValueUpdate(handle, attributeHandleSet); }
    void requestClassAttributeValueUpdate(const RTI::ObjectClassHandle& handle, const RTI::AttributeHandleSet& attributeHandleSet)
    { _rtiAmbassador.requestClassAttributeValueUpdate(handle, attributeHandleSet); }

    // Ownership Management -------------------

    // bool unconditionalAttributeOwnershipDivestiture(const RTIHandle& objectHandle, const RTIHandleSet& attributeHandles)
    // {
    //     try {
    //         std::unique_ptr<RTI::AttributeHandleSet> attributeHandleSet(RTI::AttributeHandleSetFactory::create(attributeHandles.size()));
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
    //         std::unique_ptr<RTI::AttributeHandleSet> attributeHandleSet(RTI::AttributeHandleSetFactory::create(attributeHandles.size()));
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
    //         std::unique_ptr<RTI::AttributeHandleSet> attributeHandleSet(RTI::AttributeHandleSetFactory::create(attributeHandles.size()));
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
    //         std::unique_ptr<RTI::AttributeHandleSet> attributeHandleSet(RTI::AttributeHandleSetFactory::create(attributeHandles.size()));
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
    //         std::unique_ptr<RTI::AttributeHandleSet> attributeHandleSet(RTI::AttributeHandleSetFactory::create(attributeHandles.size()));
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
    //         std::unique_ptr<RTI::AttributeHandleSet> attributeHandleSet(RTI::AttributeHandleSetFactory::create(attributeHandles.size()));
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
    //         std::unique_ptr<RTI::AttributeHandleSet> attributeHandleSet(RTI::AttributeHandleSetFactory::create(attributeHandles.size()));
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
    bool isAttributeOwnedByFederate(const RTI::ObjectHandle& objectHandle, const RTI::AttributeHandle& attributeHandle)
    { return _rtiAmbassador.isAttributeOwnedByFederate(objectHandle, attributeHandle); }

    /// Time Management

    void enableTimeRegulation(const SGTimeStamp& lookahead)
    {
        RTIfedTime federateTime;
        federateTime.setZero();
        _rtiAmbassador.enableTimeRegulation(federateTime, toFedTime(lookahead));
    }
    void disableTimeRegulation()
    { _rtiAmbassador.disableTimeRegulation();}

    void enableTimeConstrained()
    { _rtiAmbassador.enableTimeConstrained(); }
    void disableTimeConstrained()
    { _rtiAmbassador.disableTimeConstrained(); }

    void timeAdvanceRequest(const SGTimeStamp& time)
    { _rtiAmbassador.timeAdvanceRequest(toFedTime(time)); }
    void timeAdvanceRequestAvailable(const SGTimeStamp& time)
    { _rtiAmbassador.timeAdvanceRequestAvailable(toFedTime(time)); }
    void flushQueueRequest(const SGTimeStamp& time)
    { _rtiAmbassador.flushQueueRequest(toFedTime(time)); }

    bool queryGALT(SGTimeStamp& timeStamp)
    {
        RTIfedTime fedTime;
        fedTime.setPositiveInfinity();
        _rtiAmbassador.queryLBTS(fedTime);
        if (fedTime.isPositiveInfinity())
            return false;
        timeStamp = toTimeStamp(fedTime);
        return true;
    }
    bool queryLITS(SGTimeStamp& timeStamp)
    {
        RTIfedTime fedTime;
        fedTime.setPositiveInfinity();
        _rtiAmbassador.queryMinNextEventTime(fedTime);
        if (fedTime.isPositiveInfinity())
            return false;
        timeStamp = toTimeStamp(fedTime);
        return true;
    }
    void queryFederateTime(SGTimeStamp& timeStamp)
    {
        RTIfedTime fedTime;
        _rtiAmbassador.queryFederateTime(fedTime);
        timeStamp = toTimeStamp(fedTime);
    }
    void modifyLookahead(const SGTimeStamp& timeStamp)
    { _rtiAmbassador.modifyLookahead(toFedTime(timeStamp)); }
    void queryLookahead(SGTimeStamp& timeStamp)
    {
        RTIfedTime fedTime;
        _rtiAmbassador.queryLookahead(fedTime);
        timeStamp = toTimeStamp(fedTime);
    }

    RTI::ObjectClassHandle getObjectClassHandle(const std::string& name)
    { return _rtiAmbassador.getObjectClassHandle(name.c_str()); }
    std::string getObjectClassName(const RTI::ObjectClassHandle& handle)
    { return rtiToStdString(_rtiAmbassador.getObjectClassName(handle)); }

    RTI::AttributeHandle getAttributeHandle(const std::string& attributeName, const RTI::ObjectClassHandle& objectClassHandle)
    { return _rtiAmbassador.getAttributeHandle(attributeName.c_str(), objectClassHandle); }
    std::string getAttributeName(const RTI::AttributeHandle& attributeHandle, const RTI::ObjectClassHandle& objectClassHandle)
    { return rtiToStdString(_rtiAmbassador.getAttributeName(attributeHandle, objectClassHandle)); }

    RTI::InteractionClassHandle getInteractionClassHandle(const std::string& name)
    { return _rtiAmbassador.getInteractionClassHandle(name.c_str()); }
    std::string getInteractionClassName(const RTI::InteractionClassHandle& handle)
    { return rtiToStdString(_rtiAmbassador.getInteractionClassName(handle)); }

    RTI::ParameterHandle getParameterHandle(const std::string& parameterName, const RTI::InteractionClassHandle& interactionClassHandle)
    { return _rtiAmbassador.getParameterHandle(parameterName.c_str(), interactionClassHandle); }
    std::string getParameterName(const RTI::ParameterHandle& parameterHandle, const RTI::InteractionClassHandle& interactionClassHandle)
    { return rtiToStdString(_rtiAmbassador.getParameterName(parameterHandle, interactionClassHandle)); }

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

    // The connection class
    RTI::RTIambassador _rtiAmbassador;
    SGWeakPtr<RTI13Federate> _federate;
};

} // namespace simgear

#endif
