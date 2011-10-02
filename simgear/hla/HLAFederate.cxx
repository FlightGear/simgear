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

#include "HLAFederate.hxx"

#include "RTI13Federate.hxx"
#include "RTIFederate.hxx"
#include "RTIInteractionClass.hxx"
#include "RTIObjectClass.hxx"
#include "HLADataElement.hxx"
#include "HLADataType.hxx"
#include "HLAOMTXmlVisitor.hxx"

namespace simgear {

HLAFederate::HLAFederate() :
    _version(RTI13),
    _createFederationExecution(true),
    _timeConstrained(false),
    _timeRegulating(false),
    _timeConstrainedByLocalClock(false),
    _done(false)
{
}

HLAFederate::~HLAFederate()
{
}

HLAFederate::Version
HLAFederate::getVersion() const
{
    return _version;
}

bool
HLAFederate::setVersion(HLAFederate::Version version)
{
    if (_rtiFederate.valid()) {
        SG_LOG(SG_NETWORK, SG_ALERT, "HLA: Ignoring HLAFederate parameter setting on already connected federate!");
        return false;
    }
    _version = version;
    return true;
}

const std::list<std::string>&
HLAFederate::getConnectArguments() const
{
    return _connectArguments;
}

bool
HLAFederate::setConnectArguments(const std::list<std::string>& connectArguments)
{
    if (_rtiFederate.valid()) {
        SG_LOG(SG_NETWORK, SG_ALERT, "HLA: Ignoring HLAFederate parameter setting on already connected federate!");
        return false;
    }
    _connectArguments = connectArguments;
    return true;
}

bool
HLAFederate::getCreateFederationExecution() const
{
    return _createFederationExecution;
}

bool
HLAFederate::setCreateFederationExecution(bool createFederationExecution)
{
    _createFederationExecution = createFederationExecution;
    return true;
}

const std::string&
HLAFederate::getFederationExecutionName() const
{
    return _federationExecutionName;
}

bool
HLAFederate::setFederationExecutionName(const std::string& federationExecutionName)
{
    if (_rtiFederate.valid()) {
        SG_LOG(SG_NETWORK, SG_ALERT, "HLA: Ignoring HLAFederate parameter setting on already connected federate!");
        return false;
    }
    _federationExecutionName = federationExecutionName;
    return true;
}

const std::string&
HLAFederate::getFederationObjectModel() const
{
    return _federationObjectModel;
}

bool
HLAFederate::setFederationObjectModel(const std::string& federationObjectModel)
{
    if (_rtiFederate.valid()) {
        SG_LOG(SG_NETWORK, SG_ALERT, "HLA: Ignoring HLAFederate parameter setting on already connected federate!");
        return false;
    }
    _federationObjectModel = federationObjectModel;
    return true;
}

const std::string&
HLAFederate::getFederateType() const
{
    return _federateType;
}

bool
HLAFederate::setFederateType(const std::string& federateType)
{
    if (_rtiFederate.valid()) {
        SG_LOG(SG_NETWORK, SG_ALERT, "HLA: Ignoring HLAFederate parameter setting on already connected federate!");
        return false;
    }
    _federateType = federateType;
    return true;
}

const std::string&
HLAFederate::getFederateName() const
{
    return _federateName;
}

bool
HLAFederate::setFederateName(const std::string& federateName)
{
    if (_rtiFederate.valid()) {
        SG_LOG(SG_NETWORK, SG_ALERT, "HLA: Ignoring HLAFederate parameter setting on already connected federate!");
        return false;
    }
    _federateName = federateName;
    return true;
}

bool
HLAFederate::connect(Version version, const std::list<std::string>& stringList)
{
    if (_rtiFederate.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "HLA: Trying to connect to already connected federate!");
        return false;
    }
    switch (version) {
    case RTI13:
        _rtiFederate = new RTI13Federate(stringList);
        _version = version;
        _connectArguments = stringList;
        break;
    case RTI1516:
        SG_LOG(SG_IO, SG_ALERT, "HLA version RTI1516 not yet(!?) supported.");
        // _rtiFederate = new RTI1516Federate(stringList);
        break;
    case RTI1516E:
        SG_LOG(SG_IO, SG_ALERT, "HLA version RTI1516E not yet(!?) supported.");
        // _rtiFederate = new RTI1516eFederate(stringList);
        break;
    default:
        SG_LOG(SG_NETWORK, SG_WARN, "HLA: Unknown rti version in connect!");
    }
    return _rtiFederate.valid();
}

bool
HLAFederate::connect()
{
    if (_rtiFederate.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "HLA: Trying to connect to already connected federate!");
        return false;
    }
    switch (_version) {
    case RTI13:
        _rtiFederate = new RTI13Federate(_connectArguments);
        break;
    case RTI1516:
        SG_LOG(SG_IO, SG_ALERT, "HLA version RTI1516 not yet(!?) supported.");
        // _rtiFederate = new RTI1516Federate(_connectArguments);
        break;
    case RTI1516E:
        SG_LOG(SG_IO, SG_ALERT, "HLA version RTI1516E not yet(!?) supported.");
        // _rtiFederate = new RTI1516eFederate(_connectArguments);
        break;
    default:
        SG_LOG(SG_NETWORK, SG_WARN, "HLA: Unknown rti version in connect!");
    }
    return _rtiFederate.valid();
}

bool
HLAFederate::disconnect()
{
    if (!_rtiFederate.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "HLA: Accessing unconnected federate!");
        return false;
    }
    _rtiFederate = 0;
    return true;
}

bool
HLAFederate::createFederationExecution(const std::string& federation, const std::string& objectModel)
{
    if (!_rtiFederate.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "HLA: Accessing unconnected federate!");
        return false;
    }

    RTIFederate::FederationManagementResult createResult;
    createResult = _rtiFederate->createFederationExecution(federation, objectModel);
    if (createResult == RTIFederate::FederationManagementFatal)
        return false;

    _federationExecutionName = federation;
    _federationObjectModel = objectModel;
    return true;
}

bool
HLAFederate::destroyFederationExecution(const std::string& federation)
{
    if (!_rtiFederate.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "HLA: Accessing unconnected federate!");
        return false;
    }

    RTIFederate::FederationManagementResult destroyResult;
    destroyResult = _rtiFederate->destroyFederationExecution(federation);
    if (destroyResult == RTIFederate::FederationManagementFatal)
        return false;

    return true;
}

bool
HLAFederate::createFederationExecution()
{
    if (!_rtiFederate.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "HLA: Accessing unconnected federate!");
        return false;
    }

    RTIFederate::FederationManagementResult createResult;
    createResult = _rtiFederate->createFederationExecution(_federationExecutionName, _federationObjectModel);
    if (createResult != RTIFederate::FederationManagementSuccess)
        return false;

    return true;
}

bool
HLAFederate::destroyFederationExecution()
{
    if (!_rtiFederate.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "HLA: Accessing unconnected federate!");
        return false;
    }

    RTIFederate::FederationManagementResult destroyResult;
    destroyResult = _rtiFederate->destroyFederationExecution(_federationExecutionName);
    if (destroyResult != RTIFederate::FederationManagementSuccess)
        return false;

    return true;
}

bool
HLAFederate::join(const std::string& federateType, const std::string& federation)
{
    if (!_rtiFederate.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "HLA: Accessing unconnected federate!");
        return false;
    }

    RTIFederate::FederationManagementResult joinResult;
    joinResult = _rtiFederate->join(federateType, federation);
    if (joinResult == RTIFederate::FederationManagementFatal)
        return false;

    return true;
}

bool
HLAFederate::join()
{
    if (!_rtiFederate.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "HLA: Accessing unconnected federate!");
        return false;
    }

    RTIFederate::FederationManagementResult joinResult;
    joinResult = _rtiFederate->join(_federateType, _federationExecutionName);
    if (joinResult != RTIFederate::FederationManagementSuccess)
        return false;

    return true;
}

bool
HLAFederate::resign()
{
    if (!_rtiFederate.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "HLA: Accessing unconnected federate!");
        return false;
    }
    return _rtiFederate->resign();
}

bool
HLAFederate::createJoinFederationExecution()
{
    if (!_rtiFederate.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "HLA: Accessing unconnected federate!");
        return false;
    }

    for (;;) {
        // Try to join.
        RTIFederate::FederationManagementResult joinResult;
        joinResult = _rtiFederate->join(_federateType, _federationExecutionName);
        switch (joinResult) {
        case RTIFederate::FederationManagementSuccess:
            // Fast return on success
            return true;
        case RTIFederate::FederationManagementFatal:
            // Abort on fatal errors
            return false;
        default:
            break;
        };

        // If not already joinable, try to create the requested federation
        RTIFederate::FederationManagementResult createResult;
        createResult = _rtiFederate->createFederationExecution(_federationExecutionName, _federationObjectModel);
        switch (createResult) {
        case RTIFederate::FederationManagementFatal:
            // Abort on fatal errors
            return false;
        default:
            // Try again to join
            break;
        }
    }
}

bool
HLAFederate::resignDestroyFederationExecution()
{
    if (!_rtiFederate.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "HLA: Accessing unconnected federate!");
        return false;
    }
    
    // Resign ourselves
    bool success = _rtiFederate->resign();

    // and try to destroy, non fatal if still some federates joined
    if (_rtiFederate->destroyFederationExecution(_federationExecutionName) == RTIFederate::FederationManagementFatal)
        success = false;

    return success;
}

bool
HLAFederate::getTimeConstrained() const
{
    return _timeConstrained;
}

bool
HLAFederate::setTimeConstrained(bool timeConstrained)
{
    _timeConstrained = timeConstrained;

    if (_rtiFederate.valid() && _rtiFederate->getJoined()) {
        if (_timeConstrained && !_rtiFederate->getTimeConstrainedEnabled()) {
            if (!enableTimeConstrained())
                return false;
        } else if (!_timeConstrained && _rtiFederate->getTimeConstrainedEnabled()) {
            if (!disableTimeConstrained())
                return false;
        }

    }

    return true;
}

bool
HLAFederate::getTimeConstrainedByLocalClock() const
{
    return _timeConstrainedByLocalClock;
}

bool
HLAFederate::setTimeConstrainedByLocalClock(bool timeConstrainedByLocalClock)
{
    _timeConstrainedByLocalClock = timeConstrainedByLocalClock;

    if (_rtiFederate.valid() && _rtiFederate->getJoined()) {
        if (_timeConstrainedByLocalClock) {
            if (!enableTimeConstrainedByLocalClock())
                return false;
        }
    }

    return true;
}

bool
HLAFederate::getTimeRegulating() const
{
    return _timeRegulating;
}

bool
HLAFederate::setTimeRegulating(bool timeRegulating)
{
    _timeRegulating = timeRegulating;

    if (_rtiFederate.valid() && _rtiFederate->getJoined()) {
        if (_timeRegulating && !_rtiFederate->getTimeRegulationEnabled()) {
            if (!enableTimeRegulation())
                return false;
        } else if (!_timeRegulating && _rtiFederate->getTimeRegulationEnabled()) {
            if (!disableTimeRegulation())
                return false;
        }

    }
    return true;
}

bool
HLAFederate::setLeadTime(const SGTimeStamp& leadTime)
{
    if (leadTime < SGTimeStamp::fromSec(0)) {
        SG_LOG(SG_NETWORK, SG_WARN, "Ignoring negative lead time!");
        return false;
    }

    _leadTime = leadTime;

    if (_rtiFederate.valid() && _rtiFederate->getJoined()) {
        if (!modifyLookahead(_leadTime + SGTimeStamp::fromSec(_timeIncrement.toSecs()*0.9))) {
            SG_LOG(SG_NETWORK, SG_WARN, "Cannot modify lookahead!");
            return false;
        }
    }

    return true;
}

const SGTimeStamp&
HLAFederate::getLeadTime() const
{
    return _leadTime;
}

bool
HLAFederate::setTimeIncrement(const SGTimeStamp& timeIncrement)
{
    if (timeIncrement < SGTimeStamp::fromSec(0)) {
        SG_LOG(SG_NETWORK, SG_WARN, "Ignoring negative time increment!");
        return false;
    }

    _timeIncrement = timeIncrement;

    if (_rtiFederate.valid() && _rtiFederate->getJoined()) {
        if (!modifyLookahead(_leadTime + SGTimeStamp::fromSec(_timeIncrement.toSecs()*0.9))) {
            SG_LOG(SG_NETWORK, SG_WARN, "Cannot modify lookahead!");
            return false;
        }
    }

    return true;
}

const SGTimeStamp&
HLAFederate::getTimeIncrement() const
{
    return _timeIncrement;
}

bool
HLAFederate::enableTimeConstrained()
{
    if (!_rtiFederate.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "HLA: Accessing unconnected federate!");
        return false;
    }

    if (!_rtiFederate->enableTimeConstrained()) {
        SG_LOG(SG_NETWORK, SG_WARN, "HLA: Could not enable time constrained!");
        return false;
    }

    while (!_rtiFederate->getTimeConstrainedEnabled()) {
        _rtiFederate->processMessage();
    }

    return true;
}

bool
HLAFederate::disableTimeConstrained()
{
    if (!_rtiFederate.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "HLA: Accessing unconnected federate!");
        return false;
    }
    return _rtiFederate->disableTimeConstrained();
}

bool
HLAFederate::enableTimeConstrainedByLocalClock()
{
    // Compute the time offset from the system time to the simulation time
    SGTimeStamp federateTime;
    if (!queryFederateTime(federateTime)) {
        SG_LOG(SG_NETWORK, SG_WARN, "Cannot get federate time!");
        return false;
    }
    _localClockOffset = SGTimeStamp::now() - federateTime;
    return true;
}

bool
HLAFederate::enableTimeRegulation(const SGTimeStamp& lookahead)
{
    if (!_rtiFederate.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "HLA: Accessing unconnected federate!");
        return false;
    }

    if (!_rtiFederate->enableTimeRegulation(lookahead)) {
        SG_LOG(SG_NETWORK, SG_WARN, "HLA: Could not enable time regulation!");
        return false;
    }

    while (!_rtiFederate->getTimeRegulationEnabled()) {
        _rtiFederate->processMessage();
    }

    return true;
}

bool
HLAFederate::enableTimeRegulation()
{
    if (!enableTimeRegulation(SGTimeStamp::fromSec(0))) {
        SG_LOG(SG_NETWORK, SG_WARN, "Cannot enable time regulation!");
        return false;
    }
    if (!modifyLookahead(_leadTime + SGTimeStamp::fromSec(_timeIncrement.toSecs()*0.9))) {
        SG_LOG(SG_NETWORK, SG_WARN, "Cannot modify lookahead!");
        return false;
    }
    return true;
}

bool
HLAFederate::disableTimeRegulation()
{
    if (!_rtiFederate.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "HLA: Accessing unconnected federate!");
        return false;
    }
    return _rtiFederate->disableTimeRegulation();
}

bool
HLAFederate::modifyLookahead(const SGTimeStamp& timeStamp)
{
    if (!_rtiFederate.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "HLA: Accessing unconnected federate!");
        return false;
    }
    return _rtiFederate->modifyLookahead(timeStamp);
}

bool
HLAFederate::timeAdvanceBy(const SGTimeStamp& timeIncrement)
{
    if (!_rtiFederate.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "HLA: Accessing unconnected federate!");
        return false;
    }

    SGTimeStamp timeStamp;
    if (!_rtiFederate->queryFederateTime(timeStamp)) {
        SG_LOG(SG_NETWORK, SG_WARN, "HLA: Could not query federate time!");
        return false;
    }

    if (!_rtiFederate->timeAdvanceRequest(timeStamp + timeIncrement)) {
        SG_LOG(SG_NETWORK, SG_WARN, "HLA: Time advance request failed!");
        return false;
    }

    return processMessages();
}

bool
HLAFederate::timeAdvance(const SGTimeStamp& timeStamp)
{
    if (!_rtiFederate.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "HLA: Accessing unconnected federate!");
        return false;
    }

    if (!_rtiFederate->timeAdvanceRequest(timeStamp)) {
        SG_LOG(SG_NETWORK, SG_WARN, "HLA: Time advance request failed!");
        return false;
    }

    return processMessages();
}

bool
HLAFederate::timeAdvanceAvailable()
{
    if (!_rtiFederate.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "HLA: Accessing unconnected federate!");
        return false;
    }

    SGTimeStamp timeStamp;
    if (!_rtiFederate->queryGALT(timeStamp)) {
        SG_LOG(SG_NETWORK, SG_WARN, "HLA: Could not query GALT!");
        return false;
    }

    if (!_rtiFederate->timeAdvanceRequestAvailable(timeStamp)) {
        SG_LOG(SG_NETWORK, SG_WARN, "HLA: Time advance request failed!");
        return false;
    }

    return processMessages();
}

bool
HLAFederate::queryFederateTime(SGTimeStamp& timeStamp)
{
    if (!_rtiFederate.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "HLA: Accessing unconnected federate!");
        return false;
    }
    return _rtiFederate->queryFederateTime(timeStamp);
}

bool
HLAFederate::queryLookahead(SGTimeStamp& timeStamp)
{
    if (!_rtiFederate.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "HLA: Accessing unconnected federate!");
        return false;
    }
    return _rtiFederate->queryLookahead(timeStamp);
}

bool
HLAFederate::processMessage()
{
    if (!_rtiFederate.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "HLA: Accessing unconnected federate!");
        return false;
    }
    return _rtiFederate->processMessage();
}

bool
HLAFederate::processMessage(const SGTimeStamp& timeout)
{
    if (!_rtiFederate.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "HLA: Accessing unconnected federate!");
        return false;
    }
    return _rtiFederate->processMessages(timeout.toSecs(), 0);
}

bool
HLAFederate::processMessages()
{
    if (!_rtiFederate.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "HLA: Accessing unconnected federate!");
        return false;
    }

    while (_rtiFederate->getTimeAdvancePending()) {
        _rtiFederate->processMessage();
    }

    if (_timeConstrainedByLocalClock) {
        SGTimeStamp federateTime;
        if (!_rtiFederate->queryFederateTime(federateTime)) {
            SG_LOG(SG_NETWORK, SG_WARN, "HLA: Error querying federate time!");
            return false;
        }
        SGTimeStamp systemTime = federateTime + _localClockOffset;
        for (;;) {
            double rest = (systemTime - SGTimeStamp::now()).toSecs();
            if (rest < 0)
                break;
            _rtiFederate->processMessages(rest, rest);
        }
    }
    
    // Now flush just what is left
    while (!_rtiFederate->processMessages(0, 0));

    return true;
}

bool
HLAFederate::tick(const double& minimum, const double& maximum)
{
    if (!_rtiFederate.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "HLA: Accessing unconnected federate!");
        return false;
    }
    return _rtiFederate->processMessages(minimum, maximum);
}

bool
HLAFederate::readObjectModelTemplate(const std::string& objectModel,
                                     HLAFederate::ObjectModelFactory& objectModelFactory)
{
    if (!_rtiFederate.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "HLA: Accessing unconnected federate!");
        return false;
    }

    // The XML version of the federate object model.
    // This one covers the generic attributes, parameters and data types.
    HLAOMTXmlVisitor omtXmlVisitor;
    try {
        readXML(objectModel, omtXmlVisitor);
    } catch (const sg_throwable& e) {
        SG_LOG(SG_IO, SG_ALERT, "Could not open HLA XML object model file: "
               << e.getMessage());
        return false;
    } catch (...) {
        SG_LOG(SG_IO, SG_ALERT, "Could not open HLA XML object model file");
        return false;
    }

    unsigned numObjectClasses = omtXmlVisitor.getNumObjectClasses();
    for (unsigned i = 0; i < numObjectClasses; ++i) {
        const HLAOMTXmlVisitor::ObjectClass* objectClass = omtXmlVisitor.getObjectClass(i);
        std::string objectClassName = objectClass->getName();

        SGSharedPtr<HLAObjectClass> hlaObjectClass = objectModelFactory.createObjectClass(objectClassName, *this);
        if (!hlaObjectClass.valid()) {
            SG_LOG(SG_IO, SG_INFO, "Ignoring object class \"" << objectClassName << "\".");
            continue;
        }

        bool publish = objectModelFactory.publishObjectClass(objectClassName, objectClass->getSharing());
        bool subscribe = objectModelFactory.subscribeObjectClass(objectClassName, objectClass->getSharing());

        std::set<unsigned> subscriptions;
        std::set<unsigned> publications;

        // process the attributes
        for (unsigned j = 0; j < objectClass->getNumAttributes(); ++j) {
            const simgear::HLAOMTXmlVisitor::Attribute* attribute;
            attribute = objectClass->getAttribute(j);

            std::string attributeName = attribute->getName();
            unsigned index = hlaObjectClass->getAttributeIndex(attributeName);

            if (index == ~0u) {
                SG_LOG(SG_IO, SG_WARN, "RTI does not know the \"" << attributeName << "\" attribute!");
                continue;
            }

            SGSharedPtr<HLADataType> dataType;
            dataType = omtXmlVisitor.getAttributeDataType(objectClassName, attributeName);
            if (!dataType.valid()) {
                SG_LOG(SG_IO, SG_WARN, "Could not find data type for attribute \""
                       << attributeName << "\" in object class \"" << objectClassName << "\"!");
            }
            hlaObjectClass->setAttributeDataType(index, dataType);

            HLAUpdateType updateType = HLAUndefinedUpdate;
            if (attribute->_updateType == "Periodic")
                updateType = HLAPeriodicUpdate;
            else if (attribute->_updateType == "Static")
                updateType = HLAStaticUpdate;
            else if (attribute->_updateType == "Conditional")
                updateType = HLAConditionalUpdate;
            hlaObjectClass->setAttributeUpdateType(index, updateType);

            if (subscribe && objectModelFactory.subscribeAttribute(objectClassName, attributeName, attribute->_sharing))
                subscriptions.insert(index);
            if (publish && objectModelFactory.publishAttribute(objectClassName, attributeName, attribute->_sharing))
                publications.insert(index);
        }

        if (publish)
            hlaObjectClass->publish(publications);
        if (subscribe)
            hlaObjectClass->subscribe(subscriptions, true);

        _objectClassMap[objectClassName] = hlaObjectClass;
    }

    return true;
}

HLAObjectClass*
HLAFederate::getObjectClass(const std::string& name)
{
    ObjectClassMap::const_iterator i = _objectClassMap.find(name);
    if (i == _objectClassMap.end())
        return 0;
    return i->second.get();
}

const HLAObjectClass*
HLAFederate::getObjectClass(const std::string& name) const
{
    ObjectClassMap::const_iterator i = _objectClassMap.find(name);
    if (i == _objectClassMap.end())
        return 0;
    return i->second.get();
}

HLAInteractionClass*
HLAFederate::getInteractionClass(const std::string& name)
{
    InteractionClassMap::const_iterator i = _interactionClassMap.find(name);
    if (i == _interactionClassMap.end())
        return 0;
    return i->second.get();
}

const HLAInteractionClass*
HLAFederate::getInteractionClass(const std::string& name) const
{
    InteractionClassMap::const_iterator i = _interactionClassMap.find(name);
    if (i == _interactionClassMap.end())
        return 0;
    return i->second.get();
}

void
HLAFederate::setDone(bool done)
{
    _done = done;
}

bool
HLAFederate::getDone() const
{
    return _done;
}

bool
HLAFederate::readObjectModel()
{
    /// Currently empty, but is called at the right time so that
    /// the object model is present when it is needed
    return true;
}

bool
HLAFederate::subscribe()
{
    /// Currently empty, but is called at the right time
    return true;
}

bool
HLAFederate::publish()
{
    /// Currently empty, but is called at the right time
    return true;
}

bool
HLAFederate::init()
{
    // We need to talk to the rti
    if (!connect())
        return false;
    // Join ...
    if (_createFederationExecution) {
        if (!createJoinFederationExecution())
            return false;
    } else {
        if (!join())
            return false;
    }
    // Read the xml file containing the object model
    if (!readObjectModel()) {
        shutdown();
        return false;
    }
    // start being time constrained if required
    if (_timeConstrained) {
        if (!enableTimeConstrained()) {
            shutdown();
            return false;
        }
    }
    // Now that we are potentially time constrained, we can subscribe.
    // This is to make sure we do not get any time stamped message
    // converted to a non time stamped message by the rti.
    if (!subscribe()) {
        shutdown();
        return false;
    }
    // Before we publish anything start getting regulating if required
    if (_timeRegulating) {
        if (!enableTimeRegulation()) {
            shutdown();
            return false;
        }
    }
    // Note that starting from here, we need to be careful with things
    // requireing unbounded time. The rest of the federation might wait
    // for us to finish!
    
    // Compute the time offset from the system time to the simulation time
    if (_timeConstrainedByLocalClock) {
        if (!enableTimeConstrainedByLocalClock()) {
            SG_LOG(SG_NETWORK, SG_WARN, "Cannot enable time constrained by local clock!");
            shutdown();
            return false;
        }
    }
    
    // Publish what we want to write
    if (!publish()) {
        shutdown();
        return false;
    }
    
    return true;
}

bool
HLAFederate::update()
{
    return timeAdvanceBy(_timeIncrement);
}

bool
HLAFederate::shutdown()
{
    // On shutdown, just try all in order.
    // If something goes wrong, continue and try to get out here as good as possible.
    bool ret = true;
    
    if (_createFederationExecution) {
        if (!resignDestroyFederationExecution())
            ret = false;
    } else {
        if (!resign())
            ret = false;
    }
    
    if (!disconnect())
        ret = false;
    
    return ret;
}

bool
HLAFederate::exec()
{
    if (!init())
        return false;
    
    while (!getDone()) {
        if (!update()) {
            shutdown();
            return false;
        }
    }
    
    if (!shutdown())
        return false;
    
    return true;
}

} // namespace simgear
