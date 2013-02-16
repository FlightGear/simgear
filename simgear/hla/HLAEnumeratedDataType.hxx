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

#ifndef HLAEnumeratedDataType_hxx
#define HLAEnumeratedDataType_hxx

#include <string>
#include <simgear/structure/SGSharedPtr.hxx>
#include "HLADataType.hxx"

namespace simgear {

class HLAEnumeratedDataType : public HLADataType {
public:
    HLAEnumeratedDataType(const std::string& name = "HLAEnumeratedDataType");
    virtual ~HLAEnumeratedDataType();

    virtual void accept(HLADataTypeVisitor& visitor) const;

    virtual const HLAEnumeratedDataType* toEnumeratedDataType() const;

    virtual bool decode(HLADecodeStream& stream, unsigned& index) const;
    virtual bool encode(HLAEncodeStream& stream, unsigned index) const;

    std::string getString(unsigned index) const
    {
        if (!_map.valid())
            return std::string();
        return _map->getString(index);
    }
    unsigned getIndex(const std::string& name) const
    {
        if (!_map.valid())
            return ~unsigned(0);
        return _map->getIndex(name);
    }
    unsigned getNumEnumerators() const
    {
        if (!_map.valid())
            return 0;
        return _map->getNumEnumerators();
    }

    bool addEnumerator(const std::string& name, const std::string& number)
    {
        if (!_map.valid())
            return false;
        return _map->add(name, number);
    }

    void setRepresentation(HLABasicDataType* representation);
    const HLABasicDataType* getRepresentation() const
    {
        if (!_map.valid())
            return 0;
        return _map->getDataType();
    }

protected:
    virtual void _recomputeAlignmentImplementation();

private:
    class AbstractMap : public SGReferenced {
    public:
        virtual ~AbstractMap() {}
        virtual bool encode(HLAEncodeStream& stream, unsigned index) const = 0;
        virtual bool decode(HLADecodeStream& stream, unsigned& index) const = 0;
        virtual const HLABasicDataType* getDataType() const = 0;
        virtual bool add(const std::string& name, const std::string& number) = 0;
        virtual std::string getString(unsigned index) const = 0;
        virtual unsigned getIndex(const std::string& name) const = 0;
        virtual unsigned getNumEnumerators() const = 0;
    };

    template<typename DataType, typename T>
    class Map;
    class RepresentationVisitor;

    SGSharedPtr<AbstractMap> _map;
};

} // namespace simgear

#endif
