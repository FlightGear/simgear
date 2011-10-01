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

#ifndef HLAPropertyDataElement_hxx
#define HLAPropertyDataElement_hxx

#include <set>
#include <simgear/props/props.hxx>
#include "HLADataElement.hxx"

namespace simgear {

class HLAPropertyReference : public SGReferenced {
public:
    HLAPropertyReference()
    { }
    HLAPropertyReference(const std::string& relativePath) :
        _relativePath(relativePath)
    { }

    void setIntValue(int value)
    {
        if (!_propertyNode.valid())
            return;
        _propertyNode->setIntValue(value);
    }
    int getIntValue() const
    {
        if (!_propertyNode.valid())
            return 0;
        return _propertyNode->getIntValue();
    }

    void setLongValue(long value)
    {
        if (!_propertyNode.valid())
            return;
        _propertyNode->setLongValue(value);
    }
    long getLongValue() const
    {
        if (!_propertyNode.valid())
            return 0;
        return _propertyNode->getLongValue();
    }

    void setFloatValue(float value)
    {
        if (!_propertyNode.valid())
            return;
        _propertyNode->setFloatValue(value);
    }
    float getFloatValue() const
    {
        if (!_propertyNode.valid())
            return 0;
        return _propertyNode->getFloatValue();
    }

    void setDoubleValue(double value)
    {
        if (!_propertyNode.valid())
            return;
        _propertyNode->setDoubleValue(value);
    }
    double getDoubleValue() const
    {
        if (!_propertyNode.valid())
            return 0;
        return _propertyNode->getDoubleValue();
    }

    void setStringValue(const std::string& value)
    {
        if (!_propertyNode.valid())
            return;
        _propertyNode->setStringValue(value);
    }
    std::string getStringValue() const
    {
        if (!_propertyNode.valid())
            return std::string();
        return _propertyNode->getStringValue();
    }

    SGPropertyNode* getPropertyNode()
    { return _propertyNode.get(); }

    void setRootNode(SGPropertyNode* rootNode)
    {
        if (!rootNode)
            _propertyNode.clear();
        else
            _propertyNode = rootNode->getNode(_relativePath, true);
    }

private:
    std::string _relativePath;
    SGSharedPtr<SGPropertyNode> _propertyNode;
};

class HLAPropertyReferenceSet : public SGReferenced {
public:
    void insert(const SGSharedPtr<HLAPropertyReference>& propertyReference)
    {
        _propertyReferenceSet.insert(propertyReference);
        propertyReference->setRootNode(_rootNode.get());
    }
    void remove(const SGSharedPtr<HLAPropertyReference>& propertyReference)
    {
        PropertyReferenceSet::iterator i = _propertyReferenceSet.find(propertyReference);
        if (i == _propertyReferenceSet.end())
            return;
        _propertyReferenceSet.erase(i);
        propertyReference->setRootNode(0);
    }

    void setRootNode(SGPropertyNode* rootNode)
    {
        _rootNode = rootNode;
        for (PropertyReferenceSet::iterator i = _propertyReferenceSet.begin();
             i != _propertyReferenceSet.end(); ++i) {
            (*i)->setRootNode(_rootNode.get());
        }
    }
    SGPropertyNode* getRootNode()
    { return _rootNode.get(); }

private:
    SGSharedPtr<SGPropertyNode> _rootNode;

    typedef std::set<SGSharedPtr<HLAPropertyReference> > PropertyReferenceSet;
    PropertyReferenceSet _propertyReferenceSet;
};

class HLAPropertyDataElement : public HLADataElement {
public:
    HLAPropertyDataElement(HLAPropertyReference* propertyReference);
    HLAPropertyDataElement(const simgear::HLADataType* dataType, HLAPropertyReference* propertyReference);
    virtual ~HLAPropertyDataElement();

    virtual bool encode(HLAEncodeStream& stream) const;
    virtual bool decode(HLADecodeStream& stream);

    virtual const HLADataType* getDataType() const;
    virtual bool setDataType(const HLADataType* dataType);

private:
    class DecodeVisitor;
    class EncodeVisitor;

    SGSharedPtr<const HLADataType> _dataType;
    SGSharedPtr<HLAPropertyReference> _propertyReference;
};

} // namespace simgear

#endif
