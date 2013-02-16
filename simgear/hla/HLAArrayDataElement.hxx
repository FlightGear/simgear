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

#ifndef HLAArrayDataElement_hxx
#define HLAArrayDataElement_hxx

#include <string>
#include <vector>
#include <simgear/math/SGMath.hxx>
#include "HLAArrayDataType.hxx"
#include "HLADataElement.hxx"
#include "HLAVariantRecordDataElement.hxx"
#include "HLADataTypeVisitor.hxx"

namespace simgear {

class HLAAbstractArrayDataElement : public HLADataElement {
public:
    HLAAbstractArrayDataElement(const HLAArrayDataType* dataType);
    virtual ~HLAAbstractArrayDataElement();

    virtual void accept(HLADataElementVisitor& visitor);
    virtual void accept(HLAConstDataElementVisitor& visitor) const;

    virtual bool decode(HLADecodeStream& stream);
    virtual bool encode(HLAEncodeStream& stream) const;

    virtual const HLAArrayDataType* getDataType() const;
    virtual bool setDataType(const HLADataType* dataType);

    const HLADataType* getElementDataType() const;

    virtual bool setNumElements(unsigned count) = 0;
    virtual bool decodeElement(HLADecodeStream& stream, unsigned i) = 0;

    virtual unsigned getNumElements() const = 0;
    virtual bool encodeElement(HLAEncodeStream& stream, unsigned i) const = 0;

protected:
    SGSharedPtr<const HLAArrayDataType> _dataType;
};

class HLAArrayDataElement : public HLAAbstractArrayDataElement {
public:
    HLAArrayDataElement(const HLAArrayDataType* dataType = 0);
    virtual ~HLAArrayDataElement();

    virtual bool setDataElement(HLADataElementIndex::const_iterator begin, HLADataElementIndex::const_iterator end, HLADataElement* dataElement);
    virtual HLADataElement* getDataElement(HLADataElementIndex::const_iterator begin, HLADataElementIndex::const_iterator end);
    virtual const HLADataElement* getDataElement(HLADataElementIndex::const_iterator begin, HLADataElementIndex::const_iterator end) const;

    virtual bool setDataType(const HLADataType* dataType);

    virtual bool setNumElements(unsigned size);
    virtual bool decodeElement(HLADecodeStream& stream, unsigned i);
    virtual unsigned getNumElements() const;
    virtual bool encodeElement(HLAEncodeStream& stream, unsigned i) const;

    const HLADataElement* getElement(unsigned index) const;
    HLADataElement* getElement(unsigned index);
    HLADataElement* getOrCreateElement(unsigned index);
    void setElement(unsigned i, HLADataElement* value);

    class DataElementFactory : public SGReferenced {
    public:
        virtual ~DataElementFactory();
        virtual HLADataElement* createElement(const HLAArrayDataElement&, unsigned) = 0;
    };

    void setDataElementFactory(DataElementFactory* dataElementFactory);
    DataElementFactory* getDataElementFactory();

protected:
    virtual void _setStamp(Stamp* stamp);

private:
    HLADataElement* newElement(unsigned index);

    typedef std::vector<SGSharedPtr<HLADataElement> > ElementVector;
    ElementVector _elementVector;

    SGSharedPtr<DataElementFactory> _dataElementFactory;
};

// Holds an array of variants.
// Factors out common code for that use case.
class HLAVariantArrayDataElement : public HLAAbstractArrayDataElement {
public:
    HLAVariantArrayDataElement();
    virtual ~HLAVariantArrayDataElement();

    // Overwrite this from the abstract class, need some more checks here
    virtual bool setDataType(const HLADataType* dataType);

    virtual bool setNumElements(unsigned size);
    virtual bool decodeElement(HLADecodeStream& stream, unsigned i);
    virtual unsigned getNumElements() const;
    virtual bool encodeElement(HLAEncodeStream& stream, unsigned i) const;

    const HLAVariantRecordDataElement* getElement(unsigned index) const;
    HLAVariantRecordDataElement* getElement(unsigned index);
    HLAVariantRecordDataElement* getOrCreateElement(unsigned index);
    void setElement(unsigned index, HLAVariantRecordDataElement* value);

    typedef HLAVariantRecordDataElement::DataElementFactory AlternativeDataElementFactory;

    void setAlternativeDataElementFactory(AlternativeDataElementFactory* alternativeDataElementFactory);
    AlternativeDataElementFactory* getAlternativeDataElementFactory();

protected:
    virtual void _setStamp(Stamp* stamp);

private:
    HLAVariantRecordDataElement* newElement();

    typedef std::vector<SGSharedPtr<HLAVariantRecordDataElement> > ElementVector;
    ElementVector _elementVector;

    SGSharedPtr<AlternativeDataElementFactory> _alternativeDataElementFactory;
};

class HLAStringDataElement : public HLAAbstractArrayDataElement {
public:
    HLAStringDataElement(const HLAArrayDataType* dataType = 0) :
        HLAAbstractArrayDataElement(dataType)
    {}
    HLAStringDataElement(const HLAArrayDataType* dataType, const std::string& value) :
        HLAAbstractArrayDataElement(dataType),
        _value(value)
    {}
    const std::string& getValue() const
    { return _value; }
    void setValue(const std::string& value)
    { _value = value; setDirty(true); }

    virtual bool setNumElements(unsigned count)
    {
        _value.resize(count);
        return true;
    }
    virtual bool decodeElement(HLADecodeStream& stream, unsigned i)
    {
        HLATemplateDecodeVisitor<std::string::value_type> visitor(stream);
        getElementDataType()->accept(visitor);
        _value[i] = visitor.getValue();
        return true;
    }

    virtual unsigned getNumElements() const
    {
        return _value.size();
    }
    virtual bool encodeElement(HLAEncodeStream& stream, unsigned i) const
    {
        HLATemplateEncodeVisitor<std::string::value_type> visitor(stream, _value[i]);
        getElementDataType()->accept(visitor);
        return true;
    }

private:
    std::string _value;
};

class HLAStringData {
public:
    HLAStringData() :
        _value(new HLAStringDataElement(0))
    { }
    HLAStringData(const std::string& value) :
        _value(new HLAStringDataElement(0))
    { _value->setValue(value); }

    operator const std::string&() const
    { return _value->getValue(); }
    HLAStringData& operator=(const std::string& value)
    { _value->setValue(value); return *this; }

    const std::string& getValue() const
    { return _value->getValue(); }
    void setValue(const std::string& value)
    { _value->setValue(value); }

    const HLAStringDataElement* getDataElement() const
    { return _value.get(); }
    HLAStringDataElement* getDataElement()
    { return _value.get(); }

    const HLAArrayDataType* getDataType() const
    { return _value->getDataType(); }
    void setDataType(const HLAArrayDataType* dataType)
    { _value->setDataType(dataType); }

private:
    SGSharedPtr<HLAStringDataElement> _value;
};

template<typename T>
class HLAVec2DataElement : public HLAAbstractArrayDataElement {
public:
    HLAVec2DataElement(const HLAArrayDataType* dataType = 0) :
        HLAAbstractArrayDataElement(dataType),
        _value(SGVec2<T>::zeros())
    {}
    HLAVec2DataElement(const HLAArrayDataType* dataType, const SGVec2<T>& value) :
        HLAAbstractArrayDataElement(dataType),
        _value(value)
    {}
    const SGVec2<T>& getValue() const
    { return _value; }
    void setValue(const SGVec2<T>& value)
    { _value = value; setDirty(true); }

    virtual bool setNumElements(unsigned count)
    {
        for (unsigned i = count; i < 2; ++i)
            _value[i] = 0;
        return true;
    }
    virtual bool decodeElement(HLADecodeStream& stream, unsigned i)
    {
        if (i < 2) {
            HLATemplateDecodeVisitor<typename SGVec2<T>::value_type> visitor(stream);
            getElementDataType()->accept(visitor);
            _value[i] = visitor.getValue();
        } else {
            HLADataTypeDecodeVisitor visitor(stream);
            getElementDataType()->accept(visitor);
        }
        return true;
    }

    virtual unsigned getNumElements() const
    {
        return 2;
    }
    virtual bool encodeElement(HLAEncodeStream& stream, unsigned i) const
    {
        if (i < 2) {
            HLATemplateEncodeVisitor<typename SGVec2<T>::value_type> visitor(stream, _value[i]);
            getElementDataType()->accept(visitor);
        } else {
            HLADataTypeEncodeVisitor visitor(stream);
            getElementDataType()->accept(visitor);
        }
        return true;
    }

private:
    SGVec2<T> _value;
};

template<typename T>
class HLAVec2Data {
public:
    HLAVec2Data() :
        _value(new HLAVec2DataElement<T>(0))
    { }
    HLAVec2Data(const SGVec2<T>& value) :
        _value(new HLAVec2DataElement<T>(0, value))
    { }

    operator const SGVec2<T>&() const
    { return _value->getValue(); }
    HLAVec2Data& operator=(const SGVec2<T>& value)
    { _value->setValue(value); return *this; }

    const SGVec2<T>& getValue() const
    { return _value->getValue(); }
    void setValue(const SGVec2<T>& value)
    { _value->setValue(value); }

    const HLAVec2DataElement<T>* getDataElement() const
    { return _value.get(); }
    HLAVec2DataElement<T>* getDataElement()
    { return _value.get(); }

    const HLAArrayDataType* getDataType() const
    { return _value->getDataType(); }
    void setDataType(const HLAArrayDataType* dataType)
    { _value->setDataType(dataType); }

private:
    SGSharedPtr<HLAVec2DataElement<T> > _value;
};

typedef HLAVec2Data<float> HLAVec2fData;
typedef HLAVec2Data<double> HLAVec2dData;
typedef HLAVec2Data<int> HLAVec2iData;

template<typename T>
class HLAVec3DataElement : public HLAAbstractArrayDataElement {
public:
    HLAVec3DataElement(const HLAArrayDataType* dataType = 0) :
        HLAAbstractArrayDataElement(dataType),
        _value(SGVec3<T>::zeros())
    {}
    HLAVec3DataElement(const HLAArrayDataType* dataType, const SGVec3<T>& value) :
        HLAAbstractArrayDataElement(dataType),
        _value(value)
    {}
    const SGVec3<T>& getValue() const
    { return _value; }
    void setValue(const SGVec3<T>& value)
    { _value = value; setDirty(true); }

    virtual bool setNumElements(unsigned count)
    {
        for (unsigned i = count; i < 3; ++i)
            _value[i] = 0;
        return true;
    }
    virtual bool decodeElement(HLADecodeStream& stream, unsigned i)
    {
        if (i < 3) {
            HLATemplateDecodeVisitor<typename SGVec3<T>::value_type> visitor(stream);
            getElementDataType()->accept(visitor);
            _value[i] = visitor.getValue();
        } else {
            HLADataTypeDecodeVisitor visitor(stream);
            getElementDataType()->accept(visitor);
        }
        return true;
    }

    virtual unsigned getNumElements() const
    {
        return 3;
    }
    virtual bool encodeElement(HLAEncodeStream& stream, unsigned i) const
    {
        if (i < 3) {
            HLATemplateEncodeVisitor<typename SGVec3<T>::value_type> visitor(stream, _value[i]);
            getElementDataType()->accept(visitor);
        } else {
            HLADataTypeEncodeVisitor visitor(stream);
            getElementDataType()->accept(visitor);
        }
        return true;
    }

private:
    SGVec3<T> _value;
};

template<typename T>
class HLAVec3Data {
public:
    HLAVec3Data() :
        _value(new HLAVec3DataElement<T>(0))
    { }
    HLAVec3Data(const SGVec3<T>& value) :
        _value(new HLAVec3DataElement<T>(0, value))
    { }

    operator const SGVec3<T>&() const
    { return _value->getValue(); }
    HLAVec3Data& operator=(const SGVec3<T>& value)
    { _value->setValue(value); return *this; }

    const SGVec3<T>& getValue() const
    { return _value->getValue(); }
    void setValue(const SGVec3<T>& value)
    { _value->setValue(value); }

    const HLAVec3DataElement<T>* getDataElement() const
    { return _value.get(); }
    HLAVec3DataElement<T>* getDataElement()
    { return _value.get(); }

    const HLAArrayDataType* getDataType() const
    { return _value->getDataType(); }
    void setDataType(const HLAArrayDataType* dataType)
    { _value->setDataType(dataType); }

private:
    SGSharedPtr<HLAVec3DataElement<T> > _value;
};

typedef HLAVec3Data<float> HLAVec3fData;
typedef HLAVec3Data<double> HLAVec3dData;
typedef HLAVec3Data<int> HLAVec3iData;

template<typename T>
class HLAVec4DataElement : public HLAAbstractArrayDataElement {
public:
    HLAVec4DataElement(const HLAArrayDataType* dataType = 0) :
        HLAAbstractArrayDataElement(dataType),
        _value(SGVec4<T>::zeros())
    {}
    HLAVec4DataElement(const HLAArrayDataType* dataType, const SGVec4<T>& value) :
        HLAAbstractArrayDataElement(dataType),
        _value(value)
    {}
    const SGVec4<T>& getValue() const
    { return _value; }
    void setValue(const SGVec4<T>& value)
    { _value = value; setDirty(true); }

    virtual bool setNumElements(unsigned count)
    {
        for (unsigned i = count; i < 4; ++i)
            _value[i] = 0;
        return true;
    }
    virtual bool decodeElement(HLADecodeStream& stream, unsigned i)
    {
        if (i < 4) {
            HLATemplateDecodeVisitor<typename SGVec4<T>::value_type> visitor(stream);
            getElementDataType()->accept(visitor);
            _value[i] = visitor.getValue();
        } else {
            HLADataTypeDecodeVisitor visitor(stream);
            getElementDataType()->accept(visitor);
        }
        return true;
    }

    virtual unsigned getNumElements() const
    {
        return 4;
    }
    virtual bool encodeElement(HLAEncodeStream& stream, unsigned i) const
    {
        if (i < 4) {
            HLATemplateEncodeVisitor<typename SGVec4<T>::value_type> visitor(stream, _value[i]);
            getElementDataType()->accept(visitor);
        } else {
            HLADataTypeEncodeVisitor visitor(stream);
            getElementDataType()->accept(visitor);
        }
        return true;
    }

private:
    SGVec4<T> _value;
};

template<typename T>
class HLAVec4Data {
public:
    HLAVec4Data() :
        _value(new HLAVec4DataElement<T>(0))
    { }
    HLAVec4Data(const SGVec4<T>& value) :
        _value(new HLAVec4DataElement<T>(0, value))
    { }

    operator const SGVec4<T>&() const
    { return _value->getValue(); }
    HLAVec4Data& operator=(const SGVec4<T>& value)
    { _value->setValue(value); return *this; }

    const SGVec4<T>& getValue() const
    { return _value->getValue(); }
    void setValue(const SGVec4<T>& value)
    { _value->setValue(value); }

    const HLAVec4DataElement<T>* getDataElement() const
    { return _value.get(); }
    HLAVec4DataElement<T>* getDataElement()
    { return _value.get(); }

    const HLAArrayDataType* getDataType() const
    { return _value->getDataType(); }
    void setDataType(const HLAArrayDataType* dataType)
    { _value->setDataType(dataType); }

private:
    SGSharedPtr<HLAVec4DataElement<T> > _value;
};

typedef HLAVec4Data<float> HLAVec4fData;
typedef HLAVec4Data<double> HLAVec4dData;
typedef HLAVec4Data<int> HLAVec4iData;

template<typename T>
class HLAQuatDataElement : public HLAAbstractArrayDataElement {
public:
    HLAQuatDataElement(const HLAArrayDataType* dataType = 0) :
        HLAAbstractArrayDataElement(dataType),
        _value(SGQuat<T>::unit())
    {}
    HLAQuatDataElement(const HLAArrayDataType* dataType, const SGQuat<T>& value) :
        HLAAbstractArrayDataElement(dataType),
        _value(value)
    {}

    const SGQuat<T>& getValue() const
    { return _value; }
    void setValue(const SGQuat<T>& value)
    { _value = value; setDirty(true); }

    virtual bool setNumElements(unsigned count)
    {
        for (unsigned i = count; i < 4; ++i)
            _value[i] = 0;
        return true;
    }
    virtual bool decodeElement(HLADecodeStream& stream, unsigned i)
    {
        if (i < 4) {
            HLATemplateDecodeVisitor<typename SGQuat<T>::value_type> visitor(stream);
            getElementDataType()->accept(visitor);
            _value[i] = visitor.getValue();
        } else {
            HLADataTypeDecodeVisitor visitor(stream);
            getElementDataType()->accept(visitor);
        }
        return true;
    }

    virtual unsigned getNumElements() const
    {
        return 4;
    }
    virtual bool encodeElement(HLAEncodeStream& stream, unsigned i) const
    {
        if (i < 4) {
            HLATemplateEncodeVisitor<typename SGQuat<T>::value_type> visitor(stream, _value[i]);
            getElementDataType()->accept(visitor);
        } else {
            HLADataTypeEncodeVisitor visitor(stream);
            getElementDataType()->accept(visitor);
        }
        return true;
    }

private:
    SGQuat<T> _value;
};

template<typename T>
class HLAQuatData {
public:
    HLAQuatData() :
        _value(new HLAQuatDataElement<T>(0))
    { }
    HLAQuatData(const SGQuat<T>& value) :
        _value(new HLAQuatDataElement<T>(0, value))
    { }

    operator const SGQuat<T>&() const
    { return _value->getValue(); }
    HLAQuatData& operator=(const SGQuat<T>& value)
    { _value->setValue(value); return *this; }

    const SGQuat<T>& getValue() const
    { return _value->getValue(); }
    void setValue(const SGQuat<T>& value)
    { _value->setValue(value); }

    const HLAQuatDataElement<T>* getDataElement() const
    { return _value.get(); }
    HLAQuatDataElement<T>* getDataElement()
    { return _value.get(); }

    const HLAArrayDataType* getDataType() const
    { return _value->getDataType(); }
    void setDataType(const HLAArrayDataType* dataType)
    { _value->setDataType(dataType); }

private:
    SGSharedPtr<HLAQuatDataElement<T> > _value;
};

typedef HLAQuatData<float> HLAQuatfData;
typedef HLAQuatData<double> HLAQuatdData;

template<typename T>
class HLAQuat3DataElement : public HLAAbstractArrayDataElement {
public:
    HLAQuat3DataElement(const HLAArrayDataType* dataType = 0) :
        HLAAbstractArrayDataElement(dataType),
        _value(SGQuat<T>::unit()),
        _imag(SGQuat<T>::unit().getPositiveRealImag())
    {}
    HLAQuat3DataElement(const HLAArrayDataType* dataType, const SGQuat<T>& value) :
        HLAAbstractArrayDataElement(dataType),
        _value(value),
        _imag(value.getPositiveRealImag())
    {}

    const SGQuat<T>& getValue() const
    { return _value; }
    void setValue(const SGQuat<T>& value)
    { _value = value; _imag = _value.getPositiveRealImag(); setDirty(true); }

    virtual bool encode(HLAEncodeStream& stream) const
    {
        return HLAAbstractArrayDataElement::encode(stream);
    }
    virtual bool decode(HLADecodeStream& stream)
    {
        if (!HLAAbstractArrayDataElement::decode(stream))
            return false;
        _value = SGQuat<T>::fromPositiveRealImag(_imag);
        return true;
    }

    virtual bool setNumElements(unsigned count)
    {
        for (unsigned i = count; i < 3; ++i)
            _imag[i] = 0;
        return true;
    }
    virtual bool decodeElement(HLADecodeStream& stream, unsigned i)
    {
        if (i < 3) {
            HLATemplateDecodeVisitor<typename SGQuat<T>::value_type> visitor(stream);
            getElementDataType()->accept(visitor);
            _imag[i] = visitor.getValue();
        } else {
            HLADataTypeDecodeVisitor visitor(stream);
            getElementDataType()->accept(visitor);
        }
        return true;
    }

    virtual unsigned getNumElements() const
    {
        return 3;
    }
    virtual bool encodeElement(HLAEncodeStream& stream, unsigned i) const
    {
        if (i < 3) {
            HLATemplateEncodeVisitor<typename SGQuat<T>::value_type> visitor(stream, _imag[i]);
            getElementDataType()->accept(visitor);
        } else {
            HLADataTypeEncodeVisitor visitor(stream);
            getElementDataType()->accept(visitor);
        }
        return true;
    }

private:
    SGQuat<T> _value;
    SGVec3<T> _imag;
};

template<typename T>
class HLAQuat3Data {
public:
    HLAQuat3Data() :
        _value(new HLAQuat3DataElement<T>(0))
    { }
    HLAQuat3Data(const SGQuat<T>& value) :
        _value(new HLAQuat3DataElement<T>(0, value))
    { }

    operator const SGQuat<T>&() const
    { return _value->getValue(); }
    HLAQuat3Data& operator=(const SGQuat<T>& value)
    { _value->setValue(value); return *this; }

    const SGQuat<T>& getValue() const
    { return _value->getValue(); }
    void setValue(const SGQuat<T>& value)
    { _value->setValue(value); }

    const HLAQuat3DataElement<T>* getDataElement() const
    { return _value.get(); }
    HLAQuat3DataElement<T>* getDataElement()
    { return _value.get(); }

    const HLAArrayDataType* getDataType() const
    { return _value->getDataType(); }
    void setDataType(const HLAArrayDataType* dataType)
    { _value->setDataType(dataType); }

private:
    SGSharedPtr<HLAQuat3DataElement<T> > _value;
};

typedef HLAQuat3Data<float> HLAQuat3fData;
typedef HLAQuat3Data<double> HLAQuat3dData;

}

#endif
