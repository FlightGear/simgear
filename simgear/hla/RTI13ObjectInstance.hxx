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

#ifndef RTI13ObjectInstance_hxx
#define RTI13ObjectInstance_hxx

#include <map>
#include <memory>

#ifndef RTI_USES_STD_FSTREAM
#define RTI_USES_STD_FSTREAM
#endif

#include <RTI.hh>

#include <simgear/structure/SGWeakPtr.hxx>

#include "RTIObjectInstance.hxx"
#include "RTI13ObjectClass.hxx"

namespace simgear {

class RTI13Ambassador;
class RTI13ObjectClass;

typedef std::pair<RTI::AttributeHandle, RTIData> RTI13AttributeHandleDataPair;
typedef std::list<RTI13AttributeHandleDataPair> RTI13AttributeHandleDataPairList;

class RTI13ObjectInstance : public RTIObjectInstance {
public:
    RTI13ObjectInstance(const RTI::ObjectHandle& handle, HLAObjectInstance* hlaObjectInstance, const RTI13ObjectClass* objectClass, RTI13Ambassador* ambassador);
    virtual ~RTI13ObjectInstance();

    const RTI::ObjectHandle& getHandle() const
    { return _handle; }
    void setHandle(const RTI::ObjectHandle& handle)
    { _handle = handle; }

    virtual const RTIObjectClass* getObjectClass() const;
    const RTI13ObjectClass* get13ObjectClass() const;

    unsigned getNumAttributes() const
    { return get13ObjectClass()->getNumAttributes(); }
    unsigned getAttributeIndex(const RTI::AttributeHandle& handle) const
    { return get13ObjectClass()->getAttributeIndex(handle); }
    RTI::AttributeHandle getAttributeHandle(unsigned index) const
    { return get13ObjectClass()->getAttributeHandle(index); }

    virtual std::string getName() const;

    virtual void deleteObjectInstance(const RTIData& tag);
    virtual void deleteObjectInstance(const SGTimeStamp& timeStamp, const RTIData& tag);
    virtual void localDeleteObjectInstance();

    void reflectAttributeValues(RTI13AttributeHandleDataPairList& attributeHandleDataPairList,
                                const RTIData& tag, HLAIndexList& indexPool);
    void reflectAttributeValues(RTI13AttributeHandleDataPairList& attributeHandleDataPairList,
                                const SGTimeStamp& timeStamp, const RTIData& tag, HLAIndexList& indexPool);
    virtual void requestObjectAttributeValueUpdate(const HLAIndexList& indexList);
    void provideAttributeValueUpdate(const std::vector<RTI::AttributeHandle>& attributes);

    virtual void updateAttributeValues(const RTIData& tag);
    virtual void updateAttributeValues(const SGTimeStamp& timeStamp, const RTIData& tag);

    virtual bool isAttributeOwnedByFederate(unsigned index) const;

    void attributesInScope(const std::vector<RTI::AttributeHandle>& attributes);
    void attributesOutOfScope(const std::vector<RTI::AttributeHandle>& attributes);

    void turnUpdatesOnForObjectInstance(const std::vector<RTI::AttributeHandle>& attributes);
    void turnUpdatesOffForObjectInstance(const std::vector<RTI::AttributeHandle>& attributes);

    // Not yet sure what to do here. But the dispatch functions are already there
    void requestAttributeOwnershipAssumption(const std::vector<RTI::AttributeHandle>& attributes, const RTIData& tag);
    void attributeOwnershipDivestitureNotification(const std::vector<RTI::AttributeHandle>& attributes);
    void attributeOwnershipAcquisitionNotification(const std::vector<RTI::AttributeHandle>& attributes);
    void attributeOwnershipUnavailable(const std::vector<RTI::AttributeHandle>& attributes);
    void requestAttributeOwnershipRelease(const std::vector<RTI::AttributeHandle>& attributes, const RTIData& tag);
    void confirmAttributeOwnershipAcquisitionCancellation(const std::vector<RTI::AttributeHandle>& attributes);
    void informAttributeOwnership(RTI::AttributeHandle attributeHandle, RTI::FederateHandle federateHandle);
    void attributeIsNotOwned(RTI::AttributeHandle attributeHandle);
    void attributeOwnedByRTI(RTI::AttributeHandle attributeHandle);

private:
    RTI::ObjectHandle _handle;
    SGSharedPtr<const RTI13ObjectClass> _objectClass;
    SGSharedPtr<RTI13Ambassador> _ambassador;

    // cached storage for updates
    std::unique_ptr<RTI::AttributeHandleValuePairSet> _attributeValuePairSet;
};

}

#endif
