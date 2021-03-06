// VPBVPBElevationSlice.hxx -- VirtualPlanetBuilder Elevation Slice
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


#ifndef VPBELEVATIONSLICE
#define VPBELEVATIONSLICE 1

#include <osgUtil/IntersectionVisitor>

// include so we can get access to the DatabaseCacheReadCallback
#include <osgSim/LineOfSight>

namespace simgear {

class VPBElevationSlice
{
    public :

        VPBElevationSlice();

        /** Set the start point of the slice.*/
        void setStartPoint(const osg::Vec3d& startPoint) { _startPoint = startPoint; }

        /** Get the start point of the slice.*/
        const osg::Vec3d& getStartPoint() const { return _startPoint; }

        /** Set the end point of the slice.*/
        void setEndPoint(const osg::Vec3d& endPoint) { _endPoint = endPoint; }

        /** Get the end point of the slice.*/
        const osg::Vec3d& getEndPoint() const { return _endPoint; }

        /** Set the up Vector of the slice.*/
        void setUpVector(const osg::Vec3d& upVector) { _upVector = upVector; }

        /** Get the start point of the slice.*/
        const osg::Vec3d& getUpVector() const { return _upVector; }


        typedef std::vector<osg::Vec3d> Vec3dList;

        /** Get the intersections in the form of a vector of Vec3d. */
        const Vec3dList& getIntersections() const  { return _intersections; }

        typedef std::pair<double,double> DistanceHeight;
        typedef std::vector<DistanceHeight> DistanceHeightList;

        /** Get the intersections in the form a vector of pair<double,double> representing distance along the slice and height. */
        const DistanceHeightList& getDistanceHeightIntersections() const { return _distanceHeightIntersections; }


        /** Compute the intersections with the specified scene graph, the results are stored in vectors of Vec3d.
          * Note, if the topmost node is a CoordinateSystemNode then the input points are assumed to be geocentric,
          * with the up vector defined by the EllipsoidModel attached to the CoordinateSystemNode.
          * If the topmost node is not a CoordinateSystemNode then a local coordinates frame is assumed, with a local up vector. */
        void computeIntersections(osg::Node* scene, osg::Node::NodeMask traversalMask=0xffffffff);

        /** Compute the vertical distance between the specified scene graph and a single HAT point.*/
        static Vec3dList computeVPBElevationSlice(osg::Node* scene, const osg::Vec3d& startPoint, const osg::Vec3d& endPoint, const osg::Vec3d& upVector, osg::Node::NodeMask traversalMask=0xffffffff);

    protected :
        osg::Vec3d                              _startPoint;
        osg::Vec3d                              _endPoint;
        osg::Vec3d                              _upVector;
        Vec3dList                               _intersections;
        DistanceHeightList                      _distanceHeightIntersections;

        osgUtil::IntersectionVisitor            _intersectionVisitor;
};

}

#endif
