// VPBVPBElevationSlice.cxx -- VirtualPlanetBuilder Elevation Slice
//
// Copyright (C) 2021 Stuart Buchanan
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#include <osg/CoordinateSystemNode>
#include <osg/io_utils>
#include <osg/Geode>
#include <osg/Geometry>

#include <simgear/debug/logstream.hxx>
#include <simgear/scene/tgdb/VPBElevationSlice.hxx>

#include <osg/Notify>
#include <osgUtil/PlaneIntersector>

#include <osgDB/WriteFile>

using namespace osgSim;

namespace simgear {

namespace VPBElevationSliceUtils
{

struct DistanceHeightXYZ
{

    DistanceHeightXYZ():
        distance(0.0),
        height(0.0) {}

    DistanceHeightXYZ(const DistanceHeightXYZ& dh):
        distance(dh.distance),
        height(dh.height),
        position(dh.position) {}

    DistanceHeightXYZ(double d, double h, const osg::Vec3d& pos):
        distance(d),
        height(h),
        position(pos) {}

    bool operator < (const DistanceHeightXYZ& rhs) const
    {
        // small distance values first
        if (distance < rhs.distance) return true;
        if (distance > rhs.distance) return false;

        // smallest heights first
        return (height < rhs.height);
    }

    bool operator == (const DistanceHeightXYZ& rhs) const
    {
        return distance==rhs.distance && height==rhs.height;
    }

    bool operator != (const DistanceHeightXYZ& rhs) const
    {
        return distance!=rhs.distance || height!=rhs.height;
    }

    bool equal_distance(const DistanceHeightXYZ& rhs, double epsilon=1e-6) const
    {
        return osg::absolute(rhs.distance - distance) <= epsilon;
    }

    double      distance;
    double      height;
    osg::Vec3d  position;
};

struct Point : public osg::Referenced, public DistanceHeightXYZ
{
    Point() {}
    Point(double d, double h, const osg::Vec3d& pos):
         DistanceHeightXYZ(d,h,pos)
    {
    }

    Point(const Point& point):
        osg::Referenced(),
        DistanceHeightXYZ(point) {}


};

struct Segment
{
    Segment(Point* p1, Point* p2)
    {
        if (*p1 < *p2)
        {
            _p1 = p1;
            _p2 = p2;
        }
        else
        {
            _p1 = p2;
            _p2 = p1;
        }
    }

    bool operator < ( const Segment& rhs) const
    {
        if (*_p1 < *rhs._p1) return true;
        if (*rhs._p1 < *_p1) return false;

        return (*_p2 < *rhs._p2);
    }

    enum Classification
    {
        UNCLASSIFIED,
        IDENTICAL,
        SEPARATE,
        JOINED,
        OVERLAPPING,
        ENCLOSING,
        ENCLOSED
    };

    Classification compare(const Segment& rhs) const
    {
        if (*_p1 == *rhs._p1 && *_p2==*rhs._p2) return IDENTICAL;

        const double epsilon = 1e-3; // 1mm

        double delta_distance = _p2->distance - rhs._p1->distance;
        if (fabs(delta_distance) < epsilon)
        {
            if (fabs(_p2->height - rhs._p1->height) < epsilon) return JOINED;
        }

        if (delta_distance==0.0)
        {
            return SEPARATE;
        }

        if (rhs._p2->distance < _p1->distance || _p2->distance < rhs._p1->distance) return SEPARATE;

        bool rhs_p1_inside = (_p1->distance <= rhs._p1->distance) && (rhs._p1->distance <= _p2->distance);
        bool rhs_p2_inside = (_p1->distance <= rhs._p2->distance) && (rhs._p2->distance <= _p2->distance);

        if (rhs_p1_inside && rhs_p2_inside) return ENCLOSING;

        bool p1_inside = (rhs._p1->distance <= _p1->distance) && (_p1->distance <= rhs._p2->distance);
        bool p2_inside = (rhs._p1->distance <= _p2->distance) && (_p2->distance <= rhs._p2->distance);

        if (p1_inside && p2_inside) return ENCLOSED;

        if (rhs_p1_inside || rhs_p2_inside || p1_inside || p2_inside) return OVERLAPPING;

        return UNCLASSIFIED;
    }

    double height(double d) const
    {
        double delta = (_p2->distance - _p1->distance);
        return _p1->height + ((_p2->height - _p1->height) * (d - _p1->distance) / delta);
    }

    double deltaHeight(Point& point) const
    {
        return point.height - height(point.distance);
    }

    Point* createPoint(double d) const
    {
        if (d == _p1->distance) return _p1.get();
        if (d == _p2->distance) return _p2.get();

        double delta = (_p2->distance - _p1->distance);
        double r = (d - _p1->distance)/delta;
        double one_minus_r = 1.0 - r;
        return new Point(d,
                         _p1->height * one_minus_r + _p2->height * r,
                         _p1->position * one_minus_r + _p2->position * r);
    }

    Point* createIntersectionPoint(const Segment& rhs) const
    {
        double A = _p1->distance;
        double B = _p2->distance - _p1->distance;
        double C = _p1->height;
        double D = _p2->height - _p1->height;

        double E = rhs._p1->distance;
        double F = rhs._p2->distance - rhs._p1->distance;
        double G = rhs._p1->height;
        double H = rhs._p2->height - rhs._p1->height;

        double div = D*F - B*H;
        if (div==0.0)
        {
            return _p2.get();
        }

        double r = (G*F - E*H + A*H - C*F) / div;

        if (r<0.0)
        {
            return _p1.get();
        }

        if (r>1.0)
        {
            return _p2.get();
        }

        return new Point(A + B*r, C + D*r, _p1->position + (_p2->position - _p1->position)*r);
    }


    osg::ref_ptr<Point> _p1;
    osg::ref_ptr<Point> _p2;

};


struct LineConstructor
{

    typedef std::set<Segment> SegmentSet;

    LineConstructor() {}



    void add(double d, double h, const osg::Vec3d& pos)
    {
        osg::ref_ptr<Point> newPoint = new Point(d,h,pos);


        if (_previousPoint.valid() && newPoint->distance != _previousPoint->distance)
        {
            const double maxGradient = 100.0;
            double gradient = fabs( (newPoint->height - _previousPoint->height) / (newPoint->distance - _previousPoint->distance) );

            if (gradient < maxGradient)
            {
                _segments.insert( Segment(_previousPoint.get(), newPoint.get()) );
            }
        }

        _previousPoint = newPoint;
    }

    void endline()
    {
        _previousPoint = 0;
    }

    void report()
    {
    }

    void pruneOverlappingSegments()
    {
        double epsilon = 0.001;

        for(SegmentSet::iterator itr = _segments.begin();
            itr != _segments.end();
            ) // itr increment is done manually at end of loop
        {
            SegmentSet::iterator nextItr = itr;
            ++nextItr;
            Segment::Classification classification = nextItr != _segments.end() ?  itr->compare(*nextItr) : Segment::UNCLASSIFIED;

            while (classification>=Segment::OVERLAPPING)
            {

                switch(classification)
                {
                    case(Segment::OVERLAPPING):
                    {
                        // cases....
                        // compute new end points for both segments
                        // need to work out which points are overlapping - lhs_p2 && rhs_p1  or  lhs_p1 and rhs_p2
                        // also need to check for cross cases.

                        const Segment& lhs = *itr;
                        const Segment& rhs = *nextItr;

                        bool rhs_p1_inside = (lhs._p1->distance <= rhs._p1->distance) && (rhs._p1->distance <= lhs._p2->distance);
                        bool lhs_p2_inside = (rhs._p1->distance <= lhs._p2->distance) && (lhs._p2->distance <= rhs._p2->distance);

                        if (rhs_p1_inside && lhs_p2_inside)
                        {
                            double distance_between = osg::Vec2d(lhs._p2->distance - rhs._p1->distance,
                                                                 lhs._p2->height - rhs._p1->height).length2();

                            if (distance_between < epsilon)
                            {
                                Segment newSeg(lhs._p2.get(), rhs._p2.get());
                                _segments.insert(newSeg);

                                _segments.erase(nextItr);

                                nextItr = _segments.find(newSeg);
                            }
                            else
                            {
                                double dh1 = lhs.deltaHeight(*rhs._p1);
                                double dh2 = -rhs.deltaHeight(*lhs._p2);

                                if (dh1 * dh2 < 0.0)
                                {
                                    Point* cp = lhs.createIntersectionPoint(rhs);

                                    Segment seg1( lhs._p1.get(), lhs.createPoint(rhs._p2->distance) );
                                    Segment seg2( rhs._p1.get(), cp );
                                    Segment seg3( cp, lhs._p2.get() );
                                    Segment seg4( rhs.createPoint(lhs._p2->distance), lhs._p2.get() );

                                    _segments.erase(nextItr);
                                    _segments.erase(itr);

                                    _segments.insert(seg1);
                                    _segments.insert(seg2);
                                    _segments.insert(seg3);
                                    _segments.insert(seg4);

                                    itr = _segments.find(seg1);
                                    nextItr = itr;
                                    ++nextItr;

                                }
                                else if (dh1 <= 0.0 && dh2 <= 0.0)
                                {
                                    Segment newSeg(rhs.createPoint(lhs._p2->distance), rhs._p2.get());

                                    _segments.erase(nextItr);
                                    _segments.insert(newSeg);
                                    nextItr = itr;
                                    ++nextItr;

                                 }
                                else if (dh1 >= 0.0 && dh2 >= 0.0)
                                {
                                    Segment newSeg(lhs._p1.get(), lhs.createPoint(rhs._p1->distance));

                                    _segments.erase(itr);
                                    _segments.insert(newSeg);
                                    itr = _segments.find(newSeg);
                                    nextItr = itr;
                                    ++nextItr;
                                }
                                else
                                {
                                    ++nextItr;
                                }
                            }
                        }
                        else
                        {
                            ++nextItr;
                        }


                        break;
                    }
                    case(Segment::ENCLOSING):
                    {
                        // need to work out if rhs is below lhs or rhs is above lhs or crossing

                        const Segment& enclosing = *itr;
                        const Segment& enclosed = *nextItr;
                        double dh1 = enclosing.deltaHeight(*enclosed._p1);
                        double dh2 = enclosing.deltaHeight(*enclosed._p2);

                        if (dh1<=epsilon && dh2<=epsilon)
                        {
                            _segments.erase(nextItr);

                            nextItr = itr;
                            ++nextItr;

                        }
                        else if (dh1>=0.0 && dh2>=0.0)
                        {

                            double d_left = enclosed._p1->distance - enclosing._p1->distance;
                            double d_right = enclosing._p2->distance - enclosed._p2->distance;

                            if (d_left < epsilon && d_right < epsilon)
                            {
                                // treat ENCLOSED as ENCLOSING.

                                nextItr = itr;
                                ++nextItr;

                                _segments.erase(itr);

                                itr = nextItr;
                                ++nextItr;

                            }
                            else if (d_left < epsilon)
                            {
                                Segment newSeg(enclosing.createPoint(enclosed._p2->distance), enclosing._p2.get());
                                nextItr = itr;
                                ++nextItr;

                                _segments.erase(itr);
                                _segments.insert(newSeg);

                                itr = nextItr;
                                ++nextItr;
                            }
                            else if (d_right < epsilon)
                            {
                                Segment newSeg(enclosing._p1.get(), enclosing.createPoint(enclosed._p1->distance) );

                                _segments.insert(newSeg);
                                _segments.erase(itr);

                                itr = _segments.find(newSeg);
                                nextItr = itr;
                                ++nextItr;
                            }
                            else
                            {
                                Segment newSegLeft(enclosing._p1.get(), enclosing.createPoint(enclosed._p1->distance) );
                                Segment newSegRight(enclosing.createPoint(enclosed._p2->distance), enclosing._p2.get());

                                _segments.erase(itr);
                                _segments.insert(newSegLeft);
                                _segments.insert(newSegRight);

                                itr = _segments.find(newSegLeft);
                                nextItr = itr;
                                ++nextItr;
                            }

                        }
                        else if (dh1 * dh2 < 0.0)
                        {
                            double d_left = enclosed._p1->distance - enclosing._p1->distance;
                            double d_right = enclosing._p2->distance - enclosed._p2->distance;

                            if (d_left < epsilon && d_right < epsilon)
                            {
                                // treat ENCLOSED as ENCLOSING.
                                if (dh1 < 0.0)
                                {
                                    Point* cp = enclosing.createIntersectionPoint(enclosed);
                                    Segment newSegLeft(enclosing._p1.get(), cp);
                                    Segment newSegRight(cp, enclosed._p2.get());

                                    _segments.erase(itr);
                                    _segments.erase(nextItr);

                                    _segments.insert(newSegLeft);
                                    _segments.insert(newSegRight);

                                    itr = _segments.find(newSegLeft);
                                    nextItr = itr;
                                    ++nextItr;
                                }
                                else
                                {
                                    Point* cp = enclosing.createIntersectionPoint(enclosed);
                                    Segment newSegLeft(enclosed._p1.get(), cp);
                                    Segment newSegRight(cp, enclosing._p2.get());

                                    _segments.erase(itr);
                                    _segments.erase(nextItr);

                                    _segments.insert(newSegLeft);
                                    _segments.insert(newSegRight);

                                    itr = _segments.find(newSegLeft);
                                    nextItr = itr;
                                    ++nextItr;
                                }

                            }
                            else if (d_left < epsilon)
                            {
                                if (dh1 < 0.0)
                                {
                                    Point* cp = enclosing.createIntersectionPoint(enclosed);
                                    Segment newSegLeft(enclosing._p1.get(), cp);
                                    Segment newSegMid(cp, enclosed._p2.get());
                                    Segment newSegRight(enclosing.createPoint(enclosed._p2->distance), enclosing._p2.get());

                                    _segments.erase(itr);
                                    _segments.erase(nextItr);

                                    _segments.insert(newSegLeft);
                                    _segments.insert(newSegMid);
                                    _segments.insert(newSegRight);

                                    itr = _segments.find(newSegLeft);
                                    nextItr = itr;
                                    ++nextItr;
                                }
                                else
                                {
                                    Point* cp = enclosing.createIntersectionPoint(enclosed);
                                    Segment newSegLeft(enclosed._p1.get(), cp);
                                    Segment newSegRight(cp, enclosing._p2.get());

                                    _segments.erase(itr);
                                    _segments.erase(nextItr);

                                    _segments.insert(newSegLeft);
                                    _segments.insert(newSegRight);

                                    itr = _segments.find(newSegLeft);
                                    nextItr = itr;
                                    ++nextItr;
                                }
                            }
                            else if (d_right < epsilon)
                            {
                                if (dh1 < 0.0)
                                {
                                    Point* cp = enclosing.createIntersectionPoint(enclosed);
                                    Segment newSegLeft(enclosing._p1.get(), cp);
                                    Segment newSegRight(cp, enclosed._p2.get());

                                    _segments.erase(itr);
                                    _segments.erase(nextItr);

                                    _segments.insert(newSegLeft);
                                    _segments.insert(newSegRight);

                                    itr = _segments.find(newSegLeft);
                                    nextItr = itr;
                                    ++nextItr;
                                }
                                else
                                {
                                    Point* cp = enclosing.createIntersectionPoint(enclosed);
                                    Segment newSegLeft(enclosing._p1.get(), enclosing.createPoint(enclosed._p1->distance));
                                    Segment newSegMid(enclosed._p1.get(), cp);
                                    Segment newSegRight(cp, enclosing._p2.get());

                                    _segments.erase(itr);
                                    _segments.erase(nextItr);

                                    _segments.insert(newSegLeft);
                                    _segments.insert(newSegMid);
                                    _segments.insert(newSegRight);

                                    itr = _segments.find(newSegLeft);
                                    nextItr = itr;
                                    ++nextItr;
                                }
                            }
                            else
                            {
                                if (dh1 < 0.0)
                                {
                                    Point* cp = enclosing.createIntersectionPoint(enclosed);
                                    Segment newSegLeft(enclosing._p1.get(), cp);
                                    Segment newSegMid(cp, enclosed._p2.get());
                                    Segment newSegRight(enclosing.createPoint(enclosed._p2->distance), enclosing._p2.get());

                                    _segments.erase(itr);
                                    _segments.erase(nextItr);

                                    _segments.insert(newSegLeft);
                                    _segments.insert(newSegMid);
                                    _segments.insert(newSegRight);

                                    itr = _segments.find(newSegLeft);
                                    nextItr = itr;
                                    ++nextItr;
                                }
                                else
                                {
                                    Point* cp = enclosing.createIntersectionPoint(enclosed);
                                    Segment newSegLeft(enclosing._p1.get(), enclosing.createPoint(enclosed._p1->distance));
                                    Segment newSegMid(enclosed._p1.get(), cp);
                                    Segment newSegRight(cp, enclosing._p2.get());

                                    _segments.erase(itr);
                                    _segments.erase(nextItr);

                                    _segments.insert(newSegLeft);
                                    _segments.insert(newSegMid);
                                    _segments.insert(newSegRight);

                                    itr = _segments.find(newSegLeft);
                                    nextItr = itr;
                                    ++nextItr;
                                }

                            }
                        }
                        else
                        {
                            ++nextItr;
                        }

                        break;
                    }
                    case(Segment::ENCLOSED):
                    {
                        // need to work out if lhs is below rhs or lhs is above rhs or crossing
                        const Segment& enclosing = *nextItr;
                        const Segment& enclosed = *itr;
                        double dh1 = enclosing.deltaHeight(*enclosed._p1);
                        double dh2 = enclosing.deltaHeight(*enclosed._p2);

                        double d_left = enclosed._p1->distance - enclosing._p1->distance;
                        double d_right = enclosing._p2->distance - enclosed._p2->distance;

                        if (d_left<=epsilon)
                        {

                            if (dh1<=epsilon && dh2<=epsilon)
                            {
                                _segments.erase(itr);

                                itr = nextItr;
                                ++nextItr;
                            }
                            else if (dh1>=0.0 && dh2>=0.0)
                            {
                                _segments.insert(Segment(enclosing.createPoint(enclosed._p2->distance), enclosed._p2.get()));
                                _segments.erase(nextItr);

                                nextItr = itr;
                                ++nextItr;
                            }
                            else if (dh1 * dh2 < 0.0)
                            {
                                if (d_right<=epsilon)
                                {
                                    // enclosed and enclosing effectively overlap
                                    if (dh1 < 0.0)
                                    {
                                        Point* cp = enclosed.createIntersectionPoint(enclosing);
                                        Segment segLeft(enclosed._p1.get(), cp);
                                        Segment segRight(cp, enclosing._p2.get());

                                        _segments.erase(itr);
                                        _segments.erase(nextItr);

                                        _segments.insert(segLeft);
                                        _segments.insert(segRight);

                                        itr = _segments.find(segLeft);
                                        nextItr = itr;
                                        ++nextItr;
                                    }
                                    else
                                    {
                                        Point* cp = enclosed.createIntersectionPoint(enclosing);
                                        Segment segLeft(enclosing._p1.get(), cp);
                                        Segment segRight(cp, enclosed._p2.get());

                                        _segments.erase(itr);
                                        _segments.erase(nextItr);

                                        _segments.insert(segLeft);
                                        _segments.insert(segRight);

                                        itr = _segments.find(segLeft);
                                        nextItr = itr;
                                        ++nextItr;
                                    }
                                }
                                else
                                {
                                    // right hand side needs to be created
                                    if (dh1 < 0.0)
                                    {
                                        Point* cp = enclosed.createIntersectionPoint(enclosing);
                                        Segment segLeft(enclosed._p1.get(), cp);
                                        Segment segRight(cp, enclosing._p2.get());

                                        _segments.erase(itr);
                                        _segments.erase(nextItr);

                                        _segments.insert(segLeft);
                                        _segments.insert(segRight);

                                        itr = _segments.find(segLeft);
                                        nextItr = itr;
                                        ++nextItr;
                                    }
                                    else
                                    {
                                        Point* cp = enclosed.createIntersectionPoint(enclosing);
                                        Segment segLeft(enclosing._p1.get(), cp);
                                        Segment segMid(cp, enclosed._p2.get());
                                        Segment segRight(enclosing.createPoint(enclosed._p2->distance), enclosing._p2.get());

                                        _segments.erase(itr);
                                        _segments.erase(nextItr);

                                        _segments.insert(segLeft);
                                        _segments.insert(segMid);
                                        _segments.insert(segRight);

                                        itr = _segments.find(segLeft);
                                        nextItr = itr;
                                        ++nextItr;
                                    }
                                }
                            }
                            else
                            {
                                ++nextItr;
                            }
                        }
                        else
                        {
                            SG_LOG(SG_TERRAIN, SG_ALERT, "*** ENCLOSED: is not coincendet with left handside of ENCLOSING, case not handled, advancing.");
                        }

                        break;
                    }
                    default:
                        SG_LOG(SG_TERRAIN, SG_ALERT, "** Not handled, advancing");
                        ++nextItr;
                        break;
                }

                classification = ((itr != _segments.end()) && (nextItr != _segments.end())) ?  itr->compare(*nextItr) : Segment::UNCLASSIFIED;
            }

            // increment iterator it it's not already at end of container.
            if (itr!=_segments.end()) ++itr;
        }
    }

    unsigned int numOverlapping(SegmentSet::const_iterator startItr) const
    {
        if (startItr==_segments.end()) return 0;

        SegmentSet::const_iterator nextItr = startItr;
        ++nextItr;

        unsigned int num = 0;
        while (nextItr!=_segments.end() && startItr->compare(*nextItr)>=Segment::OVERLAPPING)
        {
            ++num;
            ++nextItr;
        }
        return num;
    }

    unsigned int totalNumOverlapping() const
    {
        unsigned int total = 0;
        for(SegmentSet::const_iterator itr = _segments.begin();
            itr != _segments.end();
            ++itr)
        {
            total += numOverlapping(itr);
        }
        return total;
    }

    void copyPoints(VPBElevationSlice::Vec3dList& intersections, VPBElevationSlice::DistanceHeightList& distanceHeightIntersections)
    {
        if (_segments.empty()) return;

        SegmentSet::iterator prevItr = _segments.begin();
        SegmentSet::iterator nextItr = prevItr;
        ++nextItr;

        intersections.push_back( prevItr->_p1->position );
        distanceHeightIntersections.push_back( VPBElevationSlice::DistanceHeight(prevItr->_p1->distance, prevItr->_p1->height) );

        intersections.push_back( prevItr->_p2->position );
        distanceHeightIntersections.push_back( VPBElevationSlice::DistanceHeight(prevItr->_p2->distance, prevItr->_p2->height) );

        for(;
            nextItr != _segments.end();
            ++nextItr,++prevItr)
        {
            Segment::Classification classification = prevItr->compare(*nextItr);
            switch(classification)
            {
                case(Segment::SEPARATE):
                {
                    intersections.push_back( nextItr->_p1->position );
                    distanceHeightIntersections.push_back( VPBElevationSlice::DistanceHeight(nextItr->_p1->distance, nextItr->_p1->height) );

                    intersections.push_back( nextItr->_p2->position );
                    distanceHeightIntersections.push_back( VPBElevationSlice::DistanceHeight(nextItr->_p2->distance, nextItr->_p2->height) );
                    break;
                }
                case(Segment::JOINED):
                {
#if 1
                    intersections.push_back( nextItr->_p2->position );
                    distanceHeightIntersections.push_back( VPBElevationSlice::DistanceHeight(nextItr->_p2->distance, nextItr->_p2->height) );
#else
                    intersections.push_back( nextItr->_p1->position );
                    distanceHeightIntersections.push_back( VPBElevationSlice::DistanceHeight(nextItr->_p1->distance, nextItr->_p1->height) );

                    intersections.push_back( nextItr->_p2->position );
                    distanceHeightIntersections.push_back( VPBElevationSlice::DistanceHeight(nextItr->_p2->distance, nextItr->_p2->height) );
#endif
                    break;
                }
                default:
                {
                    intersections.push_back( nextItr->_p1->position );
                    distanceHeightIntersections.push_back( VPBElevationSlice::DistanceHeight(nextItr->_p1->distance, nextItr->_p1->height) );

                    intersections.push_back( nextItr->_p2->position );
                    distanceHeightIntersections.push_back( VPBElevationSlice::DistanceHeight(nextItr->_p2->distance, nextItr->_p2->height) );
                    break;
                }
            }

        }

    }

    SegmentSet _segments;
    osg::ref_ptr<Point> _previousPoint;
    osg::Plane  _plane;
    osg::ref_ptr<osg::EllipsoidModel>   _em;

};

}

VPBElevationSlice::VPBElevationSlice()
{
    /** Remove the osgSim::DatabaseCacheReadCallback that enables automatic loading
     * of external PagedLOD tiles to ensure that the highest level of detail is used in intersections.
     * This automatic loading of tiles is done by the intersection traversal that is done within
     * the computeIntersections(..) method, so can result in long intersection times when external
     * tiles have to be loaded.
     * The external loading of tiles can be disabled by removing the read callback, this is done by
     * calling the setDatabaseCacheReadCallback(DatabaseCacheReadCallback*) method with a value of 0.*/
    _intersectionVisitor.setReadCallback(0);
}

void VPBElevationSlice::computeIntersections(osg::Node* scene, osg::Node::NodeMask traversalMask)
{
    osg::Plane plane;
    osg::Polytope boundingPolytope;


    // set up the main intersection plane
    osg::Vec3d planeNormal = (_endPoint - _startPoint) ^ _upVector;
    planeNormal.normalize();
    plane.set( planeNormal, _startPoint );

    // set up the start cut off plane
    osg::Vec3d startPlaneNormal = _upVector ^ planeNormal;
    startPlaneNormal.normalize();
    boundingPolytope.add( osg::Plane(startPlaneNormal, _startPoint) );

    // set up the end cut off plane
    osg::Vec3d endPlaneNormal = planeNormal ^ _upVector;
    endPlaneNormal.normalize();
    boundingPolytope.add( osg::Plane(endPlaneNormal, _endPoint) );

    osg::ref_ptr<osgUtil::PlaneIntersector> intersector = new osgUtil::PlaneIntersector(plane, boundingPolytope);


    intersector->setRecordHeightsAsAttributes(true);

    _intersectionVisitor.reset();
    _intersectionVisitor.setTraversalMask(traversalMask);
    _intersectionVisitor.setIntersector( intersector.get() );

    scene->accept(_intersectionVisitor);

    osgUtil::PlaneIntersector::Intersections& intersections = intersector->getIntersections();

    typedef osgUtil::PlaneIntersector::Intersection::Polyline Polyline;
    typedef osgUtil::PlaneIntersector::Intersection::Attributes Attributes;

    if (!intersections.empty())
    {

        osgUtil::PlaneIntersector::Intersections::iterator itr;
        for(itr = intersections.begin();
            itr != intersections.end();
            ++itr)
        {
            osgUtil::PlaneIntersector::Intersection& intersection = *itr;

            if (intersection.matrix.valid())
            {
                // transform points on polyline
                for(Polyline::iterator pitr = intersection.polyline.begin();
                    pitr != intersection.polyline.end();
                    ++pitr)
                {
                    *pitr = (*pitr) * (*intersection.matrix);
                }

                // matrix no longer needed.
                intersection.matrix = 0;
            }
        }

        VPBElevationSliceUtils::LineConstructor constructor;
        constructor._plane = plane;

        // convert into distance/height
        for(itr = intersections.begin();
            itr != intersections.end();
            ++itr)
        {
            osgUtil::PlaneIntersector::Intersection& intersection = *itr;
            for(Polyline::iterator pitr = intersection.polyline.begin();
                pitr != intersection.polyline.end();
                ++pitr)
            {
                const osg::Vec3d& v = *pitr;
                osg::Vec2d delta_xy( v.x() - _startPoint.x(), v.y() - _startPoint.y());
                double distance = delta_xy.length();

                constructor.add( distance, v.z(), v);
            }
            constructor.endline();
        }

        // copy final results
        _intersections.clear();
        _distanceHeightIntersections.clear();

        // constructor.report();

        unsigned int numOverlapping = constructor.totalNumOverlapping();

        while(numOverlapping>0)
        {
            unsigned int previousNumOverlapping = numOverlapping;

            constructor.pruneOverlappingSegments();
            // constructor.report();

            numOverlapping = constructor.totalNumOverlapping();
            if (previousNumOverlapping == numOverlapping) break;
        }

        constructor.copyPoints(_intersections, _distanceHeightIntersections);
    }
    else
    {
        SG_LOG(SG_TERRAIN, SG_DEBUG, "No intersections found.");
    }

}

VPBElevationSlice::Vec3dList VPBElevationSlice::computeVPBElevationSlice(osg::Node* scene, const osg::Vec3d& startPoint, const osg::Vec3d& endPoint, const osg::Vec3d& upVector, osg::Node::NodeMask traversalMask)
{
    VPBElevationSlice es;
    es.setStartPoint(startPoint);
    es.setEndPoint(endPoint);
    es.setUpVector(upVector);
    es.computeIntersections(scene, traversalMask);
    return es.getIntersections();
}

}