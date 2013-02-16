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
#include "HLATypes.hxx"

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

    bool setDataElement(const HLADataElementIndex& index, HLADataElement* dataElement)
    { return setDataElement(index.begin(), index.end(), dataElement); }
    HLADataElement* getDataElement(const HLADataElementIndex& index)
    { return getDataElement(index.begin(), index.end()); }
    const HLADataElement* getDataElement(const HLADataElementIndex& index) const
    { return getDataElement(index.begin(), index.end()); }

    virtual bool setDataElement(HLADataElementIndex::const_iterator begin, HLADataElementIndex::const_iterator end, HLADataElement* dataElement);
    virtual HLADataElement* getDataElement(HLADataElementIndex::const_iterator begin, HLADataElementIndex::const_iterator end);
    virtual const HLADataElement* getDataElement(HLADataElementIndex::const_iterator begin, HLADataElementIndex::const_iterator end) const;

    /// Returns the time stamp if this data element.
    /// Do not access this getter if the getTimeStampValid() method returns false.
    const SGTimeStamp& getTimeStamp() const
    { return _stamp->getTimeStamp(); }
    void setTimeStamp(const SGTimeStamp& timeStamp);

    bool getTimeStampValid() const
    { if (!_stamp.valid()) return false; return _stamp->getTimeStampValid(); }
    void setTimeStampValid(bool timeStampValid);

    /// Convenience function that gives the time difference in seconds to a given timestamp
    /// This function returns 0 if the timestamp is not valid.
    double getTimeDifference(const SGTimeStamp& timeStamp) const;

    /// Dirty tracking of the attribute/parameter that this data element belongs to
    bool getDirty() const
    { if (!_stamp.valid()) return true; return _stamp->getDirty(); }
    void setDirty(bool dirty)
    { if (!_stamp.valid()) return; _stamp->setDirty(dirty); }

    /// Stamp handling
    void createStamp();
    void attachStamp(HLADataElement& dataElement);
    void clearStamp();

protected:
    // Container for the timestamp the originating attribute was last updated for
    class Stamp : public SGReferenced {
    public:
        Stamp() : _timeStampValid(false), _dirty(true)
        { }

        const SGTimeStamp& getTimeStamp() const
        { return _timeStamp; }
        void setTimeStamp(const SGTimeStamp& timeStamp)
        { _timeStamp = timeStamp; }

        bool getTimeStampValid() const
        { return _timeStampValid; }
        void setTimeStampValid(bool timeStampValid)
        { _timeStampValid = timeStampValid; }

        bool getDirty() const
        { return _dirty; }
        void setDirty(bool dirty)
        { _dirty = dirty; }

    private:
        SGTimeStamp _timeStamp;
        bool _timeStampValid;
        bool _dirty;
    };

    /// get the stamp
    Stamp* _getStamp() const
    { return _stamp.get(); }
    /// Set the stamp
    virtual void _setStamp(Stamp* stamp);

private:
    SGSharedPtr<Stamp> _stamp;
};

}

#endif
