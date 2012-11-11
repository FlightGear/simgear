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

#ifndef HLALocation_hxx
#define HLALocation_hxx

#include "HLABasicDataElement.hxx"
#include "HLATypes.hxx"

namespace simgear {

class HLAAbstractLocation : public SGReferenced {
public:
    virtual ~HLAAbstractLocation() {}

    virtual SGVec3d getCartPosition() const = 0;
    virtual void setCartPosition(const SGVec3d&) = 0;

    virtual SGQuatd getCartOrientation() const = 0;
    virtual void setCartOrientation(const SGQuatd&) = 0;

    virtual SGVec3d getAngularBodyVelocity() const = 0;
    virtual void setAngularBodyVelocity(const SGVec3d& angular) = 0;

    virtual SGVec3d getLinearBodyVelocity() const = 0;
    virtual void setLinearBodyVelocity(const SGVec3d& linear) = 0;
};

class HLACartesianLocation : public HLAAbstractLocation {
public:
    HLACartesianLocation() :
        _position(SGVec3d::zeros()),
        _imag(SGVec3d::zeros()),
        _angularVelocity(SGVec3d::zeros()),
        _linearVelocity(SGVec3d::zeros())
    { }

    virtual SGVec3d getCartPosition() const
    { return _position; }
    virtual void setCartPosition(const SGVec3d& position)
    { _position = position; }

    virtual SGQuatd getCartOrientation() const
    { return SGQuatd::fromPositiveRealImag(_imag); }
    virtual void setCartOrientation(const SGQuatd& orientation)
    { _imag = orientation.getPositiveRealImag(); }

    virtual SGVec3d getAngularBodyVelocity() const
    { return _angularVelocity; }
    virtual void setAngularBodyVelocity(const SGVec3d& angularVelocity)
    { _angularVelocity = angularVelocity; }

    virtual SGVec3d getLinearBodyVelocity() const
    { return _linearVelocity; }
    virtual void setLinearBodyVelocity(const SGVec3d& linearVelocity)
    { _linearVelocity = linearVelocity; }

    HLADataElement* getPositionDataElement(unsigned i)
    {
        if (3 <= i)
            return 0;
        return new PositionDataElement(this, i);
    }
    HLADataElement* getOrientationDataElement(unsigned i)
    {
        if (3 <= i)
            return 0;
        return new OrientationDataElement(this, i);
    }

    HLADataElement* getAngularVelocityDataElement(unsigned i)
    {
        if (3 <= i)
            return 0;
        return new AngularVelocityDataElement(this, i);
    }
    HLADataElement* getLinearVelocityDataElement(unsigned i)
    {
        if (3 <= i)
            return 0;
        return new LinearVelocityDataElement(this, i);
    }

private:
    class PositionDataElement : public HLAAbstractDoubleDataElement {
    public:
        PositionDataElement(HLACartesianLocation* data, unsigned index) :
            _data(data), _index(index)
        { }
        virtual double getValue() const
        { return _data->_position[_index]; }
        virtual void setValue(double value)
        { _data->_position[_index] = value; }

    private:
        SGSharedPtr<HLACartesianLocation> _data;
        unsigned _index;
    };

    class OrientationDataElement : public HLAAbstractDoubleDataElement {
    public:
        OrientationDataElement(HLACartesianLocation* data, unsigned index) :
            _data(data), _index(index)
        { }
        virtual double getValue() const
        { return _data->_imag[_index]; }
        virtual void setValue(double value)
        { _data->_imag[_index] = value; }

    private:
        SGSharedPtr<HLACartesianLocation> _data;
        unsigned _index;
    };

    class AngularVelocityDataElement : public HLAAbstractDoubleDataElement {
    public:
        AngularVelocityDataElement(HLACartesianLocation* data, unsigned index) :
            _data(data), _index(index)
        { }
        virtual double getValue() const
        { return _data->_angularVelocity[_index]; }
        virtual void setValue(double value)
        { _data->_angularVelocity[_index] = value; }

    private:
        SGSharedPtr<HLACartesianLocation> _data;
        unsigned _index;
    };

    class LinearVelocityDataElement : public HLAAbstractDoubleDataElement {
    public:
        LinearVelocityDataElement(HLACartesianLocation* data, unsigned index) :
            _data(data), _index(index)
        { }
        virtual double getValue() const
        { return _data->_linearVelocity[_index]; }
        virtual void setValue(double value)
        { _data->_linearVelocity[_index] = value; }

    private:
        SGSharedPtr<HLACartesianLocation> _data;
        unsigned _index;
    };

    SGVec3d _position;
    SGVec3d _imag;

    SGVec3d _angularVelocity;
    SGVec3d _linearVelocity;
};

class HLALocationFactory : public SGReferenced {
public:
    virtual ~HLALocationFactory() {}
    virtual HLAAbstractLocation* createLocation(HLAObjectInstance&) const = 0;
};

class HLACartesianLocationFactory : public HLALocationFactory {
public:
    virtual HLACartesianLocation* createLocation(HLAObjectInstance& objectInstance) const
    {
        HLACartesianLocation* location = new HLACartesianLocation;

        for (unsigned i = 0; i < 3; ++i)
            objectInstance.setAttributeDataElement(_positonIndex[i], location->getPositionDataElement(i));
        for (unsigned i = 0; i < 3; ++i)
            objectInstance.setAttributeDataElement(_orientationIndex[i], location->getOrientationDataElement(i));
        for (unsigned i = 0; i < 3; ++i)
            objectInstance.setAttributeDataElement(_angularVelocityIndex[i], location->getAngularVelocityDataElement(i));
        for (unsigned i = 0; i < 3; ++i)
            objectInstance.setAttributeDataElement(_linearVelocityIndex[i], location->getLinearVelocityDataElement(i));

        return location;
    }

    void setPositionIndex(unsigned index, const HLADataElementIndex& dataElementIndex)
    {
        if (3 <= index)
            return;
        _positonIndex[index] = dataElementIndex;
    }
    void setOrientationIndex(unsigned index, const HLADataElementIndex& dataElementIndex)
    {
        if (3 <= index)
            return;
        _orientationIndex[index] = dataElementIndex;
    }

    void setAngularVelocityIndex(unsigned index, const HLADataElementIndex& dataElementIndex)
    {
        if (3 <= index)
            return;
        _angularVelocityIndex[index] = dataElementIndex;
    }
    void setLinearVelocityIndex(unsigned index, const HLADataElementIndex& dataElementIndex)
    {
        if (3 <= index)
            return;
        _linearVelocityIndex[index] = dataElementIndex;
    }

private:
    HLADataElementIndex _positonIndex[3];
    HLADataElementIndex _orientationIndex[3];

    HLADataElementIndex _angularVelocityIndex[3];
    HLADataElementIndex _linearVelocityIndex[3];
};

class HLAGeodeticLocation : public HLAAbstractLocation {
public:
    enum Semantic {
        LatitudeDeg,
        LatitudeRad,
        LongitudeDeg,
        LongitudeRad,
        ElevationM,
        ElevationFt,
        HeadingDeg,
        HeadingRad,
        PitchDeg,
        PitchRad,
        RollDeg,
        RollRad,
        GroundTrackDeg,
        GroundTrackRad,
        GroundSpeedKnots,
        GroundSpeedFtPerSec,
        GroundSpeedMPerSec,
        VerticalSpeedFtPerSec,
        VerticalSpeedFtPerMin,
        VerticalSpeedMPerSec
    };

    HLAGeodeticLocation() :
        _dirty(true),
        _cartPosition(SGVec3d::zeros()),
        _cartOrientation(SGQuatd::unit()),
        _cartBodyVelocity(SGVec3d::zeros()),
        _geodPosition(),
        _geodEulerRad(SGVec3d::zeros()),
        _groundTrackRad(0),
        _groundSpeedMPerSec(0),
        _verticalSpeedMPerSec(0)
    {
        updateCartesianFromGeodetic();
    }

    virtual SGVec3d getCartPosition() const
    { updateCartesianFromGeodetic(); return _cartPosition; }
    virtual void setCartPosition(const SGVec3d& position)
    { _cartPosition = position; _dirty = true; }

    virtual SGQuatd getCartOrientation() const
    { updateCartesianFromGeodetic(); return _cartOrientation; }
    virtual void setCartOrientation(const SGQuatd& orientation)
    { _cartOrientation = orientation; _dirty = true; }

    virtual SGVec3d getAngularBodyVelocity() const
    { return SGVec3d::zeros(); }
    virtual void setAngularBodyVelocity(const SGVec3d& angular)
    { }

    virtual SGVec3d getLinearBodyVelocity() const
    { updateCartesianFromGeodetic(); return _cartBodyVelocity; }
    virtual void setLinearBodyVelocity(const SGVec3d& linear)
    { _cartBodyVelocity = linear; _dirty = true; }

    void setLatitudeDeg(double value)
    { _geodPosition.setLatitudeDeg(value); _dirty = true; }
    double getLatitudeDeg() const
    { updateGeodeticFromCartesian(); return _geodPosition.getLatitudeDeg(); }
    void setLatitudeRad(double value)
    { _geodPosition.setLatitudeRad(value); _dirty = true; }
    double getLatitudeRad() const
    { updateGeodeticFromCartesian(); return _geodPosition.getLatitudeRad(); }
    void setLongitudeDeg(double value)
    { _geodPosition.setLongitudeDeg(value); _dirty = true; }
    double getLongitudeDeg() const
    { updateGeodeticFromCartesian(); return _geodPosition.getLongitudeDeg(); }
    void setLongitudeRad(double value)
    { _geodPosition.setLongitudeRad(value); _dirty = true; }
    double getLongitudeRad() const
    { updateGeodeticFromCartesian(); return _geodPosition.getLongitudeRad(); }
    void setElevationFt(double value)
    { _geodPosition.setElevationFt(value); _dirty = true; }
    double getElevationFt() const
    { updateGeodeticFromCartesian(); return _geodPosition.getElevationFt(); }
    void setElevationM(double value)
    { _geodPosition.setElevationM(value); _dirty = true; }
    double getElevationM() const
    { updateGeodeticFromCartesian(); return _geodPosition.getElevationM(); }

    void setHeadingRad(double value)
    { _geodEulerRad[2] = value; _dirty = true; }
    double getHeadingRad() const
    { updateGeodeticFromCartesian(); return _geodEulerRad[2]; }
    void setHeadingDeg(double value)
    { setHeadingRad(SGMiscd::deg2rad(value)); }
    double getHeadingDeg() const
    { return SGMiscd::rad2deg(getHeadingRad()); }
    void setPitchRad(double value)
    { _geodEulerRad[1] = value; _dirty = true; }
    double getPitchRad() const
    { updateGeodeticFromCartesian(); return _geodEulerRad[1]; }
    void setPitchDeg(double value)
    { setPitchRad(SGMiscd::deg2rad(value)); }
    double getPitchDeg() const
    { return SGMiscd::rad2deg(getPitchRad()); }
    void setRollRad(double value)
    { _geodEulerRad[0] = value; _dirty = true; }
    double getRollRad() const
    { updateGeodeticFromCartesian(); return _geodEulerRad[0]; }
    void setRollDeg(double value)
    { setRollRad(SGMiscd::deg2rad(value)); }
    double getRollDeg() const
    { return SGMiscd::rad2deg(getRollRad()); }

    void setGroundTrackRad(double value)
    { _groundTrackRad = value; _dirty = true; }
    double getGroundTrackRad() const
    { updateGeodeticFromCartesian(); return _groundTrackRad; }
    void setGroundTrackDeg(double value)
    { setGroundTrackRad(SGMiscd::deg2rad(value)); }
    double getGroundTrackDeg() const
    { return SGMiscd::rad2deg(getGroundTrackRad()); }


    void setGroundSpeedMPerSec(double value)
    { _groundSpeedMPerSec = value; _dirty = true; }
    double getGroundSpeedMPerSec() const
    { updateGeodeticFromCartesian(); return _groundSpeedMPerSec; }
    void setGroundSpeedFtPerSec(double value)
    { setGroundSpeedMPerSec(SG_FEET_TO_METER*value); }
    double getGroundSpeedFtPerSec() const
    { return SG_METER_TO_FEET*getGroundSpeedMPerSec(); }
    void setGroundSpeedKnots(double value)
    { setGroundSpeedMPerSec(SG_KT_TO_MPS*value); }
    double getGroundSpeedKnots() const
    { return SG_MPS_TO_KT*getGroundSpeedMPerSec(); }

    void setVerticalSpeedMPerSec(double value)
    { _verticalSpeedMPerSec = value; _dirty = true; }
    double getVerticalSpeedMPerSec() const
    { updateGeodeticFromCartesian(); return _verticalSpeedMPerSec; }
    void setVerticalSpeedFtPerSec(double value)
    { setVerticalSpeedMPerSec(SG_FEET_TO_METER*value); }
    double getVerticalSpeedFtPerSec() const
    { return SG_METER_TO_FEET*getVerticalSpeedMPerSec(); }
    void setVerticalSpeedFtPerMin(double value)
    { setVerticalSpeedFtPerSec(value/60); }
    double getVerticalSpeedFtPerMin() const
    { return 60*getVerticalSpeedFtPerSec(); }

#define DATA_ELEMENT(name) \
    HLADataElement* get## name ## DataElement()                  \
    { return new DataElement<&HLAGeodeticLocation::get## name, &HLAGeodeticLocation::set ## name>(this); }

    DATA_ELEMENT(LatitudeDeg)
    DATA_ELEMENT(LatitudeRad)
    DATA_ELEMENT(LongitudeDeg)
    DATA_ELEMENT(LongitudeRad)
    DATA_ELEMENT(ElevationFt)
    DATA_ELEMENT(ElevationM)
    DATA_ELEMENT(HeadingDeg)
    DATA_ELEMENT(HeadingRad)
    DATA_ELEMENT(PitchDeg)
    DATA_ELEMENT(PitchRad)
    DATA_ELEMENT(RollDeg)
    DATA_ELEMENT(RollRad)

    DATA_ELEMENT(GroundTrackDeg)
    DATA_ELEMENT(GroundTrackRad)
    DATA_ELEMENT(GroundSpeedMPerSec)
    DATA_ELEMENT(GroundSpeedFtPerSec)
    DATA_ELEMENT(GroundSpeedKnots)
    DATA_ELEMENT(VerticalSpeedMPerSec)
    DATA_ELEMENT(VerticalSpeedFtPerSec)
    DATA_ELEMENT(VerticalSpeedFtPerMin)

#undef DATA_ELEMENT

    HLADataElement* getDataElement(Semantic semantic)
    {
        switch (semantic) {
        case LatitudeDeg:
            return getLatitudeDegDataElement();
        case LatitudeRad:
            return getLatitudeRadDataElement();
        case LongitudeDeg:
            return getLongitudeDegDataElement();
        case LongitudeRad:
            return getLongitudeRadDataElement();
        case ElevationM:
            return getElevationMDataElement();
        case ElevationFt:
            return getElevationFtDataElement();
        case HeadingDeg:
            return getHeadingDegDataElement();
        case HeadingRad:
            return getHeadingRadDataElement();
        case PitchDeg:
            return getPitchDegDataElement();
        case PitchRad:
            return getPitchRadDataElement();
        case RollDeg:
            return getRollDegDataElement();
        case RollRad:
            return getRollRadDataElement();
        case GroundTrackDeg:
            return getGroundTrackDegDataElement();
        case GroundTrackRad:
            return getGroundTrackRadDataElement();
        case GroundSpeedKnots:
            return getGroundSpeedKnotsDataElement();
        case GroundSpeedFtPerSec:
            return getGroundSpeedFtPerSecDataElement();
        case GroundSpeedMPerSec:
            return getGroundSpeedMPerSecDataElement();
        case VerticalSpeedFtPerSec:
            return getVerticalSpeedFtPerSecDataElement();
        case VerticalSpeedFtPerMin:
            return getVerticalSpeedFtPerMinDataElement();
        case VerticalSpeedMPerSec:
            return getVerticalSpeedMPerSecDataElement();
        default:
            return 0;
        }
    }

private:
    template<double (HLAGeodeticLocation::*getter)() const,
             void (HLAGeodeticLocation::*setter)(double)>
    class DataElement : public HLAAbstractDoubleDataElement {
    public:
        DataElement(HLAGeodeticLocation* data) :
            _data(data)
        { }
        virtual double getValue() const
        { return (_data->*getter)(); }
        virtual void setValue(double value)
        { (_data->*setter)(value); }

    private:
        SGSharedPtr<HLAGeodeticLocation> _data;
    };

    void updateGeodeticFromCartesian() const
    {
        if (!_dirty)
            return;
        _geodPosition = SGGeod::fromCart(_cartPosition);
        SGQuatd geodOrientation = inverse(SGQuatd::fromLonLat(_geodPosition))*_cartOrientation;
        geodOrientation.getEulerRad(_geodEulerRad[2], _geodEulerRad[1], _geodEulerRad[0]);
        SGVec3d nedVel = geodOrientation.backTransform(_cartBodyVelocity);
        if (SGLimitsd::min() < SGMiscd::max(fabs(nedVel[0]), fabs(nedVel[1])))
            _groundTrackRad = atan2(nedVel[1], nedVel[0]);
        else
            _groundTrackRad = 0;
        _groundSpeedMPerSec = sqrt(nedVel[0]*nedVel[0] + nedVel[1]*nedVel[1]);
        _verticalSpeedMPerSec = -nedVel[2];
        _dirty = false;
    }
    void updateCartesianFromGeodetic() const
    {
        if (!_dirty)
            return;
        _cartPosition = SGVec3d::fromGeod(_geodPosition);
        SGQuatd geodOrientation = SGQuatd::fromEulerRad(_geodEulerRad[2], _geodEulerRad[1], _geodEulerRad[0]);
        _cartOrientation = SGQuatd::fromLonLat(_geodPosition)*geodOrientation;
        SGVec3d nedVel(cos(_groundTrackRad)*_groundSpeedMPerSec,
                       sin(_groundTrackRad)*_groundSpeedMPerSec,
                       -_verticalSpeedMPerSec);
        _cartBodyVelocity = geodOrientation.transform(nedVel);
        _dirty = false;
    }

    mutable bool _dirty;

    // the cartesian values
    mutable SGVec3d _cartPosition;
    mutable SGQuatd _cartOrientation;
    mutable SGVec3d _cartBodyVelocity;

    // The geodetic values
    mutable SGGeod _geodPosition;
    mutable SGVec3d _geodEulerRad;
    mutable double _groundTrackRad;
    mutable double _groundSpeedMPerSec;
    mutable double _verticalSpeedMPerSec;
};

class HLAGeodeticLocationFactory : public HLALocationFactory {
public:
    enum Semantic {
        LatitudeDeg = HLAGeodeticLocation::LatitudeDeg,
        LatitudeRad = HLAGeodeticLocation::LatitudeRad,
        LongitudeDeg = HLAGeodeticLocation::LongitudeDeg,
        LongitudeRad = HLAGeodeticLocation::LongitudeRad,
        ElevationM = HLAGeodeticLocation::ElevationM,
        ElevationFt = HLAGeodeticLocation::ElevationFt,
        HeadingDeg = HLAGeodeticLocation::HeadingDeg,
        HeadingRad = HLAGeodeticLocation::HeadingRad,
        PitchDeg = HLAGeodeticLocation::PitchDeg,
        PitchRad = HLAGeodeticLocation::PitchRad,
        RollDeg = HLAGeodeticLocation::RollDeg,
        RollRad = HLAGeodeticLocation::RollRad,
        GroundTrackDeg = HLAGeodeticLocation::GroundTrackDeg,
        GroundTrackRad = HLAGeodeticLocation::GroundTrackRad,
        GroundSpeedKnots = HLAGeodeticLocation::GroundSpeedKnots,
        GroundSpeedFtPerSec = HLAGeodeticLocation::GroundSpeedFtPerSec,
        GroundSpeedMPerSec = HLAGeodeticLocation::GroundSpeedMPerSec,
        VerticalSpeedFtPerSec = HLAGeodeticLocation::VerticalSpeedFtPerSec,
        VerticalSpeedFtPerMin = HLAGeodeticLocation::VerticalSpeedFtPerMin,
        VerticalSpeedMPerSec = HLAGeodeticLocation::VerticalSpeedMPerSec
    };

    virtual HLAGeodeticLocation* createLocation(HLAObjectInstance& objectInstance) const
    {
        HLAGeodeticLocation* location = new HLAGeodeticLocation;

        for (IndexSemanticMap::const_iterator i = _indexSemanticMap.begin();
             i != _indexSemanticMap.end(); ++i) {
            HLAGeodeticLocation::Semantic semantic = HLAGeodeticLocation::Semantic(i->second);
            objectInstance.setAttributeDataElement(i->first, location->getDataElement(semantic));
        }

        return location;
    }

    void setIndex(Semantic semantic, const HLADataElementIndex& index)
    { _indexSemanticMap[index] = semantic; }

private:
    typedef std::map<HLADataElementIndex, Semantic> IndexSemanticMap;
    IndexSemanticMap _indexSemanticMap;
};

} // namespace simgear

#endif
