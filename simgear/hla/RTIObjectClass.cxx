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

#include "RTIObjectClass.hxx"

#include "simgear/debug/logstream.hxx"
#include "RTIObjectInstance.hxx"

namespace simgear {

RTIObjectClass::RTIObjectClass(HLAObjectClass* objectClass) :
    _objectClass(objectClass)
{
}

RTIObjectClass::~RTIObjectClass()
{
}

void
RTIObjectClass::discoverInstance(RTIObjectInstance* objectInstance, const RTIData& tag) const
{
    if (!_objectClass) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Invalid hla object class pointer in RTIObjectClass::discoverInstance().");
        return;
    }
    _objectClass->_discoverInstance(objectInstance, tag);
}

void
RTIObjectClass::startRegistration() const
{
    if (!_objectClass) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Invalid hla object class pointer in RTIObjectClass::startRegstration().");
        return;
    }
    _objectClass->_startRegistration();
}

void
RTIObjectClass::stopRegistration() const
{
    if (!_objectClass) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Invalid hla object class pointer in RTIObjectClass::stopRegistration().");
        return;
    }
    _objectClass->_stopRegistration();
}

}
