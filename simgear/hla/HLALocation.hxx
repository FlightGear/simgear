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

#include <simgear/math/SGMath.hxx>
#include "HLABasicDataElement.hxx"
#include "HLATypes.hxx"

namespace simgear {

class HLAObjectInstance;

class HLAAbstractLocation : public SGReferenced {
public:
    virtual ~HLAAbstractLocation() {}

    virtual SGLocationd getLocation() const = 0;
    virtual void setLocation(const SGLocationd&) = 0;

    virtual SGVec3d getCartPosition() const = 0;
    virtual void setCartPosition(const SGVec3d&) = 0;

    virtual SGQuatd getCartOrientation() const = 0;
    virtual void setCartOrientation(const SGQuatd&) = 0;

    virtual SGVec3d getAngularBodyVelocity() const = 0;
    virtual void setAngularBodyVelocity(const SGVec3d&) = 0;

    virtual SGVec3d getLinearBodyVelocity() const = 0;
    virtual void setLinearBodyVelocity(const SGVec3d&) = 0;

    virtual double getTimeDifference(const SGTimeStamp&) const
    { return 0; }

    // Get the position and orientation extrapolated to the given time stamp.
    SGLocationd getLocation(const SGTimeStamp& timeStamp) const
    {
        SGLocationd location = getLocation();
        location.eulerStepBodyVelocitiesMidOrientation(getTimeDifference(timeStamp), getLinearBodyVelocity(), getAngularBodyVelocity());
        return location;
    }
};

class HLACartesianLocation : public HLAAbstractLocation {
public:
    HLACartesianLocation();
    virtual ~HLACartesianLocation();

    virtual SGLocationd getLocation() const;
    virtual void setLocation(const SGLocationd& location);

    virtual SGVec3d getCartPosition() const;
    virtual void setCartPosition(const SGVec3d& position);

    virtual SGQuatd getCartOrientation() const;
    virtual void setCartOrientation(const SGQuatd& orientation);

    virtual SGVec3d getAngularBodyVelocity() const;
    virtual void setAngularBodyVelocity(const SGVec3d& angularVelocity);

    virtual SGVec3d getLinearBodyVelocity() const;
    virtual void setLinearBodyVelocity(const SGVec3d& linearVelocity);

    virtual double getTimeDifference(const SGTimeStamp& timeStamp) const;

    HLADataElement* getPositionDataElement();
    HLADataElement* getPositionDataElement(unsigned i);
    HLADataElement* getOrientationDataElement();
    HLADataElement* getOrientationDataElement(unsigned i);
    HLADataElement* getAngularVelocityDataElement();
    HLADataElement* getAngularVelocityDataElement(unsigned i);
    HLADataElement* getLinearVelocityDataElement();
    HLADataElement* getLinearVelocityDataElement(unsigned i);

private:
    HLADoubleData _position[3];
    HLADoubleData _imag[3];

    HLADoubleData _angularVelocity[3];
    HLADoubleData _linearVelocity[3];
};

class HLAAbstractLocationFactory : public SGReferenced {
public:
    virtual ~HLAAbstractLocationFactory();
    virtual HLAAbstractLocation* createLocation(HLAObjectInstance&) const = 0;
};

typedef HLAAbstractLocationFactory HLALocationFactory;

class HLACartesianLocationFactory : public HLAAbstractLocationFactory {
public:
    HLACartesianLocationFactory();
    virtual ~HLACartesianLocationFactory();

    virtual HLACartesianLocation* createLocation(HLAObjectInstance& objectInstance) const;

    void setPositionIndex(const HLADataElementIndex& dataElementIndex);
    void setPositionIndex(unsigned index, const HLADataElementIndex& dataElementIndex);
    void setOrientationIndex(const HLADataElementIndex& dataElementIndex);
    void setOrientationIndex(unsigned index, const HLADataElementIndex& dataElementIndex);

    void setAngularVelocityIndex(const HLADataElementIndex& dataElementIndex);
    void setAngularVelocityIndex(unsigned index, const HLADataElementIndex& dataElementIndex);
    void setLinearVelocityIndex(const HLADataElementIndex& dataElementIndex);
    void setLinearVelocityIndex(unsigned index, const HLADataElementIndex& dataElementIndex);

private:
    HLADataElementIndex _positionIndex[3];
    HLADataElementIndex _orientationIndex[3];

    HLADataElementIndex _angularVelocityIndex[3];
    HLADataElementIndex _linearVelocityIndex[3];
};

class HLAGeodeticLocationFactory : public HLAAbstractLocationFactory {
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

    HLAGeodeticLocationFactory();
    virtual ~HLAGeodeticLocationFactory();

    virtual HLAAbstractLocation* createLocation(HLAObjectInstance& objectInstance) const;

    void setIndex(Semantic semantic, const HLADataElementIndex& index)
    { _indexSemanticMap[index] = semantic; }

private:
    class Location;

    typedef std::map<HLADataElementIndex, Semantic> IndexSemanticMap;
    IndexSemanticMap _indexSemanticMap;
};

} // namespace simgear

#endif
