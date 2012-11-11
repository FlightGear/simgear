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

#ifndef HLADataType_hxx
#define HLADataType_hxx

#include <string>
#include <simgear/structure/SGWeakPtr.hxx>
#include <simgear/structure/SGWeakReferenced.hxx>
#include "RTIData.hxx"
#include "HLATypes.hxx"

namespace simgear {

class HLADataTypeVisitor;

class HLABasicDataType;
class HLAArrayDataType;
class HLAEnumeratedDataType;
class HLAFixedRecordDataType;
class HLAVariantRecordDataType;

class HLADataType : public SGWeakReferenced {
public:
    virtual ~HLADataType();

    const std::string& getName() const
    { return _name; }

    const std::string& getSemantics() const
    { return _semantics; }
    void setSemantics(const std::string& semantics)
    { _semantics = semantics; }

    unsigned getAlignment() const
    { return _alignment; }

    virtual void accept(HLADataTypeVisitor& visitor) const;

    virtual const HLABasicDataType* toBasicDataType() const;
    virtual const HLAArrayDataType* toArrayDataType() const;
    virtual const HLAEnumeratedDataType* toEnumeratedDataType() const;
    virtual const HLAFixedRecordDataType* toFixedRecordDataType() const;
    virtual const HLAVariantRecordDataType* toVariantRecordDataType() const;

    /// Recompute the alignment value of this data type.
    /// Return true if the alignment changed, false otherwise.
    bool recomputeAlignment();
    /// Release references to other data types. Since we can have cycles this is
    /// required for propper feeing of memory.
    virtual void releaseDataTypeReferences();

    bool getDataElementIndex(HLADataElementIndex& index, const std::string& path, std::string::size_type offset) const;

protected:
    HLADataType(const std::string& name, unsigned alignment = 1);
    void setAlignment(unsigned alignment);

    virtual void _recomputeAlignmentImplementation();

private:
    class _DataElementIndexVisitor;

    std::string _name;
    std::string _semantics;
    unsigned _alignment;
};

} // namespace simgear

#endif
