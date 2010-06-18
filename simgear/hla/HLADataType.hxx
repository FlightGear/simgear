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

#ifndef HLADataType_hxx
#define HLADataType_hxx

#include <string>
#include <simgear/structure/SGWeakPtr.hxx>
#include <simgear/structure/SGWeakReferenced.hxx>
#include "RTIData.hxx"

namespace simgear {

class HLADataTypeVisitor;

class HLADataTypeReference;
class HLABasicDataType;
class HLAArrayDataType;
class HLAEnumeratedDataType;
class HLAFixedRecordDataType;
class HLAVariantDataType;

enum HLAUpdateType {
    HLAStaticUpdate,
    HLAPeriodicUpdate,
    HLAConditionalUpdate,
    HLAUndefinedUpdate
};

class HLADataType : public SGWeakReferenced {
public:
    virtual ~HLADataType();

    const std::string& getName() const
    { return _name; }

    const std::string& getSemantics() const
    { return _semantics; }
    void setSemantics(const std::string& semantics)
    { _semantics = semantics; }

    unsigned getAlignment() const
    { return _alignment; }

    virtual void accept(HLADataTypeVisitor& visitor) const;

    virtual const HLADataTypeReference* toDataTypeReference() const;
    virtual const HLABasicDataType* toBasicDataType() const;
    virtual const HLAArrayDataType* toArrayDataType() const;
    virtual const HLAEnumeratedDataType* toEnumeratedDataType() const;
    virtual const HLAFixedRecordDataType* toFixedRecordDataType() const;
    virtual const HLAVariantDataType* toVariantDataType() const;

protected:
    HLADataType(const std::string& name, unsigned alignment = 1);
    void setAlignment(unsigned alignment);

private:
    std::string _name;
    std::string _semantics;
    unsigned _alignment;
};

// Weak reference to a data type. Used to implement self referencing data types
class HLADataTypeReference : public HLADataType {
public:
    HLADataTypeReference(const SGSharedPtr<HLADataType>& dataType) :
        HLADataType(dataType->getName(), dataType->getAlignment()),
        _dataType(dataType)
    { }
    virtual ~HLADataTypeReference();

    SGSharedPtr<const HLADataType> getDataType() const
    { return _dataType.lock(); }

    virtual void accept(HLADataTypeVisitor& visitor) const;
    virtual const HLADataTypeReference* toDataTypeReference() const;

private:
    SGWeakPtr<const HLADataType> _dataType;
};

} // namespace simgear

#endif
