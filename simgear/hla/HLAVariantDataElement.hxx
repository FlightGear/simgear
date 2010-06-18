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

#ifndef HLAVariantDataElement_hxx
#define HLAVariantDataElement_hxx

#include <string>
#include <simgear/structure/SGSharedPtr.hxx>
#include "HLADataElement.hxx"
#include "HLAVariantDataType.hxx"

namespace simgear {

class HLAAbstractVariantDataElement : public HLADataElement {
public:
    HLAAbstractVariantDataElement(const HLAVariantDataType* dataType);
    virtual ~HLAAbstractVariantDataElement();

    virtual bool decode(HLADecodeStream& stream);
    virtual bool encode(HLAEncodeStream& stream) const;

    virtual const HLAVariantDataType* getDataType() const;
    virtual bool setDataType(const HLADataType* dataType);
    void setDataType(const HLAVariantDataType* dataType);

    std::string getAlternativeName() const;
    const HLADataType* getAlternativeDataType() const;

    virtual bool setAlternativeIndex(unsigned index) = 0;
    virtual bool decodeAlternative(HLADecodeStream& stream) = 0;
    virtual unsigned getAlternativeIndex() const = 0;
    virtual bool encodeAlternative(HLAEncodeStream& stream) const = 0;

private:
    SGSharedPtr<const HLAVariantDataType> _dataType;
};

class HLAVariantDataElement : public HLAAbstractVariantDataElement {
public:
    HLAVariantDataElement(const HLAVariantDataType* dataType);
    virtual ~HLAVariantDataElement();

    virtual bool setAlternativeIndex(unsigned index);
    virtual bool decodeAlternative(HLADecodeStream& stream);
    virtual unsigned getAlternativeIndex() const;
    virtual bool encodeAlternative(HLAEncodeStream& stream) const;

    class DataElementFactory : public SGReferenced {
    public:
        virtual ~DataElementFactory();
        virtual HLADataElement* createElement(const HLAVariantDataElement&, unsigned) = 0;
    };

    void setDataElementFactory(DataElementFactory* dataElementFactory);
    DataElementFactory* getDataElementFactory();

private:
    HLADataElement* newElement(unsigned index);

    SGSharedPtr<HLADataElement> _dataElement;
    unsigned _alternativeIndex;

    SGSharedPtr<DataElementFactory> _dataElementFactory;
};

}

#endif
