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

#ifndef HLADataElement_hxx
#define HLADataElement_hxx

#include <list>
#include <map>
#include <simgear/structure/SGReferenced.hxx>
#include <simgear/structure/SGSharedPtr.hxx>
#include <simgear/timing/timestamp.hxx>
#include "RTIData.hxx"
#include "HLADataType.hxx"

class SGTimeStamp;

namespace simgear {

class HLADataElementVisitor;
class HLAConstDataElementVisitor;

class HLADataElement : public SGReferenced {
public:
    virtual ~HLADataElement();

    virtual void accept(HLADataElementVisitor& visitor) = 0;
    virtual void accept(HLAConstDataElementVisitor& visitor) const = 0;

    virtual bool encode(HLAEncodeStream& stream) const = 0;
    virtual bool decode(HLADecodeStream& stream) = 0;

    virtual const HLADataType* getDataType() const = 0;
    virtual bool setDataType(const HLADataType* dataType) = 0;

    // Container for the timestamp the originating attribute was last updated for
    // class TimeStamp : public SGReferenced {
    // public:
    //     const SGTimeStamp& getTimeStamp() const
    //     { return _timeStamp; }
    //     void setTimeStamp(const SGTimeStamp& timeStamp)
    //     { _timeStamp = timeStamp; }
    // private:
    //     SGTimeStamp _timeStamp;
    // };

    // const TimeStamp* getTimeStamp() const
    // { return _timeStamp.get(); }
    // void setTimeStamp(const TimeStamp* timeStamp)
    // { _timeStamp = timeStamp; }

    // struct ChangeCount : public SGReferenced {
    //     ChangeCount() : _value(0) {}
    //     unsigned _value;
    // };
    // SGSharedPtr<ChangeCount> _changeCount;
    // unsigned getChangeCount() const
    // {
    //     // If we don't have return allways the same
    //     if (!_changeCount.valid())
    //         return 0;
    //     return _changeCount->_value;
    // }

    /// HLADataElements could be identified by path
    /// These paths are composed of structure field names and array indices in the
    /// order they appear while walking to the data element.
    /// So provide here some tool functions to access these elements
    /// Note that these functions are relatively expensive in execution time.
    /// So only use them once at object creation time and store direct references to the values

    class PathElement {
    public:
        PathElement(unsigned index) : _data(new IndexData(index)) {}
        PathElement(const std::string& name) : _data(new FieldData(name)) {}

        bool isFieldValue() const
        { return _data->toFieldData(); }
        bool isIndexValue() const
        { return _data->toIndexData(); }

        unsigned getIndexValue() const
        {
            const IndexData* indexData = _data->toIndexData();
            if (!indexData)
                return ~unsigned(0);
            return indexData->_index;
        }

        std::string getFieldValue() const
        {
            const FieldData* fieldData = _data->toFieldData();
            if (!fieldData)
                return std::string();
            return fieldData->_name;
        }

        // Want to be able to use that in std::map and std::set
        bool operator<(const PathElement& pathElement) const
        { return _data->less(pathElement._data.get()); }
        bool operator==(const PathElement& pathElement) const
        { return _data->equal(pathElement._data.get()); }

        void append(std::string& s) const
        { _data->append(s); }

    private:
        struct FieldData;
        struct IndexData;
        struct Data : public SGReferenced {
            virtual ~Data();
            virtual const FieldData* toFieldData() const;
            virtual const IndexData* toIndexData() const;
            virtual bool less(const Data*) const = 0;
            virtual bool equal(const Data*) const = 0;
            virtual void append(std::string&) const = 0;
        };
        struct FieldData : public Data {
            FieldData(const std::string& name);
            virtual ~FieldData();
            virtual const FieldData* toFieldData() const;
            virtual bool less(const Data* data) const;
            virtual bool equal(const Data* data) const;
            virtual void append(std::string& s) const;
            std::string _name;
        };
        struct IndexData : public Data {
            IndexData(unsigned index);
            virtual ~IndexData();
            virtual const IndexData* toIndexData() const;
            virtual bool less(const Data* data) const;
            virtual bool equal(const Data* data) const;
            virtual void append(std::string& s) const;
            unsigned _index;
        };

        SGSharedPtr<Data> _data;
    };
    typedef std::list<PathElement> Path;
    typedef std::pair<std::string, Path> AttributePathPair;
    typedef std::pair<unsigned, Path> IndexPathPair;

    static std::string toString(const Path& path);
    static std::string toString(const AttributePathPair& path)
    { return path.first + toString(path.second); }
    static AttributePathPair toAttributePathPair(const std::string& s);
    static Path toPath(const std::string& s)
    { return toAttributePathPair(s).second; }

private:
    // SGSharedPtr<const TimeStamp> _timeStamp;
};

class HLADataElementProvider {
public:
    class AbstractProvider : public SGReferenced {
    public:
        virtual ~AbstractProvider() { }
        virtual HLADataElement* getDataElement(const HLADataElement::Path& path) = 0;
        // virtual HLADataElement* getDataElement(const HLADataElement::Path& path, const HLADataType* dataType)
        // {
        //     SGSharedPtr<HLADataElement> dataElement = getDataElement(path);
        //     if (!dataElement.valid())
        //         return 0;
        //     if (!dataElement->setDataType(dataType))
        //         return 0;
        //     return dataElement.release();
        // }
    };

    HLADataElementProvider()
    { }
    HLADataElementProvider(HLADataElement* dataElement) :
        _provider(new ConcreteProvider(dataElement))
    { }
    HLADataElementProvider(const SGSharedPtr<HLADataElement>& dataElement) :
        _provider(new ConcreteProvider(dataElement))
    { }
    HLADataElementProvider(AbstractProvider* provider) :
        _provider(provider)
    { }

    HLADataElement* getDataElement(const HLADataElement::Path& path) const
    {
        if (!_provider.valid())
            return 0;
        return _provider->getDataElement(path);
    }

private:
    class ConcreteProvider : public AbstractProvider {
    public:
        ConcreteProvider(const SGSharedPtr<HLADataElement>& dataElement) :
            _dataElement(dataElement)
        { }
        virtual HLADataElement* getDataElement(const HLADataElement::Path&)
        { return _dataElement.get(); }
    private:
        SGSharedPtr<HLADataElement> _dataElement;
    };

    SGSharedPtr<AbstractProvider> _provider;
};

typedef std::map<HLADataElement::Path, HLADataElementProvider> HLAPathElementMap;
typedef std::map<unsigned, HLAPathElementMap> HLAAttributePathElementMap;

}

#endif
