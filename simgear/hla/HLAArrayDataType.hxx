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

#ifndef HLAArrayDataType_hxx
#define HLAArrayDataType_hxx

#include <string>
#include <simgear/structure/SGSharedPtr.hxx>
#include "HLADataType.hxx"

namespace simgear {

class HLAAbstractArrayDataElement;

class HLAArrayDataType : public HLADataType {
public:
    HLAArrayDataType(const std::string& name = "HLAArrayDataType");
    virtual ~HLAArrayDataType();

    virtual void accept(HLADataTypeVisitor& visitor) const;

    virtual const HLAArrayDataType* toArrayDataType() const;

    virtual void releaseDataTypeReferences();

    virtual bool decode(HLADecodeStream& stream, HLAAbstractArrayDataElement& value) const = 0;
    virtual bool encode(HLAEncodeStream& stream, const HLAAbstractArrayDataElement& value) const = 0;

    void setElementDataType(const HLADataType* elementDataType);
    const HLADataType* getElementDataType() const
    { return _elementDataType.get(); }

    void setIsOpaque(bool isOpaque);
    bool getIsOpaque() const
    { return _isOpaque; }

    void setIsString(bool isString);
    bool getIsString() const
    { return _isString; }

protected:
    virtual void _recomputeAlignmentImplementation();

private:
    SGSharedPtr<const HLADataType> _elementDataType;
    bool _isOpaque;
    bool _isString;
};

class HLAFixedArrayDataType : public HLAArrayDataType {
public:
    HLAFixedArrayDataType(const std::string& name = "HLAFixedArrayDataType");
    virtual ~HLAFixedArrayDataType();

    virtual void accept(HLADataTypeVisitor& visitor) const;

    virtual bool decode(HLADecodeStream& stream, HLAAbstractArrayDataElement& value) const;
    virtual bool encode(HLAEncodeStream& stream, const HLAAbstractArrayDataElement& value) const;

    void setNumElements(unsigned numElements)
    { _numElements = numElements; }
    unsigned getNumElements() const
    { return _numElements; }

private:
    unsigned _numElements;
};

class HLAVariableArrayDataType : public HLAArrayDataType {
public:
    HLAVariableArrayDataType(const std::string& name = "HLAVariableArrayDataType");
    virtual ~HLAVariableArrayDataType();

    virtual void accept(HLADataTypeVisitor& visitor) const;

    virtual bool decode(HLADecodeStream& stream, HLAAbstractArrayDataElement& value) const;
    virtual bool encode(HLAEncodeStream& stream, const HLAAbstractArrayDataElement& value) const;

    void setSizeDataType(const HLABasicDataType* sizeDataType);
    const HLABasicDataType* getSizeDataType() const
    { return _sizeDataType.get(); }

protected:
    virtual void _recomputeAlignmentImplementation();

private:
    SGSharedPtr<const HLABasicDataType> _sizeDataType;
};

} // namespace simgear

#endif
