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

#ifndef HLAVariantRecordDataType_hxx
#define HLAVariantRecordDataType_hxx

#include <string>
#include <vector>
#include "simgear/structure/SGSharedPtr.hxx"
#include "HLADataType.hxx"
#include "HLAEnumeratedDataType.hxx"

namespace simgear {

class HLAAbstractVariantRecordDataElement;

class HLAVariantRecordDataType : public HLADataType {
public:
    HLAVariantRecordDataType(const std::string& name = "HLAVariantRecordDataType");
    virtual ~HLAVariantRecordDataType();

    virtual void accept(HLADataTypeVisitor& visitor) const;

    virtual const HLAVariantRecordDataType* toVariantRecordDataType() const;

    virtual void releaseDataTypeReferences();
    
    virtual bool decode(HLADecodeStream& stream, HLAAbstractVariantRecordDataElement& value) const;
    virtual bool encode(HLAEncodeStream& stream, const HLAAbstractVariantRecordDataElement& value) const;

    const HLAEnumeratedDataType* getEnumeratedDataType() const
    { return _enumeratedDataType.get(); }
    void setEnumeratedDataType(HLAEnumeratedDataType* dataType);

    bool addAlternative(const std::string& name, const std::string& enumerator,
                        const HLADataType* dataType, const std::string& semantics);

    unsigned getNumAlternatives() const
    { return _alternativeList.size(); }

    unsigned getAlternativeIndex(const std::string& enumerator) const
    {
        if (!_enumeratedDataType.valid())
            return ~unsigned(0);
        return _enumeratedDataType->getIndex(enumerator);
    }

    const HLADataType* getAlternativeDataType(unsigned index) const
    {
        if (_alternativeList.size() <= index)
            return 0;
        return _alternativeList[index]._dataType.get();
    }
    const HLADataType* getAlternativeDataType(const std::string& enumerator) const
    { return getAlternativeDataType(getAlternativeIndex(enumerator)); }

    std::string getAlternativeName(unsigned index) const
    {
        if (_alternativeList.size() <= index)
            return std::string();
        return _alternativeList[index]._name;
    }
    std::string getAlternativeName(const std::string& enumerator) const
    { return getAlternativeName(getAlternativeIndex(enumerator)); }

    std::string getAlternativeSemantics(unsigned index) const
    {
        if (_alternativeList.size() <= index)
            return std::string();
        return _alternativeList[index]._semantics;
    }
    std::string getAlternativeSemantics(const std::string& enumerator) const
    { return getAlternativeSemantics(getAlternativeIndex(enumerator)); }

protected:
    virtual void _recomputeAlignmentImplementation();

private:
    SGSharedPtr<HLAEnumeratedDataType> _enumeratedDataType;

    struct Alternative {
        std::string _name;
        SGSharedPtr<const HLADataType> _dataType;
        std::string _semantics;
    };

    typedef std::vector<Alternative> AlternativeList;
    AlternativeList _alternativeList;
};

} // namespace simgear

#endif
