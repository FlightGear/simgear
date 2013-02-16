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

#include "RTI13InteractionClass.hxx"

#include "RTI13Ambassador.hxx"

namespace simgear {

RTI13InteractionClass::RTI13InteractionClass(HLAInteractionClass* interactionClass, const RTI::InteractionClassHandle& handle, RTI13Ambassador* ambassador) :
    RTIInteractionClass(interactionClass),
    _handle(handle),
    _ambassador(ambassador)
{
}

RTI13InteractionClass::~RTI13InteractionClass()
{
}

bool
RTI13InteractionClass::resolveParameterIndex(const std::string& name, unsigned index)
{
    if (!_ambassador.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "Error: Ambassador is zero.");
        return false;
    }

    if (index != _parameterHandleVector.size()) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Resolving needs to happen in growing index order!");
        return false;
    }

    try {
        RTI::ParameterHandle parameterHandle = _ambassador->getParameterHandle(name, _handle);

        ParameterHandleIndexMap::const_iterator i = _parameterHandleIndexMap.find(parameterHandle);
        if (i != _parameterHandleIndexMap.end()) {
            SG_LOG(SG_NETWORK, SG_WARN, "RTI: Resolving parameterIndex for parameter \"" << name << "\" twice!");
            return false;
        }

        _parameterHandleIndexMap[parameterHandle] = index;
        _parameterHandleVector.push_back(parameterHandle);

        return true;

    } catch (RTI::NameNotFound& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not get interaction class parameter: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::FederateNotExecutionMember& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not get interaction class parameter: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::ConcurrentAccessAttempted& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not get interaction class parameter: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::RTIinternalError& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not get interaction class parameter: " << e._name << " " << e._reason);
        return false;
    } catch (...) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not get interaction class parameter.");
        return false;
    }

    return false;
}

bool
RTI13InteractionClass::publish()
{
    if (!_ambassador.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "Error: Ambassador is zero.");
        return false;
    }

    try {
        _ambassador->publishInteractionClass(_handle);
        return true;
    } catch (RTI::InteractionClassNotDefined& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not publish interaction class: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::FederateNotExecutionMember& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not publish interaction class: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::ConcurrentAccessAttempted& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not publish interaction class: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::SaveInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not publish interaction class: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::RestoreInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not publish interaction class: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::RTIinternalError& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not publish interaction class: " << e._name << " " << e._reason);
        return false;
    } catch (...) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not publish interaction class.");
        return false;
    }

    return false;
}

bool
RTI13InteractionClass::unpublish()
{
    if (!_ambassador.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "Error: Ambassador is zero.");
        return false;
    }

    try {
        _ambassador->unpublishInteractionClass(_handle);
        return true;
    } catch (RTI::InteractionClassNotDefined& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not unpublish interaction class: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::InteractionClassNotPublished& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not unpublish interaction class: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::FederateNotExecutionMember& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not unpublish interaction class: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::ConcurrentAccessAttempted& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not unpublish interaction class: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::SaveInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not unpublish interaction class: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::RestoreInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not unpublish interaction class: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::RTIinternalError& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not unpublish interaction class: " << e._name << " " << e._reason);
        return false;
    } catch (...) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not unpublish interaction class.");
        return false;
    }

    return false;
}

bool
RTI13InteractionClass::subscribe(bool active)
{
    if (!_ambassador.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "Error: Ambassador is zero.");
        return false;
    }

    try {
        _ambassador->subscribeInteractionClass(_handle, active);
        return true;
    } catch (RTI::InteractionClassNotDefined& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not subscribe interaction class: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::FederateNotExecutionMember& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not subscribe interaction class: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::ConcurrentAccessAttempted& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not subscribe interaction class: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::FederateLoggingServiceCalls& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not subscribe interaction class: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::SaveInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not subscribe interaction class: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::RestoreInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not subscribe interaction class: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::RTIinternalError& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not subscribe interaction class: " << e._name << " " << e._reason);
        return false;
    } catch (...) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not subscribe interaction class.");
        return false;
    }

    return false;
}

bool
RTI13InteractionClass::unsubscribe()
{
    if (!_ambassador.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "Error: Ambassador is zero.");
        return false;
    }

    try {
        _ambassador->unsubscribeInteractionClass(_handle);
        return true;
    } catch (RTI::InteractionClassNotDefined& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not unsubscribe interaction class: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::InteractionClassNotSubscribed& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not unsubscribe interaction class: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::FederateNotExecutionMember& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not unsubscribe interaction class: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::ConcurrentAccessAttempted& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not unsubscribe interaction class: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::SaveInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not unsubscribe interaction class: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::RestoreInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not unsubscribe interaction class: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::RTIinternalError& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not unsubscribe interaction class: " << e._name << " " << e._reason);
        return false;
    } catch (...) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not unsubscribe interaction class.");
        return false;
    }

    return false;
}

}
