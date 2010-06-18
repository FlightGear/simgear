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

#ifndef HLARawDataElement_hxx
#define HLARawDataElement_hxx

#include "RTIData.hxx"
#include "HLADataElement.hxx"
#include "HLADataType.hxx"

namespace simgear {

class HLARawDataElement : public HLADataElement {
public:
    HLARawDataElement(const HLADataType* dataType);
    virtual ~HLARawDataElement();

    virtual bool encode(HLAEncodeStream& stream) const;
    virtual bool decode(HLADecodeStream& stream);

    virtual const HLADataType* getDataType() const;
    virtual bool setDataType(const HLADataType* dataType);

    const RTIData& getData() const
    { return _rtiData; }
    RTIData& getData()
    { return _rtiData; }
    void setData(const RTIData& rtiData)
    { _rtiData = rtiData; }

protected:
    SGSharedPtr<const HLADataType> _dataType;
    RTIData _rtiData;
};

}

#endif
