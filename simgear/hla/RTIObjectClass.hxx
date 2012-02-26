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

#ifndef RTIObjectClass_hxx
#define RTIObjectClass_hxx

#include <string>
#include "simgear/structure/SGReferenced.hxx"
#include "RTIData.hxx"
#include "HLAObjectClass.hxx"

namespace simgear {

class RTIData;
class RTIObjectInstance;
class HLAObjectClass;

class RTIObjectClass : public SGReferenced {
public:
    RTIObjectClass(HLAObjectClass* objectClass);
    virtual ~RTIObjectClass();

    virtual bool resolveAttributeIndex(const std::string& name, unsigned index) = 0;

    virtual unsigned getNumAttributes() const = 0;

    virtual bool publish(const HLAIndexList& indexList) = 0;
    virtual bool unpublish() = 0;

    virtual bool subscribe(const HLAIndexList& indexList, bool) = 0;
    virtual bool unsubscribe() = 0;

    // Factory to create an object instance that can be used in this current federate
    virtual RTIObjectInstance* registerObjectInstance(HLAObjectInstance*) = 0;

    // Call back into HLAObjectClass
    void discoverInstance(RTIObjectInstance* objectInstance, const RTIData& tag) const;

    void startRegistration() const;
    void stopRegistration() const;

private:
    HLAObjectClass* _objectClass;

    friend class HLAObjectClass;
};

}

#endif
