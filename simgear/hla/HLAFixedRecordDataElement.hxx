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

#ifndef HLAFixedRecordDataElement_hxx
#define HLAFixedRecordDataElement_hxx

#include <string>
#include <vector>
#include "HLADataElement.hxx"
#include "HLAFixedRecordDataType.hxx"

namespace simgear {

class HLAAbstractFixedRecordDataElement : public HLADataElement {
public:
    HLAAbstractFixedRecordDataElement(const HLAFixedRecordDataType* dataType);
    virtual ~HLAAbstractFixedRecordDataElement();

    virtual void accept(HLADataElementVisitor& visitor);
    virtual void accept(HLAConstDataElementVisitor& visitor) const;

    virtual bool decode(HLADecodeStream& stream);
    virtual bool encode(HLAEncodeStream& stream) const;

    virtual const HLAFixedRecordDataType* getDataType() const;
    virtual bool setDataType(const HLADataType* dataType);
    void setDataType(const HLAFixedRecordDataType* dataType);

    unsigned getNumFields() const;
    unsigned getFieldNumber(const std::string& name) const;

    const HLADataType* getFieldDataType(unsigned i) const;
    const HLADataType* getFieldDataType(const std::string& name) const;

    virtual bool decodeField(HLADecodeStream& stream, unsigned i) = 0;
    virtual bool encodeField(HLAEncodeStream& stream, unsigned i) const = 0;

private:
    SGSharedPtr<const HLAFixedRecordDataType> _dataType;
};

class HLAFixedRecordDataElement : public HLAAbstractFixedRecordDataElement {
public:
    HLAFixedRecordDataElement(const HLAFixedRecordDataType* dataType);
    virtual ~HLAFixedRecordDataElement();

    virtual bool setDataType(const HLADataType* dataType);

    virtual bool setDataElement(HLADataElementIndex::const_iterator begin, HLADataElementIndex::const_iterator end, HLADataElement* dataElement);
    virtual HLADataElement* getDataElement(HLADataElementIndex::const_iterator begin, HLADataElementIndex::const_iterator end);
    virtual const HLADataElement* getDataElement(HLADataElementIndex::const_iterator begin, HLADataElementIndex::const_iterator end) const;

    virtual bool decodeField(HLADecodeStream& stream, unsigned i);
    virtual bool encodeField(HLAEncodeStream& stream, unsigned i) const;

    HLADataElement* getField(const std::string& name);
    const HLADataElement* getField(const std::string& name) const;

    HLADataElement* getField(unsigned i);
    const HLADataElement* getField(unsigned i) const;

    void setField(unsigned index, HLADataElement* value);
    void setField(const std::string& name, HLADataElement* value);

protected:
    virtual void _setStamp(Stamp* stamp);

private:
    typedef std::vector<SGSharedPtr<HLADataElement> > FieldVector;
    FieldVector _fieldVector;
};

}

#endif
