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

#include "RTI13Federate.hxx"

#include "RTI13Ambassador.hxx"

namespace simgear {

RTI13Federate::RTI13Federate() :
    _tickTimeout(10),
    _ambassador(new RTI13Ambassador)
{
}

RTI13Federate::~RTI13Federate()
{
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
        _federateHandle = _ambassador->joinFederationExecution(federateType, federationName);
        SG_LOG(SG_NETWORK, SG_INFO, "RTI: Joined federation \""
               << federationName << "\" as \"" << federateType << "\"");
        setFederateType(federateType);
        setFederationName(federationName);
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
    while (!_ambassador->getFederationSynchronizationPointAnnounced(label)) {
        _ambassador->tick(_tickTimeout, 0);
        _ambassador->processQueues();
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
    while (!_ambassador->getFederationSynchronized(label)) {
        _ambassador->tick(_tickTimeout, 0);
        _ambassador->processQueues();
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

    if (_ambassador->getTimeConstrainedEnabled()) {
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

    while (!_ambassador->getTimeConstrainedEnabled()) {
        _ambassador->tick(_tickTimeout, 0);
        _ambassador->processQueues();
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

    if (!_ambassador->getTimeConstrainedEnabled()) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Time constrained is not enabled.");
        return false;
    }

    try {
        _ambassador->disableTimeConstrained();
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

    if (_ambassador->getTimeRegulationEnabled()) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Time regulation already enabled.");
        return false;
    }

    try {
        _ambassador->enableTimeRegulation(SGTimeStamp(), lookahead);
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

    while (!_ambassador->getTimeRegulationEnabled()) {
        _ambassador->tick(_tickTimeout, 0);
        _ambassador->processQueues();
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

    if (!_ambassador->getTimeRegulationEnabled()) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Time regulation is not enabled.");
        return false;
    }

    try {
        _ambassador->disableTimeRegulation();
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

    SGTimeStamp fedTime = _ambassador->getCurrentLogicalTime() + dt;
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

    while (_ambassador->getTimeAdvancePending()) {
        _ambassador->tick(_tickTimeout, 0);
        _ambassador->processQueues();
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
RTI13Federate::tick()
{
    bool result = _ambassador->tick();
    _ambassador->processQueues();
    return result;
}

bool
RTI13Federate::tick(const double& minimum, const double& maximum)
{
    bool result = _ambassador->tick(minimum, maximum);
    _ambassador->processQueues();
    return result;
}

RTI13ObjectClass*
RTI13Federate::createObjectClass(const std::string& objectClassName, HLAObjectClass* hlaObjectClass)
{
    try {
        return _ambassador->createObjectClass(objectClassName, hlaObjectClass);
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
        return _ambassador->getObjectInstance(objectInstanceName);
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

}
