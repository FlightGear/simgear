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

#include "HLAOMTXmlVisitor.hxx"

#include <map>
#include <string>
#include <sstream>

#include <simgear/structure/exception.hxx>
#include <simgear/xml/easyxml.hxx>
#include "HLAArrayDataType.hxx"
#include "HLABasicDataType.hxx"
#include "HLADataTypeVisitor.hxx"
#include "HLAEnumeratedDataType.hxx"
#include "HLAFixedRecordDataType.hxx"
#include "HLAVariantRecordDataType.hxx"

namespace simgear {

HLAOMTXmlVisitor::ObjectClass::ObjectClass(const std::string& name, const std::string& sharing) :
    _name(name),
    _sharing(sharing)
{
}

HLAOMTXmlVisitor::ObjectClass::~ObjectClass()
{
}

const std::string&
HLAOMTXmlVisitor::ObjectClass::getName() const
{
    return _name;
}

const std::string&
HLAOMTXmlVisitor::ObjectClass::getSharing() const
{
    return _sharing;
}

unsigned
HLAOMTXmlVisitor::ObjectClass::getNumAttributes() const
{
    return _attributes.size();
}

const HLAOMTXmlVisitor::Attribute*
HLAOMTXmlVisitor::ObjectClass::getAttribute(unsigned index) const
{
    if (_attributes.size() <= index)
        return 0;
    return _attributes[index];
}

const HLAOMTXmlVisitor::Attribute*
HLAOMTXmlVisitor::ObjectClass::getAttribute(const std::string& name) const
{
    for (AttributeList::const_iterator i = _attributes.begin(); i != _attributes.end(); ++i) {
        if ((*i)->_name != name)
            continue;
        return i->get();
    }
    SG_LOG(SG_IO, SG_ALERT, "Could not find class attribute \"" << name << "\".");
    return 0;
}

const HLAOMTXmlVisitor::ObjectClass*
HLAOMTXmlVisitor::ObjectClass::getParentObjectClass() const
{
    return _parentObjectClass.get();
}

HLAOMTXmlVisitor::InteractionClass::InteractionClass(const std::string& name) :
    _name(name)
{
}

HLAOMTXmlVisitor::InteractionClass::~InteractionClass()
{
}

const std::string&
HLAOMTXmlVisitor::InteractionClass::getName() const
{
    return _name;
}

const std::string&
HLAOMTXmlVisitor::InteractionClass::getDimensions() const
{
    return _dimensions;
}

const std::string&
HLAOMTXmlVisitor::InteractionClass::getTransportation() const
{
    return _transportation;
}

const std::string&
HLAOMTXmlVisitor::InteractionClass::getOrder() const
{
    return _order;
}

unsigned
HLAOMTXmlVisitor::InteractionClass::getNumParameters() const
{
    return _parameters.size();
}

const HLAOMTXmlVisitor::Parameter*
HLAOMTXmlVisitor::InteractionClass::getParameter(unsigned index) const
{
    if (_parameters.size() <= index)
        return 0;
    return _parameters[index];
}

const HLAOMTXmlVisitor::Parameter*
HLAOMTXmlVisitor::InteractionClass::getParameter(const std::string& name) const
{
    for (ParameterList::const_iterator i = _parameters.begin(); i != _parameters.end(); ++i) {
        if ((*i)->_name != name)
            continue;
        return i->get();
    }
    SG_LOG(SG_IO, SG_ALERT, "Could not find parameter \"" << name << "\".");
    return 0;
}

const HLAOMTXmlVisitor::InteractionClass*
HLAOMTXmlVisitor::InteractionClass::getParentInteractionClass() const
{
    return _parentInteractionClass.get();
}

HLAOMTXmlVisitor::HLAOMTXmlVisitor()
{
}

HLAOMTXmlVisitor::~HLAOMTXmlVisitor()
{
}

unsigned
HLAOMTXmlVisitor::getNumObjectClasses() const
{
    return _objectClassList.size();
}

const HLAOMTXmlVisitor::ObjectClass*
HLAOMTXmlVisitor::getObjectClass(unsigned i) const
{
    if (_objectClassList.size() <= i)
        return 0;
    return _objectClassList[i];
}

const HLAOMTXmlVisitor::ObjectClass*
HLAOMTXmlVisitor::getObjectClass(const std::string& name) const
{
    for (ObjectClassList::const_iterator i = _objectClassList.begin(); i != _objectClassList.end(); ++i) {
        if ((*i)->_name != name)
            continue;
        return i->get();
    }
    SG_LOG(SG_IO, SG_ALERT, "Could not resolve ObjectClass \"" << name << "\".");
    return 0;
}

const HLAOMTXmlVisitor::Attribute*
HLAOMTXmlVisitor::getAttribute(const std::string& objectClassName, const std::string& attributeName) const
{
    const ObjectClass* objectClass = getObjectClass(objectClassName);
    if (!objectClass)
        return 0;
    return objectClass->getAttribute(attributeName);
}

HLADataType*
HLAOMTXmlVisitor::getAttributeDataType(const std::string& objectClassName, const std::string& attributeName) const
{
    const Attribute* attribute = getAttribute(objectClassName, attributeName);
    if (!attribute)
        return 0;
    return getDataType(attribute->_dataType);
}

unsigned
HLAOMTXmlVisitor::getNumInteractionClasses() const
{
    return _interactionClassList.size();
}

const HLAOMTXmlVisitor::InteractionClass*
HLAOMTXmlVisitor::getInteractionClass(unsigned i) const
{
    if (_interactionClassList.size() <= i)
        return 0;
    return _interactionClassList[i];
}

const HLAOMTXmlVisitor::InteractionClass*
HLAOMTXmlVisitor::getInteractionClass(const std::string& name) const
{
    for (InteractionClassList::const_iterator i = _interactionClassList.begin(); i != _interactionClassList.end(); ++i) {
        if ((*i)->_name != name)
            continue;
        return i->get();
    }
    SG_LOG(SG_IO, SG_ALERT, "Could not resolve InteractionClass \"" << name << "\".");
    return 0;
}

const HLAOMTXmlVisitor::Parameter*
HLAOMTXmlVisitor::getParameter(const std::string& interactionClassName, const std::string& parameterName) const
{
    const InteractionClass* interactionClass = getInteractionClass(interactionClassName);
    if (!interactionClass)
        return 0;
    return interactionClass->getParameter(parameterName);
}

HLADataType*
HLAOMTXmlVisitor::getParameterDataType(const std::string& interactionClassName, const std::string& parameterName) const
{
    const Parameter* parameter = getParameter(interactionClassName, parameterName);
    if (!parameter)
        return 0;
    return getDataType(parameter->_dataType);
}

HLADataType*
HLAOMTXmlVisitor::getDataType(const std::string& dataTypeName) const
{
    SGSharedPtr<HLADataType> dataType;
    {
        // Playing dirty things with reference counts
        StringDataTypeMap dataTypeMap;
        dataType = getDataType(dataTypeName, dataTypeMap);
    }
    return dataType.release();
}

SGSharedPtr<HLADataType>
HLAOMTXmlVisitor::getDataType(const std::string& dataTypeName, HLAOMTXmlVisitor::StringDataTypeMap& dataTypeMap) const
{
    StringDataTypeMap::const_iterator i = dataTypeMap.find(dataTypeName);
    if (i != dataTypeMap.end())
        return new HLADataTypeReference(i->second);

    SGSharedPtr<HLADataType> dataType;
    dataType = getBasicDataType(dataTypeName);
    if (dataType.valid())
        return dataType;

    dataType = getSimpleDataType(dataTypeName);
    if (dataType.valid())
        return dataType;

    dataType = getEnumeratedDataType(dataTypeName);
    if (dataType.valid())
        return dataType;

    dataType = getArrayDataType(dataTypeName, dataTypeMap);
    if (dataType.valid())
        return dataType;

    dataType = getFixedRecordDataType(dataTypeName, dataTypeMap);
    if (dataType.valid())
        return dataType;

    dataType = getVariantRecordDataType(dataTypeName, dataTypeMap);
    if (dataType.valid())
        return dataType;

    SG_LOG(SG_IO, SG_WARN, "Could not resolve dataType \"" << dataTypeName << "\".");
    return 0;
}

SGSharedPtr<HLABasicDataType>
HLAOMTXmlVisitor::getBasicDataType(const std::string& dataTypeName) const
{
    BasicDataMap::const_iterator i = _basicDataMap.find(dataTypeName);
    if (i == _basicDataMap.end())
        return 0;
    if (i->second._size == "8") {
        return new HLAUInt8DataType(dataTypeName);
    } else if (i->second._size == "16") {
        if (i->first.find("Unsigned") != std::string::npos) {
            if (i->second._endian == "Little") {
                return new HLAUInt16LEDataType(dataTypeName);
            } else {
                return new HLAUInt16BEDataType(dataTypeName);
            }
        } else if (i->first.find("octetPair") != std::string::npos) {
            if (i->second._endian == "Little") {
                return new HLAUInt16LEDataType(dataTypeName);
            } else {
                return new HLAUInt16BEDataType(dataTypeName);
            }
        } else {
            if (i->second._endian == "Little") {
                return new HLAInt16LEDataType(dataTypeName);
            } else {
                return new HLAInt16BEDataType(dataTypeName);
            }
        }
    } else if (i->second._size == "32") {
        if (i->first.find("Unsigned") != std::string::npos) {
            if (i->second._endian == "Little") {
                return new HLAUInt32LEDataType(dataTypeName);
            } else {
                return new HLAUInt32BEDataType(dataTypeName);
            }
        } else if (i->first.find("float") != std::string::npos) {
            if (i->second._endian == "Little") {
                return new HLAFloat32LEDataType(dataTypeName);
            } else {
                return new HLAFloat32BEDataType(dataTypeName);
            }
        } else {
            if (i->second._endian == "Little") {
                return new HLAInt32LEDataType(dataTypeName);
            } else {
                return new HLAInt32BEDataType(dataTypeName);
            }
        }
    } else if (i->second._size == "64") {
        if (i->first.find("Unsigned") != std::string::npos) {
            if (i->second._endian == "Little") {
                return new HLAUInt64LEDataType(dataTypeName);
            } else {
                return new HLAUInt64BEDataType(dataTypeName);
            }
        } else if (i->first.find("float") != std::string::npos) {
            if (i->second._endian == "Little") {
                return new HLAFloat64LEDataType(dataTypeName);
            } else {
                return new HLAFloat64BEDataType(dataTypeName);
            }
        } else {
            if (i->second._endian == "Little") {
                return new HLAInt64LEDataType(dataTypeName);
            } else {
                return new HLAInt64BEDataType(dataTypeName);
            }
        }
    }

    return 0;
}

SGSharedPtr<HLADataType>
HLAOMTXmlVisitor::getSimpleDataType(const std::string& dataTypeName) const
{
    SimpleDataMap::const_iterator i = _simpleDataMap.find(dataTypeName);
    if (i == _simpleDataMap.end())
        return 0;
    return getDataType(i->second._representation);
}

SGSharedPtr<HLAEnumeratedDataType>
HLAOMTXmlVisitor::getEnumeratedDataType(const std::string& dataTypeName) const
{
    EnumeratedDataMap::const_iterator i = _enumeratedDataMap.find(dataTypeName);
    if (i == _enumeratedDataMap.end())
        return 0;

    SGSharedPtr<HLAEnumeratedDataType> enumeratedDataType = new HLAEnumeratedDataType(dataTypeName);
    enumeratedDataType->setRepresentation(getBasicDataType(i->second._representation));

    for (EnumeratorList::const_iterator j = i->second._enumeratorList.begin();
         j != i->second._enumeratorList.end(); ++j) {
        if (!enumeratedDataType->addEnumerator(j->_name, j->_values)) {
            SG_LOG(SG_IO, SG_ALERT, "Could not add enumerator \"" << j->_name
                   << "\" to find enumerated data type \"" << dataTypeName << "\".");
            return 0;
        }
    }

    return enumeratedDataType;
}

SGSharedPtr<HLADataType>
HLAOMTXmlVisitor::getArrayDataType(const std::string& dataTypeName, HLAOMTXmlVisitor::StringDataTypeMap& dataTypeMap) const
{
    ArrayDataMap::const_iterator i = _arrayDataMap.find(dataTypeName);
    if (i == _arrayDataMap.end())
        return 0;
    SGSharedPtr<HLAArrayDataType> arrayDataType;
    if (i->second._encoding == "HLAvariableArray") {
        arrayDataType = new HLAVariableArrayDataType(dataTypeName);
    } else if (i->second._encoding == "HLAfixedArray") {
        std::stringstream ss(i->second._cardinality);
        unsigned cardinality;
        ss >> cardinality;
        if (ss.fail()) {
            SG_LOG(SG_IO, SG_ALERT, "Could not interpret cardinality \""
                   << i->second._cardinality << "\" for dataType \""
                   << dataTypeName << "\".");
            return 0;
        }
        SGSharedPtr<HLAFixedArrayDataType> dataType = new HLAFixedArrayDataType(dataTypeName);
        dataType->setNumElements(cardinality);
        arrayDataType = dataType;
    } else {
        SG_LOG(SG_IO, SG_ALERT, "Can not interpret encoding \""
               << i->second._encoding << "\" for dataType \""
               << dataTypeName << "\".");
        return 0;
    }

    dataTypeMap[dataTypeName] = arrayDataType;
    SGSharedPtr<HLADataType> elementDataType = getDataType(i->second._dataType, dataTypeMap);
    if (!elementDataType.valid()) {
        SG_LOG(SG_IO, SG_ALERT, "Could not interpret dataType \""
               << i->second._dataType << "\" for array data type \""
               << dataTypeName << "\".");
        dataTypeMap.erase(dataTypeName);
        return 0;
    }
    arrayDataType->setElementDataType(elementDataType.get());

    // Check if this should be a string data type
    if (elementDataType->toBasicDataType()) {
        if (dataTypeName == "HLAopaqueData") {
            arrayDataType->setIsOpaque(true);
        } else if (dataTypeName.find("String") != std::string::npos || dataTypeName.find("string") != std::string::npos) {
            arrayDataType->setIsString(true);
        }
    }

    return arrayDataType;
}

SGSharedPtr<HLAFixedRecordDataType>
HLAOMTXmlVisitor::getFixedRecordDataType(const std::string& dataTypeName, HLAOMTXmlVisitor::StringDataTypeMap& dataTypeMap) const
{
    FixedRecordDataMap::const_iterator i = _fixedRecordDataMap.find(dataTypeName);
    if (i == _fixedRecordDataMap.end())
        return 0;

    SGSharedPtr<HLAFixedRecordDataType> dataType = new HLAFixedRecordDataType(dataTypeName);
    dataTypeMap[dataTypeName] = dataType;
    for (FieldList::size_type j = 0; j < i->second._fieldList.size(); ++j) {
        SGSharedPtr<HLADataType> fieldDataType = getDataType(i->second._fieldList[j]._dataType, dataTypeMap);
        if (!fieldDataType.valid()) {
            SG_LOG(SG_IO, SG_ALERT, "Could not get data type \"" << i->second._fieldList[j]._dataType
                   << "\" for field " << j << "of fixed record data type \"" << dataTypeName << "\".");
            dataTypeMap.erase(dataTypeName);
            return 0;
        }
        dataType->addField(i->second._fieldList[j]._name, fieldDataType.get());
    }
    return dataType;
}

SGSharedPtr<HLAVariantRecordDataType>
HLAOMTXmlVisitor::getVariantRecordDataType(const std::string& dataTypeName, HLAOMTXmlVisitor::StringDataTypeMap& dataTypeMap) const
{
    VariantRecordDataMap::const_iterator i = _variantRecordDataMap.find(dataTypeName);
    if (i == _variantRecordDataMap.end())
        return 0;
    SGSharedPtr<HLAVariantRecordDataType> dataType = new HLAVariantRecordDataType(dataTypeName);
    dataTypeMap[dataTypeName] = dataType;

    SGSharedPtr<HLAEnumeratedDataType> enumeratedDataType = getEnumeratedDataType(i->second._dataType);
    if (!enumeratedDataType.valid()) {
        SG_LOG(SG_IO, SG_ALERT, "Could not find enumerted data type \"" << i->second._dataType
               << "\" for variant data type \"" << dataTypeName << "\".");
        return 0;
    }
    dataType->setEnumeratedDataType(enumeratedDataType);

    for (AlternativeList::const_iterator j = i->second._alternativeList.begin();
         j != i->second._alternativeList.end(); ++j) {
        SGSharedPtr<HLADataType> alternativeDataType = getDataType(j->_dataType, dataTypeMap);
        if (!alternativeDataType.valid()) {
            SG_LOG(SG_IO, SG_ALERT, "Could not resolve alternative dataType \"" << j->_dataType
                   << "\" for alternative \"" << j->_name << "\".");
            dataTypeMap.erase(dataTypeName);
            return 0;
        }
        if (!dataType->addAlternative(j->_name, j->_enumerator, alternativeDataType.get(), j->_semantics)) {
            SG_LOG(SG_IO, SG_ALERT, "Could not add alternative \"" << j->_name << "\".");
            return 0;
        }
    }
    return dataType;
}

HLAOMTXmlVisitor::Mode
HLAOMTXmlVisitor::getCurrentMode()
{
    if (_modeStack.empty())
        return UnknownMode;
    return _modeStack.back();
}

void
HLAOMTXmlVisitor::pushMode(HLAOMTXmlVisitor::Mode mode)
{
    _modeStack.push_back(mode);
}

void
HLAOMTXmlVisitor::popMode()
{
    _modeStack.pop_back();
}

void
HLAOMTXmlVisitor::startXML()
{
    _modeStack.clear();
}

void
HLAOMTXmlVisitor::endXML()
{
    if (!_modeStack.empty())
        throw sg_exception("Internal parse error!");

    // propagate parent attributes to the derived classes
    for (ObjectClassList::const_iterator i = _objectClassList.begin(); i != _objectClassList.end(); ++i) {
        SGSharedPtr<const ObjectClass> objectClass = (*i)->_parentObjectClass;
        while (objectClass) {
            for (AttributeList::const_reverse_iterator j = objectClass->_attributes.rbegin();
                 j != objectClass->_attributes.rend(); ++j) {
                (*i)->_attributes.insert((*i)->_attributes.begin(), *j);
            }
            objectClass = objectClass->_parentObjectClass;
        }
    }

    // propagate parent parameter to the derived interactions
    for (InteractionClassList::const_iterator i = _interactionClassList.begin(); i != _interactionClassList.end(); ++i) {
        SGSharedPtr<const InteractionClass> interactionClass = (*i)->_parentInteractionClass;
        while (interactionClass) {
            for (ParameterList::const_reverse_iterator j = interactionClass->_parameters.rbegin();
                 j != interactionClass->_parameters.rend(); ++j) {
                (*i)->_parameters.insert((*i)->_parameters.begin(), *j);
            }
            interactionClass = interactionClass->_parentInteractionClass;
        }
    }
}

void
HLAOMTXmlVisitor::startElement(const char* name, const XMLAttributes& atts)
{
    if (strcmp(name, "attribute") == 0) {
        if (getCurrentMode() != ObjectClassMode)
            throw sg_exception("attribute tag outside objectClass!");
        pushMode(AttributeMode);

        if (_objectClassList.empty())
            throw sg_exception("attribute tag outside of an objectClass");

        std::string name = getAttribute("name", atts);
        if (name.empty())
            throw sg_exception("attribute tag without name attribute");

        SGSharedPtr<Attribute> attribute = new Attribute(name);

        attribute->_dataType = getAttribute("dataType", atts);
        attribute->_updateType = getAttribute("updateType", atts);
        attribute->_updateCondition = getAttribute("updateCondition", atts);
        attribute->_ownership = getAttribute("ownership", atts);
        attribute->_sharing = getAttribute("sharing", atts);
        attribute->_dimensions = getAttribute("dimensions", atts);
        attribute->_transportation = getAttribute("transportation", atts);
        attribute->_order = getAttribute("order", atts);

        _objectClassStack.back()->_attributes.push_back(attribute);

    } else if (strcmp(name, "objectClass") == 0) {
        if (getCurrentMode() != ObjectsMode && getCurrentMode() != ObjectClassMode)
            throw sg_exception("objectClass tag outside objectClass or objects!");
        pushMode(ObjectClassMode);

        std::string name = getAttribute("name", atts);
        if (name.empty())
            throw sg_exception("objectClass tag without name attribute");

        std::string sharing = getAttribute("sharing", atts);

        // The new ObjectClass
        ObjectClass* objectClass = new ObjectClass(name, sharing);

        // Inherit all previous attributes
        if (!_objectClassStack.empty())
            objectClass->_parentObjectClass = _objectClassStack.back();

        _objectClassStack.push_back(objectClass);
        _objectClassList.push_back(objectClass);

    } else if (strcmp(name, "objects") == 0) {
        if (getCurrentMode() != ObjectModelMode)
            throw sg_exception("objects tag outside objectModel!");
        pushMode(ObjectsMode);

    } else if (strcmp(name, "parameter") == 0) {
        if (getCurrentMode() != InteractionClassMode)
            throw sg_exception("parameter tag outside interactionClass!");
        pushMode(ParameterMode);

        if (_interactionClassList.empty())
            throw sg_exception("parameter tag outside of an interactionClass");

        std::string name = getAttribute("name", atts);
        if (name.empty())
            throw sg_exception("parameter tag without name parameter");

        SGSharedPtr<Parameter> parameter = new Parameter(name);
        parameter->_dataType = getAttribute("dataType", atts);

        _interactionClassStack.back()->_parameters.push_back(parameter);

    } else if (strcmp(name, "interactionClass") == 0) {
        if (getCurrentMode() != InteractionsMode && getCurrentMode() != InteractionClassMode)
            throw sg_exception("interactionClass tag outside interactions or interactionClass!");
        pushMode(InteractionClassMode);

        std::string name = getAttribute("name", atts);
        if (name.empty())
            throw sg_exception("interactionClass tag without name attribute");

        // The new ObjectClass
        InteractionClass* interactionClass = new InteractionClass(name);
        interactionClass->_dimensions = getAttribute("dimensions", atts);
        interactionClass->_transportation = getAttribute("transportation", atts);
        interactionClass->_order = getAttribute("order", atts);

        // Inherit all previous attributes
        if (!_interactionClassStack.empty())
            interactionClass->_parentInteractionClass = _interactionClassStack.back();

        _interactionClassStack.push_back(interactionClass);
        _interactionClassList.push_back(interactionClass);

    } else if (strcmp(name, "interactions") == 0) {
        if (getCurrentMode() != ObjectModelMode)
            throw sg_exception("interactions tag outside objectModel!");
        pushMode(InteractionsMode);

    } else if (strcmp(name, "basicData") == 0) {
        if (getCurrentMode() != BasicDataRepresentationsMode)
            throw sg_exception("basicData tag outside basicDataRepresentations!");
        pushMode(BasicDataMode);

        std::string name = getAttribute("name", atts);
        if (name.empty())
            throw sg_exception("basicData tag without name attribute");

        _basicDataMap[name]._size = getAttribute("size", atts);
        _basicDataMap[name]._endian = getAttribute("endian", atts);

    } else if (strcmp(name, "basicDataRepresentations") == 0) {
        if (getCurrentMode() != DataTypesMode)
            throw sg_exception("basicDataRepresentations tag outside dataTypes!");
        pushMode(BasicDataRepresentationsMode);

    } else if (strcmp(name, "simpleData") == 0) {
        if (getCurrentMode() != SimpleDataTypesMode)
            throw sg_exception("simpleData tag outside simpleDataTypes!");
        pushMode(SimpleDataMode);

        std::string name = getAttribute("name", atts);
        if (name.empty())
            throw sg_exception("simpleData tag without name attribute");

        _simpleDataMap[name]._representation = getAttribute("representation", atts);
        _simpleDataMap[name]._units = getAttribute("units", atts);
        _simpleDataMap[name]._resolution = getAttribute("resolution", atts);
        _simpleDataMap[name]._accuracy = getAttribute("accuracy", atts);

    } else if (strcmp(name, "simpleDataTypes") == 0) {
        if (getCurrentMode() != DataTypesMode)
            throw sg_exception("simpleDataTypes tag outside dataTypes!");
        pushMode(SimpleDataTypesMode);

    } else if (strcmp(name, "enumerator") == 0) {
        if (getCurrentMode() != EnumeratedDataMode)
            throw sg_exception("enumerator tag outside enumeratedData!");
        pushMode(EnumeratorMode);

        std::string name = getAttribute("name", atts);
        if (name.empty())
            throw sg_exception("enumerator tag without name attribute");

        Enumerator enumerator;
        enumerator._name = name;
        enumerator._values = getAttribute("values", atts);
        _enumeratedDataMap[_enumeratedDataName]._enumeratorList.push_back(enumerator);

    } else if (strcmp(name, "enumeratedData") == 0) {
        if (getCurrentMode() != EnumeratedDataTypesMode)
            throw sg_exception("enumeratedData tag outside enumeratedDataTypes!");
        pushMode(EnumeratedDataMode);

        std::string name = getAttribute("name", atts);
        if (name.empty())
            throw sg_exception("enumeratedData tag without name attribute");

        _enumeratedDataName = name;
        _enumeratedDataMap[_enumeratedDataName]._representation = getAttribute("representation", atts);

    } else if (strcmp(name, "enumeratedDataTypes") == 0) {
        if (getCurrentMode() != DataTypesMode)
            throw sg_exception("enumeratedDataTypes tag outside dataTypes!");
        pushMode(EnumeratedDataTypesMode);

        Enumerator enumerator;
        enumerator._name = getAttribute("name", atts);
        enumerator._values = getAttribute("values", atts);
        _enumeratedDataMap[_enumeratedDataName]._enumeratorList.push_back(enumerator);

    } else if (strcmp(name, "arrayData") == 0) {
        if (getCurrentMode() != ArrayDataTypesMode)
            throw sg_exception("arrayData tag outside arrayDataTypes!");
        pushMode(ArrayDataMode);

        std::string name = getAttribute("name", atts);
        if (name.empty())
            throw sg_exception("arrayData tag without name attribute");

        _arrayDataMap[name]._dataType = getAttribute("dataType", atts);
        _arrayDataMap[name]._cardinality = getAttribute("cardinality", atts);
        _arrayDataMap[name]._encoding = getAttribute("encoding", atts);

    } else if (strcmp(name, "arrayDataTypes") == 0) {
        if (getCurrentMode() != DataTypesMode)
            throw sg_exception("arrayDataTypes tag outside dataTypes!");
        pushMode(ArrayDataTypesMode);

    } else if (strcmp(name, "field") == 0) {
        if (getCurrentMode() != FixedRecordDataMode)
            throw sg_exception("field tag outside fixedRecordData!");
        pushMode(FieldMode);

        std::string name = getAttribute("name", atts);
        if (name.empty())
            throw sg_exception("field tag without name attribute");

        Field field;
        field._name = name;
        field._dataType = getAttribute("dataType", atts);
        _fixedRecordDataMap[_fixedRecordDataName]._fieldList.push_back(field);

    } else if (strcmp(name, "fixedRecordData") == 0) {
        if (getCurrentMode() != FixedRecordDataTypesMode)
            throw sg_exception("fixedRecordData tag outside fixedRecordDataTypes!");
        pushMode(FixedRecordDataMode);

        std::string name = getAttribute("name", atts);
        if (name.empty())
            throw sg_exception("fixedRecordData tag without name attribute");

        _fixedRecordDataName = name;
        _fixedRecordDataMap[name]._encoding = getAttribute("encoding", atts);

    } else if (strcmp(name, "fixedRecordDataTypes") == 0) {
        if (getCurrentMode() != DataTypesMode)
            throw sg_exception("fixedRecordDataTypes tag outside dataTypes!");
        pushMode(FixedRecordDataTypesMode);

    } else if (strcmp(name, "alternative") == 0) {

        if (getCurrentMode() != VariantRecordDataMode)
            throw sg_exception("alternative tag outside variantRecordData!");
        pushMode(AlternativeDataMode);

        std::string name = getAttribute("name", atts);
        if (name.empty())
            throw sg_exception("alternative tag without name attribute");

        Alternative alternative;
        alternative._name = name;
        alternative._dataType = getAttribute("dataType", atts);
        alternative._semantics = getAttribute("semantics", atts);
        alternative._enumerator = getAttribute("enumerator", atts);
        _variantRecordDataMap[_variantRecordDataName]._alternativeList.push_back(alternative);

    } else if (strcmp(name, "variantRecordData") == 0) {
        if (getCurrentMode() != VariantRecordDataTypesMode)
            throw sg_exception("variantRecordData tag outside variantRecordDataTypes!");
        pushMode(VariantRecordDataMode);

        std::string name = getAttribute("name", atts);
        if (name.empty())
            throw sg_exception("fixedRecordData tag without name attribute");

        _variantRecordDataName = name;
        _variantRecordDataMap[name]._encoding = getAttribute("encoding", atts);
        _variantRecordDataMap[name]._dataType = getAttribute("dataType", atts);
        _variantRecordDataMap[name]._semantics = getAttribute("semantics", atts);
        _variantRecordDataMap[name]._discriminant = getAttribute("discriminant", atts);

    } else if (strcmp(name, "variantRecordDataTypes") == 0) {
        if (getCurrentMode() != DataTypesMode)
            throw sg_exception("variantRecordDataTypes tag outside dataTypes!");
        pushMode(VariantRecordDataTypesMode);

    } else if (strcmp(name, "dataTypes") == 0) {
        if (getCurrentMode() != ObjectModelMode)
            throw sg_exception("dataTypes tag outside objectModel!");
        pushMode(DataTypesMode);

    } else if (strcmp(name, "objectModel") == 0) {
        if (!_modeStack.empty())
            throw sg_exception("objectModel tag not at top level!");
        pushMode(ObjectModelMode);

    } else {
        _modeStack.push_back(UnknownMode);
    }
}

void
HLAOMTXmlVisitor::endElement(const char* name)
{
    if (strcmp(name, "objectClass") == 0) {
        _objectClassStack.pop_back();
    } else if (strcmp(name, "interactionClass") == 0) {
        _interactionClassStack.pop_back();
    } else if (strcmp(name, "enumeratedData") == 0) {
        _enumeratedDataName.clear();
    } else if (strcmp(name, "fixedRecordData") == 0) {
        _fixedRecordDataName.clear();
    } else if (strcmp(name, "variantRecordData") == 0) {
        _variantRecordDataName.clear();
    }

    _modeStack.pop_back();
}

std::string
HLAOMTXmlVisitor::getAttribute(const char* name, const XMLAttributes& atts)
{
    int index = atts.findAttribute(name);
    if (index < 0 || atts.size() <= index)
        return std::string();
    return std::string(atts.getValue(index));
}

std::string
HLAOMTXmlVisitor::getAttribute(const std::string& name, const XMLAttributes& atts)
{
    int index = atts.findAttribute(name.c_str());
    if (index < 0 || atts.size() <= index)
        return std::string();
    return std::string(atts.getValue(index));
}

} // namespace simgear
