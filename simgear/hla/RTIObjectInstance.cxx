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

#include "RTIObjectInstance.hxx"
#include "RTIObjectClass.hxx"
#include "HLAObjectInstance.hxx"

namespace simgear {

RTIObjectInstance::RTIObjectInstance(HLAObjectInstance* hlaObjectInstance) :
    _hlaObjectInstance(hlaObjectInstance),
    _pendingAttributeUpdateRequest(false)
{
}

RTIObjectInstance::~RTIObjectInstance()
{
}

unsigned
RTIObjectInstance::getNumAttributes() const
{
    return getObjectClass()->getNumAttributes();
}

unsigned
RTIObjectInstance::getAttributeIndex(const std::string& name) const
{
    return getObjectClass()->getAttributeIndex(name);
}

std::string
RTIObjectInstance::getAttributeName(unsigned index) const
{
    return getObjectClass()->getAttributeName(index);
}

void
RTIObjectInstance::removeInstance(const RTIData& tag)
{
    SGSharedPtr<HLAObjectInstance> hlaObjectInstance =  _hlaObjectInstance.lock();
    if (!hlaObjectInstance.valid())
        return;
    hlaObjectInstance->removeInstance(tag);
}

void
RTIObjectInstance::reflectAttributeValues(const RTIIndexDataPairList& dataPairList, const RTIData& tag)
{
    for (RTIIndexDataPairList::const_iterator i = dataPairList.begin();
         i != dataPairList.end(); ++i) {
        reflectAttributeValue(i->first, i->second);
    }

    SGSharedPtr<HLAObjectInstance> hlaObjectInstance =  _hlaObjectInstance.lock();
    if (!hlaObjectInstance.valid())
        return;
    hlaObjectInstance->reflectAttributeValues(dataPairList, tag);
}

void
RTIObjectInstance::reflectAttributeValues(const RTIIndexDataPairList& dataPairList,
                                          const SGTimeStamp& timeStamp, const RTIData& tag)
{
    for (RTIIndexDataPairList::const_iterator i = dataPairList.begin();
         i != dataPairList.end(); ++i) {
        reflectAttributeValue(i->first, i->second);
    }

    SGSharedPtr<HLAObjectInstance> hlaObjectInstance =  _hlaObjectInstance.lock();
    if (!hlaObjectInstance.valid())
        return;
    hlaObjectInstance->reflectAttributeValues(dataPairList, timeStamp, tag);
}

}
