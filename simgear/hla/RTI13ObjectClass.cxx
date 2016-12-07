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

#include "RTI13ObjectClass.hxx"
#include "RTI13Ambassador.hxx"

namespace simgear {

RTI13ObjectClass::RTI13ObjectClass(HLAObjectClass* hlaObjectClass, const RTI::ObjectClassHandle& handle, RTI13Ambassador* ambassador) :
    RTIObjectClass(hlaObjectClass),
    _handle(handle),
    _ambassador(ambassador)
{
}

RTI13ObjectClass::~RTI13ObjectClass()
{
}

bool
RTI13ObjectClass::resolveAttributeIndex(const std::string& name, unsigned index)
{
    if (!_ambassador.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "Error: Ambassador is zero.");
        return false;
    }

    if (index != _attributeHandleVector.size()) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Resolving needs to happen in growing index order!");
        return false;
    }

    try {
        RTI::AttributeHandle attributeHandle = _ambassador->getAttributeHandle(name, _handle);

        AttributeHandleIndexMap::const_iterator i = _attributeHandleIndexMap.find(attributeHandle);
        if (i != _attributeHandleIndexMap.end()) {
            SG_LOG(SG_NETWORK, SG_WARN, "RTI: Resolving attributeIndex for attribute \"" << name << "\" twice!");
            return false;
        }

        _attributeHandleIndexMap[attributeHandle] = index;
        _attributeHandleVector.push_back(attributeHandle);

        return true;

    } catch (RTI::ObjectClassNotDefined& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not get object class attribute: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::NameNotFound& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not get object class attribute: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::FederateNotExecutionMember& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not get object class attribute: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::ConcurrentAccessAttempted& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not get object class attribute: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::RTIinternalError& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not get object class attribute: " << e._name << " " << e._reason);
        return false;
    } catch (...) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not get object class attribute.");
        return false;
    }
}

unsigned
RTI13ObjectClass::getNumAttributes() const
{
    return _attributeHandleVector.size();
}

bool
RTI13ObjectClass::publish(const HLAIndexList& indexList)
{
    if (!_ambassador.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "Error: Ambassador is zero.");
        return false;
    }

    try {
        unsigned numAttributes = getNumAttributes();
        std::unique_ptr<RTI::AttributeHandleSet> attributeHandleSet(RTI::AttributeHandleSetFactory::create(numAttributes));
        for (HLAIndexList::const_iterator i = indexList.begin(); i != indexList.end(); ++i) {
            if (_attributeHandleVector.size() <= *i) {
                SG_LOG(SG_NETWORK, SG_WARN, "RTI13ObjectClass::publish(): Invalid attribute index!");
                continue;
            }
            attributeHandleSet->add(_attributeHandleVector[*i]);
        }

        _ambassador->publishObjectClass(_handle, *attributeHandleSet);

        return true;
    } catch (RTI::ObjectClassNotDefined& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not publish object class: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::AttributeNotDefined& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not publish object class: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::OwnershipAcquisitionPending& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not publish object class: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::FederateNotExecutionMember& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not publish object class: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::ConcurrentAccessAttempted& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not publish object class: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::SaveInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not publish object class: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::RestoreInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not publish object class: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::RTIinternalError& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not publish object class: " << e._name << " " << e._reason);
        return false;
    } catch (...) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not publish object class.");
        return false;
    }
}

bool
RTI13ObjectClass::unpublish()
{
    if (!_ambassador.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "Error: Ambassador is zero.");
        return false;
    }

    try {
        _ambassador->unpublishObjectClass(_handle);

        return true;
    } catch (RTI::ObjectClassNotDefined& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not unpublish object class: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::ObjectClassNotPublished& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not unpublish object class: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::OwnershipAcquisitionPending& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not unpublish object class: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::FederateNotExecutionMember& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not unpublish object class: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::ConcurrentAccessAttempted& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not unpublish object class: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::SaveInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not unpublish object class: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::RestoreInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not unpublish object class: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::RTIinternalError& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not unpublish object class: " << e._name << " " << e._reason);
        return false;
    } catch (...) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not unpublish object class.");
        return false;
    }
}

bool
RTI13ObjectClass::subscribe(const HLAIndexList& indexList, bool active)
{
    if (!_ambassador.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "Error: Ambassador is zero.");
        return false;
    }

    try {
        unsigned numAttributes = getNumAttributes();
        std::unique_ptr<RTI::AttributeHandleSet> attributeHandleSet(RTI::AttributeHandleSetFactory::create(numAttributes));
        for (HLAIndexList::const_iterator i = indexList.begin(); i != indexList.end(); ++i) {
            if (_attributeHandleVector.size() <= *i) {
                SG_LOG(SG_NETWORK, SG_WARN, "RTI13ObjectClass::subscribe(): Invalid attribute index!");
                continue;
            }
            attributeHandleSet->add(_attributeHandleVector[*i]);
        }

        _ambassador->subscribeObjectClassAttributes(_handle, *attributeHandleSet, active);

        return true;
    } catch (RTI::ObjectClassNotDefined& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not subscribe object class: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::AttributeNotDefined& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not subscribe object class: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::FederateNotExecutionMember& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not subscribe object class: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::ConcurrentAccessAttempted& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not subscribe object class: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::SaveInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not subscribe object class: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::RestoreInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not subscribe object class: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::RTIinternalError& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not subscribe object class: " << e._name << " " << e._reason);
        return false;
    } catch (...) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not subscribe object class.");
        return false;
    }
}

bool
RTI13ObjectClass::unsubscribe()
{
    if (!_ambassador.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "Error: Ambassador is zero.");
        return 0;
    }

    try {
        _ambassador->unsubscribeObjectClass(_handle);

        return true;
    } catch (RTI::ObjectClassNotDefined& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not unsubscribe object class: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::ObjectClassNotSubscribed& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not unsubscribe object class: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::FederateNotExecutionMember& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not unsubscribe object class: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::ConcurrentAccessAttempted& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not unsubscribe object class: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::SaveInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not unsubscribe object class: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::RestoreInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not unsubscribe object class: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::RTIinternalError& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not unsubscribe object class: " << e._name << " " << e._reason);
        return false;
    } catch (...) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not unsubscribe object class.");
        return false;
    }
}

RTIObjectInstance*
RTI13ObjectClass::registerObjectInstance(HLAObjectInstance* hlaObjectInstance)
{
    if (!_ambassador.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "Error: Ambassador is zero.");
        return 0;
    }

    SGSharedPtr<RTI13Federate> federate = _ambassador->_federate.lock();
    if (!federate.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "Error: Federate is zero.");
        return 0;
    }

    try {
        RTI::ObjectHandle objectHandle = _ambassador->registerObjectInstance(getHandle());
        RTI13ObjectInstance* objectInstance = new RTI13ObjectInstance(objectHandle, hlaObjectInstance, this, _ambassador.get());
        federate->insertObjectInstance(objectInstance);
        return objectInstance;
    } catch (RTI::ObjectClassNotDefined& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not register object instance: " << e._name << " " << e._reason);
        return 0;
    } catch (RTI::ObjectClassNotPublished& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not register object instance: " << e._name << " " << e._reason);
        return 0;
    } catch (RTI::FederateNotExecutionMember& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not register object instance: " << e._name << " " << e._reason);
        return 0;
    } catch (RTI::ConcurrentAccessAttempted& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not register object instance: " << e._name << " " << e._reason);
        return 0;
    } catch (RTI::SaveInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not register object instance: " << e._name << " " << e._reason);
        return 0;
    } catch (RTI::RestoreInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not register object instance: " << e._name << " " << e._reason);
        return 0;
    } catch (RTI::RTIinternalError& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not register object instance: " << e._name << " " << e._reason);
        return 0;
    }
}

}
