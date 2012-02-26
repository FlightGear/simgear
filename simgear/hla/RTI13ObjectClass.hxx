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

#ifndef RTI13ObjectClass_hxx
#define RTI13ObjectClass_hxx

#include <map>

#ifndef RTI_USES_STD_FSTREAM
#define RTI_USES_STD_FSTREAM
#endif

#include <RTI.hh>

#include <simgear/structure/SGWeakPtr.hxx>

#include "RTIObjectClass.hxx"

namespace simgear {

class RTI13Ambassador;
class RTIObjectInstance;

class RTI13ObjectClass : public RTIObjectClass {
public:
    RTI13ObjectClass(HLAObjectClass* hlaObjectClass, const RTI::ObjectClassHandle& handle, RTI13Ambassador* ambassador);
    virtual ~RTI13ObjectClass();

    const RTI::ObjectClassHandle& getHandle() const
    { return _handle; }

    virtual bool resolveAttributeIndex(const std::string& name, unsigned index);

    virtual unsigned getNumAttributes() const;

    unsigned getAttributeIndex(const RTI::AttributeHandle& handle) const
    {
        AttributeHandleIndexMap::const_iterator i = _attributeHandleIndexMap.find(handle);
        if (i == _attributeHandleIndexMap.end())
            return ~0u;
        return i->second;
    }
    RTI::AttributeHandle getAttributeHandle(unsigned index) const
    {
        if (_attributeHandleVector.size() <= index)
            return -1;
        return _attributeHandleVector[index];
    }

    virtual bool publish(const HLAIndexList& indexList);
    virtual bool unpublish();

    virtual bool subscribe(const HLAIndexList& indexList, bool);
    virtual bool unsubscribe();

    virtual RTIObjectInstance* registerObjectInstance(HLAObjectInstance* hlaObjectInstance);

private:
    RTI::ObjectClassHandle _handle;
    SGSharedPtr<RTI13Ambassador> _ambassador;

    typedef std::map<RTI::AttributeHandle, unsigned> AttributeHandleIndexMap;
    AttributeHandleIndexMap _attributeHandleIndexMap;

    typedef std::vector<RTI::AttributeHandle> AttributeHandleVector;
    AttributeHandleVector _attributeHandleVector;
};

}

#endif
