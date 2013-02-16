// Copyright (C) 2009 - 2011  Mathias Froehlich - Mathias.Froehlich@web.de
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

#include <simgear/props/props.hxx>
#include "HLADataElement.hxx"

namespace simgear {

class HLAPropertyDataElement : public HLADataElement {
public:
    HLAPropertyDataElement();
    HLAPropertyDataElement(SGPropertyNode* propertyNode);
    HLAPropertyDataElement(const HLADataType* dataType, SGPropertyNode* propertyNode);
    HLAPropertyDataElement(const HLADataType* dataType);
    virtual ~HLAPropertyDataElement();

    virtual void accept(HLADataElementVisitor& visitor);
    virtual void accept(HLAConstDataElementVisitor& visitor) const;

    virtual bool encode(HLAEncodeStream& stream) const;
    virtual bool decode(HLADecodeStream& stream);

    virtual const HLADataType* getDataType() const;
    virtual bool setDataType(const HLADataType* dataType);

    void setPropertyNode(SGPropertyNode* propertyNode);
    SGPropertyNode* getPropertyNode();
    const SGPropertyNode* getPropertyNode() const;
    
private:
    HLADataElement*
    createDataElement(const SGSharedPtr<const HLADataType>& dataType, const SGSharedPtr<SGPropertyNode>& propertyNode);
    
    class ScalarDecodeVisitor;
    class ScalarEncodeVisitor;
    class ScalarDataElement;
    class StringDecodeVisitor;
    class StringEncodeVisitor;
    class StringDataElement;
    class DataElementFactoryVisitor;

    SGSharedPtr<const HLADataType> _dataType;
    SGSharedPtr<HLADataElement> _dataElement;
    SGSharedPtr<SGPropertyNode> _propertyNode;
};

} // namespace simgear

#endif
