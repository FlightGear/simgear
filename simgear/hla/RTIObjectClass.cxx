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

#include "RTIObjectClass.hxx"

#include "RTIObjectInstance.hxx"

namespace simgear {

RTIObjectClass::RTIObjectClass(HLAObjectClass* hlaObjectClass) :
    _hlaObjectClass(hlaObjectClass)
{
}

RTIObjectClass::~RTIObjectClass()
{
}

void
RTIObjectClass::discoverInstance(RTIObjectInstance* objectInstance, const RTIData& tag) const
{
    SGSharedPtr<HLAObjectClass> hlaObjectClass = _hlaObjectClass.lock();
    if (!hlaObjectClass.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Invalid hla object class pointer in RTIObjectClass::discoverInstance().");
        return;
    }
    hlaObjectClass->discoverInstance(objectInstance, tag);
}

void
RTIObjectClass::startRegistration() const
{
    SGSharedPtr<HLAObjectClass> hlaObjectClass = _hlaObjectClass.lock();
    if (!hlaObjectClass.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Invalid hla object class pointer in RTIObjectClass::startRegstration().");
        return;
    }
    hlaObjectClass->startRegistrationCallback();
}

void
RTIObjectClass::stopRegistration() const
{
    SGSharedPtr<HLAObjectClass> hlaObjectClass = _hlaObjectClass.lock();
    if (!hlaObjectClass.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Invalid hla object class pointer in RTIObjectClass::stopRegistration().");
        return;
    }
    hlaObjectClass->stopRegistrationCallback();
}

}
