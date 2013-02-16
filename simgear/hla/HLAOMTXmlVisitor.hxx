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

#ifndef HLAOMTXmlVisitor_hxx
#define HLAOMTXmlVisitor_hxx

#include <map>
#include <string>

#include <simgear/structure/exception.hxx>
#include <simgear/structure/SGSharedPtr.hxx>
#include <simgear/xml/easyxml.hxx>
#include "HLADataType.hxx"
#include "HLATypes.hxx"

namespace simgear {

class HLAFederate;

class HLAOMTXmlVisitor : public XMLVisitor {
public:
    /// structures representing the federate object model data
    struct Attribute : public SGReferenced {
        Attribute(const std::string& name) :
            _name(name)
        { }
        const std::string& getName() const
        { return _name; }
        const std::string& getDataType() const
        { return _dataType; }
        const std::string& getSharing() const
        { return _sharing; }
        const std::string& getDimensions() const
        { return _dimensions; }
        const std::string& getTransportation() const
        { return _transportation; }
        const std::string& getOrder() const
        { return _order; }

        HLASubscriptionType getSubscriptionType() const
        {
            if (_sharing.find("Subscribe") != std::string::npos)
                return HLASubscribedActive;
            else
                return HLAUnsubscribed;
        }

        HLAPublicationType getPublicationType() const
        {
            if (_sharing.find("Publish") != std::string::npos)
                return HLAPublished;
            else
                return HLAUnpublished;
        }

        HLAUpdateType getUpdateType() const
        {
            if (_updateType == "Periodic")
                return HLAPeriodicUpdate;
            else if (_updateType == "Static")
                return HLAStaticUpdate;
            else if (_updateType == "Conditional")
                return HLAConditionalUpdate;
            else
                return HLAUndefinedUpdate;
        }

        std::string _name;
        std::string _dataType;
        std::string _updateType;
        std::string _updateCondition;
        std::string _ownership;
        std::string _sharing;
        std::string _dimensions;
        std::string _transportation;
        std::string _order;
        friend class HLAOMTXmlVisitor;
    };
    typedef std::vector<SGSharedPtr<Attribute> > AttributeList;

    struct ObjectClass : public SGReferenced {
        ObjectClass(const std::string& name, const std::string& sharing);
        ~ObjectClass();

        const std::string& getName() const;
        const std::string& getSharing() const;

        unsigned getNumAttributes() const;
        const Attribute* getAttribute(unsigned index) const;

        const ObjectClass* getParentObjectClass() const;

    private:
        friend class HLAOMTXmlVisitor;
        std::string _name;
        std::string _sharing;
        AttributeList _attributes;
        SGSharedPtr<ObjectClass> _parentObjectClass;
    };
    typedef std::vector<SGSharedPtr<ObjectClass> > ObjectClassList;

    struct Parameter : public SGReferenced {
        Parameter(const std::string& name) :
            _name(name)
        { }
        const std::string& getName() const
        { return _name; }
        const std::string& getDataType() const
        { return _dataType; }

    private:
        std::string _name;
        std::string _dataType;
        friend class HLAOMTXmlVisitor;
    };
    typedef std::vector<SGSharedPtr<Parameter> > ParameterList;

    struct InteractionClass : public SGReferenced {
        InteractionClass(const std::string& name);
        ~InteractionClass();

        const std::string& getName() const;
        const std::string& getDimensions() const;
        const std::string& getSharing() const;
        const std::string& getTransportation() const;
        const std::string& getOrder() const;

        HLASubscriptionType getSubscriptionType() const
        {
            if (_sharing.find("Subscribe") != std::string::npos)
                return HLASubscribedActive;
            else
                return HLAUnsubscribed;
        }

        HLAPublicationType getPublicationType() const
        {
            if (_sharing.find("Publish") != std::string::npos)
                return HLAPublished;
            else
                return HLAUnpublished;
        }

        unsigned getNumParameters() const;
        const Parameter* getParameter(unsigned index) const;

        const InteractionClass* getParentInteractionClass() const;

    private:
        friend class HLAOMTXmlVisitor;
        std::string _name;
        std::string _dimensions;
        std::string _sharing;
        std::string _transportation;
        std::string _order;
        ParameterList _parameters;
        SGSharedPtr<InteractionClass> _parentInteractionClass;
    };
    typedef std::vector<SGSharedPtr<InteractionClass> > InteractionClassList;

    HLAOMTXmlVisitor();
    ~HLAOMTXmlVisitor();

    void setDataTypesToFederate(HLAFederate& federate);
    void setToFederate(HLAFederate& federate);

    unsigned getNumObjectClasses() const;
    const ObjectClass* getObjectClass(unsigned i) const;

    unsigned getNumInteractionClasses() const;
    const InteractionClass* getInteractionClass(unsigned i) const;

private:
    SGSharedPtr<HLADataType> getDataType(const std::string& dataTypeName);
    SGSharedPtr<HLABasicDataType> getBasicDataType(const std::string& dataTypeName);
    SGSharedPtr<HLADataType> getSimpleDataType(const std::string& dataTypeName);
    SGSharedPtr<HLAEnumeratedDataType> getEnumeratedDataType(const std::string& dataTypeName);
    SGSharedPtr<HLADataType> getArrayDataType(const std::string& dataTypeName);
    SGSharedPtr<HLAFixedRecordDataType> getFixedRecordDataType(const std::string& dataTypeName);
    SGSharedPtr<HLAVariantRecordDataType> getVariantRecordDataType(const std::string& dataTypeName);

    enum Mode {
        UnknownMode,

        ObjectModelMode,

        ObjectsMode,
        ObjectClassMode,
        AttributeMode,

        InteractionsMode,
        InteractionClassMode,
        ParameterMode,

        DataTypesMode,
        BasicDataRepresentationsMode,
        BasicDataMode,
        SimpleDataTypesMode,
        SimpleDataMode,
        EnumeratedDataTypesMode,
        EnumeratedDataMode,
        EnumeratorMode,
        ArrayDataTypesMode,
        ArrayDataMode,
        FixedRecordDataTypesMode,
        FixedRecordDataMode,
        FieldMode,
        VariantRecordDataTypesMode,
        VariantRecordDataMode,
        AlternativeDataMode
    };

    Mode getCurrentMode();
    void pushMode(Mode mode);
    void popMode();

    virtual void startXML();
    virtual void endXML ();
    virtual void startElement(const char* name, const XMLAttributes& atts);
    virtual void endElement(const char* name);

    static std::string getAttribute(const char* name, const XMLAttributes& atts);
    static std::string getAttribute(const std::string& name, const XMLAttributes& atts);

    struct BasicData {
        // std::string _name;
        std::string _size;
        std::string _endian;
    };
    typedef std::map<std::string, BasicData> BasicDataMap;

    struct SimpleData {
        // std::string _name;
        std::string _representation;
        std::string _units;
        std::string _resolution;
        std::string _accuracy;
    };
    typedef std::map<std::string, SimpleData> SimpleDataMap;

    struct Enumerator {
        std::string _name;
        std::string _values;
    };
    typedef std::vector<Enumerator> EnumeratorList;

    struct EnumeratedData {
        // std::string _name;
        std::string _representation;
        EnumeratorList _enumeratorList;
    };
    typedef std::map<std::string, EnumeratedData> EnumeratedDataMap;

    struct ArrayData {
        // std::string _name;
        std::string _dataType;
        std::string _cardinality;
        std::string _encoding;
    };
    typedef std::map<std::string, ArrayData> ArrayDataMap;

    struct Field {
        std::string _name;
        std::string _dataType;
    };
    typedef std::vector<Field> FieldList;

    struct FixedRecordData {
        // std::string _name;
        std::string _encoding;
        FieldList _fieldList;
    };
    typedef std::map<std::string, FixedRecordData> FixedRecordDataMap;

    struct Alternative {
        std::string _name;
        std::string _dataType;
        std::string _semantics;
        std::string _enumerator;
    };
    typedef std::vector<Alternative> AlternativeList;

    struct VariantRecordData {
        // std::string _name;
        std::string _encoding;
        std::string _dataType;
        std::string _discriminant;
        std::string _semantics;
        AlternativeList _alternativeList;
    };
    typedef std::map<std::string, VariantRecordData> VariantRecordDataMap;

    std::vector<Mode> _modeStack;

    /// The total list of object classes
    ObjectClassList _objectClassList;
    ObjectClassList _objectClassStack;

    /// The total list of interaction classes
    InteractionClassList _interactionClassList;
    InteractionClassList _interactionClassStack;

    typedef std::map<std::string, SGSharedPtr<HLADataType> > StringDataTypeMap;
    StringDataTypeMap _dataTypeMap;

    /// DataType definitions
    BasicDataMap _basicDataMap;
    SimpleDataMap _simpleDataMap;
    std::string _enumeratedDataName;
    EnumeratedDataMap _enumeratedDataMap;
    ArrayDataMap _arrayDataMap;
    std::string _fixedRecordDataName;
    FixedRecordDataMap _fixedRecordDataMap;
    std::string _variantRecordDataName;
    VariantRecordDataMap _variantRecordDataMap;
};

} // namespace simgear

#endif
