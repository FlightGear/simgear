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

#include "RTI13ObjectClass.hxx"
#include "RTI13Ambassador.hxx"

namespace simgear {

RTI13ObjectClass::RTI13ObjectClass(HLAObjectClass* hlaObjectClass, RTI::ObjectClassHandle& handle, RTI13Ambassador* ambassador) :
    RTIObjectClass(hlaObjectClass),
    _handle(handle),
    _ambassador(ambassador)
{
    if (0 != getOrCreateAttributeIndex("privilegeToDelete") &&
        0 != getOrCreateAttributeIndex("HLAprivilegeToDeleteObject"))
        SG_LOG(SG_NETWORK, SG_WARN, "RTI13ObjectClass: Cannot find object root attribute.");
}

RTI13ObjectClass::~RTI13ObjectClass()
{
}

std::string
RTI13ObjectClass::getName() const
{
    SGSharedPtr<RTI13Ambassador> ambassador = _ambassador.lock();
    if (!ambassador.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "Error: Ambassador is zero.");
        return std::string();
    }
    return ambassador->getObjectClassName(_handle);
}

unsigned
RTI13ObjectClass::getNumAttributes() const
{
    return _attributeHandleVector.size();
}

unsigned
RTI13ObjectClass::getAttributeIndex(const std::string& name) const
{
    SGSharedPtr<RTI13Ambassador> ambassador = _ambassador.lock();
    if (!ambassador.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "Error: Ambassador is zero.");
        return ~0u;
    }

    try {
        RTI::AttributeHandle attributeHandle = ambassador->getAttributeHandle(name, _handle);

        AttributeHandleIndexMap::const_iterator i = _attributeHandleIndexMap.find(attributeHandle);
        if (i !=  _attributeHandleIndexMap.end())
            return i->second;

        return ~0u;

    } catch (RTI::ObjectClassNotDefined& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not get object class attribute: " << e._name << " " << e._reason);
        return ~0u;
    } catch (RTI::NameNotFound& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not get object class attribute: " << e._name << " " << e._reason);
        return ~0u;
    } catch (RTI::FederateNotExecutionMember& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not get object class attribute: " << e._name << " " << e._reason);
        return ~0u;
    } catch (RTI::ConcurrentAccessAttempted& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not get object class attribute: " << e._name << " " << e._reason);
        return ~0u;
    } catch (RTI::RTIinternalError& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not get object class attribute: " << e._name << " " << e._reason);
        return ~0u;
    } catch (...) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not get object class attribute.");
        return ~0u;
    }
}

unsigned
RTI13ObjectClass::getOrCreateAttributeIndex(const std::string& name)
{
    SGSharedPtr<RTI13Ambassador> ambassador = _ambassador.lock();
    if (!ambassador.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "Error: Ambassador is zero.");
        return ~0u;
    }

    try {
        RTI::AttributeHandle attributeHandle = ambassador->getAttributeHandle(name, _handle);

        AttributeHandleIndexMap::const_iterator i = _attributeHandleIndexMap.find(attributeHandle);
        if (i !=  _attributeHandleIndexMap.end())
            return i->second;

        unsigned index = _attributeHandleVector.size();
        _attributeHandleIndexMap[attributeHandle] = index;
        _attributeHandleVector.push_back(attributeHandle);
        _attributeDataVector.push_back(name);

        return index;

    } catch (RTI::ObjectClassNotDefined& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not get object class attribute: " << e._name << " " << e._reason);
        return ~0u;
    } catch (RTI::NameNotFound& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not get object class attribute: " << e._name << " " << e._reason);
        return ~0u;
    } catch (RTI::FederateNotExecutionMember& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not get object class attribute: " << e._name << " " << e._reason);
        return ~0u;
    } catch (RTI::ConcurrentAccessAttempted& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not get object class attribute: " << e._name << " " << e._reason);
        return ~0u;
    } catch (RTI::RTIinternalError& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not get object class attribute: " << e._name << " " << e._reason);
        return ~0u;
    } catch (...) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not get object class attribute.");
        return ~0u;
    }
}

// std::string
// RTI13ObjectClass::getAttributeName(unsigned index) const
// {
//     SGSharedPtr<RTI13Ambassador> ambassador = _ambassador.lock();
//     if (!ambassador.valid()) {
//         SG_LOG(SG_NETWORK, SG_WARN, "Error: Ambassador is zero.");
//         return std::string();
//     }

//     try {
//         return ambassador->getAttributeName(getAttributeHandle(index), _handle);
//     } catch (RTI::ObjectClassNotDefined& e) {
//         SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not get object class attribute name: " << e._name << " " << e._reason);
//         return std::string();
//     } catch (RTI::AttributeNotDefined& e) {
//         SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not get object class attribute name: " << e._name << " " << e._reason);
//         return std::string();
//     } catch (RTI::FederateNotExecutionMember& e) {
//         SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not get object class attribute name: " << e._name << " " << e._reason);
//         return std::string();
//     } catch (RTI::ConcurrentAccessAttempted& e) {
//         SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not get object class attribute name: " << e._name << " " << e._reason);
//         return std::string();
//     } catch (RTI::RTIinternalError& e) {
//         SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not get object class attribute name: " << e._name << " " << e._reason);
//         return std::string();
//     } catch (...) {
//         SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not get object class attribute name.");
//         return std::string();
//     }
// }

bool
RTI13ObjectClass::publish(const std::set<unsigned>& indexSet)
{
    SGSharedPtr<RTI13Ambassador> ambassador = _ambassador.lock();
    if (!ambassador.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "Error: Ambassador is zero.");
        return false;
    }

    try {
        std::auto_ptr<RTI::AttributeHandleSet> attributeHandleSet(RTI::AttributeHandleSetFactory::create(indexSet.size()));
        for (std::set<unsigned>::const_iterator i = indexSet.begin(); i != indexSet.end(); ++i) {
            if (_attributeHandleVector.size() <= *i) {
                SG_LOG(SG_NETWORK, SG_WARN, "RTI13ObjectClass::publish(): Invalid attribute index!");
                continue;
            }
            attributeHandleSet->add(_attributeHandleVector[*i]);
        }

        ambassador->publishObjectClass(_handle, *attributeHandleSet);

        for (unsigned i = 0; i < getNumAttributes(); ++i) {
            _attributeDataVector[i]._published = true;
        }

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
    SGSharedPtr<RTI13Ambassador> ambassador = _ambassador.lock();
    if (!ambassador.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "Error: Ambassador is zero.");
        return false;
    }

    try {
        ambassador->unpublishObjectClass(_handle);

        for (unsigned i = 0; i < getNumAttributes(); ++i) {
            _attributeDataVector[i]._published = false;
        }

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
RTI13ObjectClass::subscribe(const std::set<unsigned>& indexSet, bool active)
{
    SGSharedPtr<RTI13Ambassador> ambassador = _ambassador.lock();
    if (!ambassador.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "Error: Ambassador is zero.");
        return false;
    }

    try {
        std::auto_ptr<RTI::AttributeHandleSet> attributeHandleSet(RTI::AttributeHandleSetFactory::create(indexSet.size()));
        for (std::set<unsigned>::const_iterator i = indexSet.begin();
             i != indexSet.end(); ++i) {
            if (_attributeHandleVector.size() <= *i) {
                SG_LOG(SG_NETWORK, SG_WARN, "RTI13ObjectClass::subscribe(): Invalid attribute index!");
                continue;
            }
            attributeHandleSet->add(_attributeHandleVector[*i]);
        }

        ambassador->subscribeObjectClassAttributes(_handle, *attributeHandleSet, active);

        for (unsigned i = 0; i < getNumAttributes(); ++i) {
            _attributeDataVector[i]._subscribed = true;
        }

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
    SGSharedPtr<RTI13Ambassador> ambassador = _ambassador.lock();
    if (!ambassador.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "Error: Ambassador is zero.");
        return 0;
    }

    try {
        ambassador->unsubscribeObjectClass(_handle);

        for (unsigned i = 0; i < getNumAttributes(); ++i) {
            _attributeDataVector[i]._subscribed = false;
        }

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
    SGSharedPtr<RTI13Ambassador> ambassador = _ambassador.lock();
    if (!ambassador.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "Error: Ambassador is zero.");
        return 0;
    }

    try {
        return ambassador->registerObjectInstance(this, hlaObjectInstance);
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
