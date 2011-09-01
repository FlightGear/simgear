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

#include "HLAFederate.hxx"

#include "RTI13Federate.hxx"
#include "RTIFederate.hxx"
#include "RTIInteractionClass.hxx"
#include "RTIObjectClass.hxx"
#include "HLADataElement.hxx"
#include "HLADataType.hxx"
#include "HLAOMTXmlVisitor.hxx"

namespace simgear {

HLAFederate::HLAFederate()
{
}

HLAFederate::~HLAFederate()
{
}

std::string
HLAFederate::getFederateType() const
{
    if (!_rtiFederate.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "HLA: Accessing unconnected federate!");
        return std::string();
    }
    return _rtiFederate->getFederateType();
}

std::string
HLAFederate::getFederationName() const
{
    if (!_rtiFederate.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "HLA: Accessing unconnected federate!");
        return std::string();
    }
    return _rtiFederate->getFederationName();
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
    return _rtiFederate->createFederationExecution(federation, objectModel);
}

bool
HLAFederate::destroyFederationExecution(const std::string& federation)
{
    if (!_rtiFederate.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "HLA: Accessing unconnected federate!");
        return false;
    }
    return _rtiFederate->destroyFederationExecution(federation);
}

bool
HLAFederate::join(const std::string& federateType, const std::string& federation)
{
    if (!_rtiFederate.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "HLA: Accessing unconnected federate!");
        return false;
    }
    return _rtiFederate->join(federateType, federation);
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
HLAFederate::enableTimeConstrained()
{
    if (!_rtiFederate.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "HLA: Accessing unconnected federate!");
        return false;
    }
    return _rtiFederate->enableTimeConstrained();
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
HLAFederate::enableTimeRegulation(const SGTimeStamp& lookahead)
{
    if (!_rtiFederate.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "HLA: Accessing unconnected federate!");
        return false;
    }
    return _rtiFederate->enableTimeRegulation(lookahead);
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
HLAFederate::timeAdvanceRequestBy(const SGTimeStamp& dt)
{
    if (!_rtiFederate.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "HLA: Accessing unconnected federate!");
        return false;
    }
    return _rtiFederate->timeAdvanceRequestBy(dt);
}

bool
HLAFederate::timeAdvanceRequest(const SGTimeStamp& dt)
{
    if (!_rtiFederate.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "HLA: Accessing unconnected federate!");
        return false;
    }
    return _rtiFederate->timeAdvanceRequest(dt);
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
HLAFederate::modifyLookahead(const SGTimeStamp& timeStamp)
{
    if (!_rtiFederate.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "HLA: Accessing unconnected federate!");
        return false;
    }
    return _rtiFederate->modifyLookahead(timeStamp);
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
HLAFederate::tick()
{
    if (!_rtiFederate.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "HLA: Accessing unconnected federate!");
        return false;
    }
    return _rtiFederate->tick();
}

bool
HLAFederate::tick(const double& minimum, const double& maximum)
{
    if (!_rtiFederate.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "HLA: Accessing unconnected federate!");
        return false;
    }
    return _rtiFederate->tick(minimum, maximum);
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

} // namespace simgear
