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

#ifndef HLAFixedRecordDataType_hxx
#define HLAFixedRecordDataType_hxx

#include <string>
#include <vector>
#include <simgear/structure/SGSharedPtr.hxx>
#include "HLADataType.hxx"

namespace simgear {

class HLAAbstractFixedRecordDataElement;

class HLAFixedRecordDataType : public HLADataType {
public:
    HLAFixedRecordDataType(const std::string& name = "HLAFixedRecordDataType");
    virtual ~HLAFixedRecordDataType();

    virtual void accept(HLADataTypeVisitor& visitor) const;

    virtual const HLAFixedRecordDataType* toFixedRecordDataType() const;

    virtual void releaseDataTypeReferences();

    virtual bool decode(HLADecodeStream& stream, HLAAbstractFixedRecordDataElement& value) const;
    virtual bool encode(HLAEncodeStream& stream, const HLAAbstractFixedRecordDataElement& value) const;

    unsigned getNumFields() const
    { return _fieldList.size(); }

    std::string getFieldName(unsigned i) const
    {
        if (_fieldList.size() <= i)
            return std::string();
        return _fieldList[i].getName();
    }
    const HLADataType* getFieldDataType(unsigned i) const
    {
        if (_fieldList.size() <= i)
            return 0;
        return _fieldList[i].getDataType();
    }

    unsigned getFieldNumber(const std::string& name) const
    {
        for (unsigned i = 0; i < _fieldList.size(); ++i) {
            if (_fieldList[i].getName() != name)
                continue;
            return i;
        }
        return ~0u;
    }

    void addField(const std::string& name, const HLADataType* dataType);

protected:
    virtual void _recomputeAlignmentImplementation();

private:
    struct Field {
        Field(const std::string& name, const HLADataType* dataType) :
            _name(name), _dataType(dataType) {}
        const std::string& getName() const
        { return _name; }

        const HLADataType* getDataType() const
        { return _dataType.get(); }
        void releaseDataTypeReferences()
        { _dataType = 0; }

    private:
        std::string _name;
        SGSharedPtr<const HLADataType> _dataType;
    };

    typedef std::vector<Field> FieldList;
    FieldList _fieldList;
};

} // namespace simgear

#endif
