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

#ifndef HLAEnumeratedDataElement_hxx
#define HLAEnumeratedDataElement_hxx

#include "HLADataElement.hxx"
#include "HLAEnumeratedDataType.hxx"

namespace simgear {

class HLAAbstractEnumeratedDataElement : public HLADataElement {
public:
    HLAAbstractEnumeratedDataElement(const HLAEnumeratedDataType* dataType);
    virtual ~HLAAbstractEnumeratedDataElement();

    virtual void accept(HLADataElementVisitor& visitor);
    virtual void accept(HLAConstDataElementVisitor& visitor) const;

    virtual bool decode(HLADecodeStream& stream);
    virtual bool encode(HLAEncodeStream& stream) const;

    virtual const HLAEnumeratedDataType* getDataType() const;
    virtual bool setDataType(const HLADataType* dataType);
    void setDataType(const HLAEnumeratedDataType* dataType);

    const HLABasicDataType* getRepresentationDataType() const;

    std::string getStringRepresentation() const;
    bool setStringRepresentation(const std::string& name);

    virtual unsigned getEnumeratorIndex() const = 0;
    virtual void setEnumeratorIndex(unsigned index) = 0;

private:
    SGSharedPtr<const HLAEnumeratedDataType> _dataType;
};

class HLAEnumeratedDataElement : public HLAAbstractEnumeratedDataElement {
public:
    HLAEnumeratedDataElement(const HLAEnumeratedDataType* dataType);
    virtual ~HLAEnumeratedDataElement();

    virtual unsigned getEnumeratorIndex() const;
    virtual void setEnumeratorIndex(unsigned index);

private:
    unsigned _enumeratorIndex;
};

}

#endif
