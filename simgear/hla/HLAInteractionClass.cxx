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

#include "HLAInteractionClass.hxx"

#include <simgear/debug/logstream.hxx>

#include "HLADataElement.hxx"
#include "HLAFederate.hxx"

#include "RTIInteractionClass.hxx"

namespace simgear {

HLAInteractionClass::HLAInteractionClass(const std::string& name, HLAFederate* federate) :
    _federate(federate),
    _rtiInteractionClass(0),
    _name(name),
    _subscriptionType(HLAUnsubscribed),
    _publicationType(HLAUnpublished)
{
    if (!federate) {
        SG_LOG(SG_NETWORK, SG_ALERT, "HLAInteractionClass::HLAInteractionClass(): "
               "No parent federate given for interaction class \"" << getName() << "\"!");
        return;
    }
    federate->_insertInteractionClass(this);
}

HLAInteractionClass::~HLAInteractionClass()
{
    // HLAInteractionClass objects only get deleted when the parent federate
    // dies. So we do not need to deregister there.

    _clearRTIInteractionClass();
}

const std::string&
HLAInteractionClass::getName() const
{
    return _name;
}

const SGWeakPtr<HLAFederate>&
HLAInteractionClass::getFederate() const
{
    return _federate;
}

HLASubscriptionType
HLAInteractionClass::getSubscriptionType() const
{
    return _subscriptionType;
}

void
HLAInteractionClass::setSubscriptionType(HLASubscriptionType subscriptionType)
{
    _subscriptionType = subscriptionType;
}

HLAPublicationType
HLAInteractionClass::getPublicationType() const
{
    return _publicationType;
}

void
HLAInteractionClass::setPublicationType(HLAPublicationType publicationType)
{
    _publicationType = publicationType;
}

unsigned
HLAInteractionClass::getNumParameters() const
{
    return _parameterVector.size();
}

unsigned
HLAInteractionClass::addParameter(const std::string& name)
{
    unsigned index = _parameterVector.size();
    _nameIndexMap[name] = index;
    _parameterVector.push_back(Parameter(name));
    _resolveParameterIndex(name, index);
    return index;
}

unsigned
HLAInteractionClass::getParameterIndex(const std::string& name) const
{
    NameIndexMap::const_iterator i = _nameIndexMap.find(name);
    if (i == _nameIndexMap.end())
        return ~0u;
    return i->second;
}

std::string
HLAInteractionClass::getParameterName(unsigned index) const
{
    if (_parameterVector.size() <= index)
        return std::string();
    return _parameterVector[index]._name;
}

const HLADataType*
HLAInteractionClass::getParameterDataType(unsigned index) const
{
    if (_parameterVector.size() <= index)
        return 0;
    return _parameterVector[index]._dataType.get();
}

void
HLAInteractionClass::setParameterDataType(unsigned index, const SGSharedPtr<const HLADataType>& dataType)
{
    if (_parameterVector.size() <= index)
        return;
    _parameterVector[index]._dataType = dataType;
}

HLADataElement::IndexPathPair
HLAInteractionClass::getIndexPathPair(const HLADataElement::StringPathPair& stringPathPair) const
{
    unsigned index = getParameterIndex(stringPathPair.first);
    if (getNumParameters() <= index) {
        SG_LOG(SG_NETWORK, SG_ALERT, "HLAInteractionClass::getIndexPathPair(\""
               << HLADataElement::toString(stringPathPair)
               << "\"): Could not resolve attribute \"" << stringPathPair.first
               << "\" for interaction class \"" << getName() << "\"!");
    }
    return HLADataElement::IndexPathPair(index, stringPathPair.second);
}

HLADataElement::IndexPathPair
HLAInteractionClass::getIndexPathPair(const std::string& path) const
{
    return getIndexPathPair(HLADataElement::toStringPathPair(path));
}

bool
HLAInteractionClass::subscribe()
{
    if (!_rtiInteractionClass) {
        SG_LOG(SG_NETWORK, SG_WARN, "HLAInteractionClass::subscribe(): No RTIInteractionClass!");
        return false;
    }
    switch (_subscriptionType) {
    case HLAUnsubscribed:
        return _rtiInteractionClass->unsubscribe();
    case HLASubscribedActive:
        return _rtiInteractionClass->subscribe(true);
    case HLASubscribedPassive:
        return _rtiInteractionClass->subscribe(false);
    }
    return false;
}

bool
HLAInteractionClass::unsubscribe()
{
    if (!_rtiInteractionClass) {
        SG_LOG(SG_NETWORK, SG_WARN, "HLAInteractionClass::unsubscribe(): No RTIInteractionClass!");
        return false;
    }
    return _rtiInteractionClass->unsubscribe();
}

bool
HLAInteractionClass::publish()
{
    if (!_rtiInteractionClass) {
        SG_LOG(SG_NETWORK, SG_WARN, "HLAInteractionClass::publish(): No RTIInteractionClass\"!");
        return false;
    }
    switch (_publicationType) {
    case HLAUnpublished:
        return _rtiInteractionClass->unpublish();
    case HLAPublished:
        return _rtiInteractionClass->publish();
    }
    return false;
}

bool
HLAInteractionClass::unpublish()
{
    if (!_rtiInteractionClass) {
        SG_LOG(SG_NETWORK, SG_WARN, "HLAInteractionClass::unpublish(): No RTIInteractionClass\"!");
        return false;
    }
    return _rtiInteractionClass->unpublish();
}

void
HLAInteractionClass::_setRTIInteractionClass(RTIInteractionClass* interactionClass)
{
    if (_rtiInteractionClass) {
        SG_LOG(SG_NETWORK, SG_ALERT, "HLAInteractionClass: Setting RTIInteractionClass twice for interaction class \"" << getName() << "\"!");
        return;
    }
    _rtiInteractionClass = interactionClass;
    if (_rtiInteractionClass->_interactionClass != this) {
        SG_LOG(SG_NETWORK, SG_ALERT, "HLAInteractionClass: backward reference does not match!");
        return;
    }
    for (unsigned i = 0; i < _parameterVector.size(); ++i)
        _resolveParameterIndex(_parameterVector[i]._name, i);
}

void
HLAInteractionClass::_resolveParameterIndex(const std::string& name, unsigned index)
{
    if (!_rtiInteractionClass)
        return;
    if (!_rtiInteractionClass->resolveParameterIndex(name, index))
        SG_LOG(SG_NETWORK, SG_ALERT, "HLAInteractionClass: Could not resolve parameter \""
               << name << "\" for interaction class \"" << getName() << "\"!");
}

void
HLAInteractionClass::_clearRTIInteractionClass()
{
    if (!_rtiInteractionClass)
        return;
    _rtiInteractionClass->_interactionClass = 0;
    _rtiInteractionClass = 0;
}

} // namespace simgear
