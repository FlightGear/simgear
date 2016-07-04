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

#include "HLAFederate.hxx"

#include <algorithm>

#include "simgear/debug/logstream.hxx"
#include "simgear/misc/sg_path.hxx"

#include "RTIFederate.hxx"
#include "RTIFederateFactoryRegistry.hxx"
#include "RTI13FederateFactory.hxx"
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
    // For now instantiate the current only available factory here explicitly
    RTI13FederateFactory::instance();
}

HLAFederate::~HLAFederate()
{
    _clearRTI();

    // Remove the data type references from the data types.
    // This is to remove the cycles from the data types that might happen if a data type references itself
    for (DataTypeMap::iterator i = _dataTypeMap.begin(); i != _dataTypeMap.end(); ++i) {
        i->second->releaseDataTypeReferences();
    }
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

bool
HLAFederate::setVersion(const std::string& version)
{
    if (version == "RTI13")
        return setVersion(RTI13);
    else if (version == "RTI1516")
        return setVersion(RTI1516);
    else if (version == "RTI1516E")
        return setVersion(RTI1516E);
    else {
        /// at some time think about routing these down to the factory
        SG_LOG(SG_NETWORK, SG_ALERT, "HLA: Unknown version string in HLAFederate::setVersion!");
        return false;
    }
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
    _version = version;
    _connectArguments = stringList;
    return connect();
}

bool
HLAFederate::connect()
{
    if (_rtiFederate.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "HLA: Trying to connect to already connected federate!");
        return false;
    }

    SGSharedPtr<RTIFederateFactoryRegistry> registry = RTIFederateFactoryRegistry::instance();
    if (!registry) {
        SG_LOG(SG_NETWORK, SG_ALERT, "HLA: RTIFederateFactoryRegistry is no longer available!");
        return false;
    }

    switch (_version) {
    case RTI13:
        _rtiFederate = registry->create("RTI13", _connectArguments);
        break;
    case RTI1516:
        _rtiFederate = registry->create("RTI1516", _connectArguments);
        break;
    case RTI1516E:
        _rtiFederate = registry->create("RTI1516E", _connectArguments);
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

    _clearRTI();
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
        if (RTIFederate::ProcessMessageFatal == _rtiFederate->processMessage()) {
            SG_LOG(SG_NETWORK, SG_WARN, "HLA: Fatal error on processing messages!");
            return false;
        }
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
        if (RTIFederate::ProcessMessageFatal == _rtiFederate->processMessage()) {
            SG_LOG(SG_NETWORK, SG_WARN, "HLA: Fatal error on processing messages!");
            return false;
        }
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
    if (_rtiFederate->queryGALT(timeStamp)) {
        if (!_rtiFederate->timeAdvanceRequestAvailable(timeStamp)) {
            SG_LOG(SG_NETWORK, SG_WARN, "HLA: Time advance request failed!");
            return false;
        }
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
    
    if (RTIFederate::ProcessMessageFatal == _rtiFederate->processMessage()) {
        SG_LOG(SG_NETWORK, SG_WARN, "HLA: Fatal error on processing messages!");
        return false;
    }
    return true;
}

bool
HLAFederate::processMessage(const SGTimeStamp& timeout)
{
    if (!_rtiFederate.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "HLA: Accessing unconnected federate!");
        return false;
    }

    if (RTIFederate::ProcessMessageFatal == _rtiFederate->processMessages(timeout.toSecs(), 0)) {
        SG_LOG(SG_NETWORK, SG_WARN, "HLA: Fatal error on processing messages!");
        return false;
    }
    return true;
}

bool
HLAFederate::processMessages()
{
    if (!_rtiFederate.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "HLA: Accessing unconnected federate!");
        return false;
    }

    while (_rtiFederate->getTimeAdvancePending()) {
        if (RTIFederate::ProcessMessageFatal == _rtiFederate->processMessage()) {
            SG_LOG(SG_NETWORK, SG_WARN, "HLA: Fatal error on processing messages!");
            return false;
        }
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
            if (RTIFederate::ProcessMessageFatal == _rtiFederate->processMessages(rest, rest)) {
                SG_LOG(SG_NETWORK, SG_WARN, "HLA: Fatal error on processing messages!");
                return false;
            }
        }
    }
    
    // Now flush just what is left
    while (true) {
        RTIFederate::ProcessMessageResult result = _rtiFederate->processMessages(0, 0);
        if (RTIFederate::ProcessMessageLast == result)
            break;
        if (RTIFederate::ProcessMessageFatal == result) {
            SG_LOG(SG_NETWORK, SG_WARN, "HLA: Fatal error on processing messages!");
            return false;
        }
    }

    return true;
}

bool
HLAFederate::readRTI13ObjectModelTemplate(const std::string& objectModel)
{
    SG_LOG(SG_IO, SG_WARN, "HLA version RTI13 not yet(!?) supported.");
    return false;
}

bool
HLAFederate::readRTI1516ObjectModelTemplate(const std::string& objectModel)
{
    // The XML version of the federate object model.
    // This one covers the generic attributes, parameters and data types.
    HLAOMTXmlVisitor omtXmlVisitor;
    try {
        readXML(SGPath(objectModel), omtXmlVisitor);
    } catch (const sg_throwable& e) {
        SG_LOG(SG_IO, SG_ALERT, "Could not open HLA XML object model file: "
               << e.getMessage());
        return false;
    } catch (...) {
        SG_LOG(SG_IO, SG_ALERT, "Could not open HLA XML object model file");
        return false;
    }

    omtXmlVisitor.setToFederate(*this);

    return resolveObjectModel();
}

bool
HLAFederate::readRTI1516EObjectModelTemplate(const std::string& objectModel)
{
    SG_LOG(SG_IO, SG_WARN, "HLA version RTI1516E not yet(!?) supported.");
    return false;
}

bool
HLAFederate::resolveObjectModel()
{
    if (!_rtiFederate.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "HLA: Accessing unconnected federate!");
        return false;
    }

    for (InteractionClassMap::iterator i = _interactionClassMap.begin(); i != _interactionClassMap.end(); ++i) {
        RTIInteractionClass* rtiInteractionClass = _rtiFederate->createInteractionClass(i->second->getName(), i->second.get());
        if (!rtiInteractionClass) {
            SG_LOG(SG_NETWORK, SG_ALERT, "HLAFederate::_insertInteractionClass(): "
                   "No RTIInteractionClass found for \"" << i->second->getName() << "\"!");
            return false;
        }
        i->second->_setRTIInteractionClass(rtiInteractionClass);
    }

    for (ObjectClassMap::iterator i = _objectClassMap.begin(); i != _objectClassMap.end(); ++i) {
        RTIObjectClass* rtiObjectClass = _rtiFederate->createObjectClass(i->second->getName(), i->second.get());
        if (!rtiObjectClass) {
            SG_LOG(SG_NETWORK, SG_ALERT, "HLAFederate::_insertObjectClass(): "
                   "No RTIObjectClass found for \"" << i->second->getName() << "\"!");
            return false;
        }
        i->second->_setRTIObjectClass(rtiObjectClass);
    }

    return true;
}

const HLADataType*
HLAFederate::getDataType(const std::string& name) const
{
    DataTypeMap::const_iterator i = _dataTypeMap.find(name);
    if (i == _dataTypeMap.end())
        return 0;
    return i->second.get();
}

bool
HLAFederate::insertDataType(const std::string& name, const SGSharedPtr<HLADataType>& dataType)
{
    if (!dataType.valid())
        return false;
    if (_dataTypeMap.find(name) != _dataTypeMap.end()) {
        SG_LOG(SG_IO, SG_ALERT, "HLAFederate::insertDataType: data type with name \""
               << name << "\" already known to federate!");
        return false;
    }
    _dataTypeMap.insert(DataTypeMap::value_type(name, dataType));
    return true;
}

void
HLAFederate::recomputeDataTypeAlignment()
{
    // Finish alignment computations
    bool changed;
    do {
        changed = false;
        for (DataTypeMap::iterator i = _dataTypeMap.begin(); i != _dataTypeMap.end(); ++i) {
            if (i->second->recomputeAlignment())
                changed = true;
        }
    } while (changed);
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

HLAInteractionClass*
HLAFederate::createInteractionClass(const std::string& name)
{
    return new HLAInteractionClass(name, this);
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

HLAObjectClass*
HLAFederate::createObjectClass(const std::string& name)
{
    return new HLAObjectClass(name, this);
}

HLAObjectInstance*
HLAFederate::getObjectInstance(const std::string& name)
{
    ObjectInstanceMap::const_iterator i = _objectInstanceMap.find(name);
    if (i == _objectInstanceMap.end())
        return 0;
    return i->second.get();
}

const HLAObjectInstance*
HLAFederate::getObjectInstance(const std::string& name) const
{
    ObjectInstanceMap::const_iterator i = _objectInstanceMap.find(name);
    if (i == _objectInstanceMap.end())
        return 0;
    return i->second.get();
}

HLAObjectInstance*
HLAFederate::createObjectInstance(HLAObjectClass* objectClass, const std::string& name)
{
    return new HLAObjectInstance(objectClass);
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
    // Depending on the actual version, try to find an apropriate
    // file format for the given file. The first one is always the
    // version native object model file format.
    switch (getVersion()) {
    case RTI13:
        if (readRTI13ObjectModelTemplate(getFederationObjectModel()))
            return true;
        if (readRTI1516ObjectModelTemplate(getFederationObjectModel()))
            return true;
        return readRTI1516EObjectModelTemplate(getFederationObjectModel());
    case RTI1516:
        if (readRTI1516ObjectModelTemplate(getFederationObjectModel()))
            return true;
        if (readRTI1516EObjectModelTemplate(getFederationObjectModel()))
            return true;
        return readRTI13ObjectModelTemplate(getFederationObjectModel());
    case RTI1516E:
        if (readRTI1516EObjectModelTemplate(getFederationObjectModel()))
            return true;
        if (readRTI1516ObjectModelTemplate(getFederationObjectModel()))
            return true;
        return readRTI13ObjectModelTemplate(getFederationObjectModel());
    default:
        return false;
    }
}

bool
HLAFederate::subscribe()
{
    for (InteractionClassMap::iterator i = _interactionClassMap.begin(); i != _interactionClassMap.end(); ++i) {
        if (!i->second->subscribe())
            return false;
    }

    for (ObjectClassMap::iterator i = _objectClassMap.begin(); i != _objectClassMap.end(); ++i) {
        if (!i->second->subscribe())
            return false;
    }

    return true;
}

bool
HLAFederate::publish()
{
    for (InteractionClassMap::iterator i = _interactionClassMap.begin(); i != _interactionClassMap.end(); ++i) {
        if (!i->second->publish())
            return false;
    }

    for (ObjectClassMap::iterator i = _objectClassMap.begin(); i != _objectClassMap.end(); ++i) {
        if (!i->second->publish())
            return false;
    }

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
    if (_timeIncrement <= SGTimeStamp::fromSec(0))
        return processMessages();
    else
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

void
HLAFederate::_clearRTI()
{
    for (InteractionClassMap::iterator i = _interactionClassMap.begin(); i != _interactionClassMap.end(); ++i)
        i->second->_clearRTIInteractionClass();
    for (ObjectInstanceMap::iterator i = _objectInstanceMap.begin(); i != _objectInstanceMap.end(); ++i)
        i->second->_clearRTIObjectInstance();
    for (ObjectClassMap::iterator i = _objectClassMap.begin(); i != _objectClassMap.end(); ++i)
        i->second->_clearRTIObjectClass();

    _rtiFederate = 0;
}

bool
HLAFederate::_insertInteractionClass(HLAInteractionClass* interactionClass)
{
    if (!interactionClass)
        return false;
    if (_interactionClassMap.find(interactionClass->getName()) != _interactionClassMap.end()) {
        SG_LOG(SG_IO, SG_ALERT, "HLA: _insertInteractionClass: object instance with name \""
               << interactionClass->getName() << "\" already known to federate!");
        return false;
    }
    _interactionClassMap.insert(InteractionClassMap::value_type(interactionClass->getName(), interactionClass));
    return true;
}

bool
HLAFederate::_insertObjectClass(HLAObjectClass* objectClass)
{
    if (!objectClass)
        return false;
    if (_objectClassMap.find(objectClass->getName()) != _objectClassMap.end()) {
        SG_LOG(SG_IO, SG_ALERT, "HLA: _insertObjectClass: object instance with name \""
               << objectClass->getName() << "\" already known to federate!");
        return false;
    }
    _objectClassMap.insert(ObjectClassMap::value_type(objectClass->getName(), objectClass));
    return true;
}

bool
HLAFederate::_insertObjectInstance(HLAObjectInstance* objectInstance)
{
    if (!objectInstance)
        return false;
    if (objectInstance->getName().empty()) {
        SG_LOG(SG_IO, SG_ALERT, "HLA: _insertObjectInstance: trying to insert object instance with empty name!");
        return false;
    }
    if (_objectInstanceMap.find(objectInstance->getName()) != _objectInstanceMap.end()) {
        SG_LOG(SG_IO, SG_WARN, "HLA: _insertObjectInstance: object instance with name \""
               << objectInstance->getName() << "\" already known to federate!");
        return false;
    }
    _objectInstanceMap.insert(ObjectInstanceMap::value_type(objectInstance->getName(), objectInstance));
    return true;
}

void
HLAFederate::_eraseObjectInstance(const std::string& name)
{
    ObjectInstanceMap::iterator i = _objectInstanceMap.find(name);
    if (i == _objectInstanceMap.end()) {
        SG_LOG(SG_IO, SG_WARN, "HLA: _eraseObjectInstance: object instance with name \""
               << name << "\" not known to federate!");
        return;
    }
    _objectInstanceMap.erase(i);
}

} // namespace simgear
