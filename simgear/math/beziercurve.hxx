// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2009 Frederic Bouvier

#ifndef SIMGEAR_BEZIERCURVE_HXX
#define SIMGEAR_BEZIERCURVE_HXX 1

#include <list>
using std::list;

namespace simgear
{
  template<class T>
  class BezierCurve {
  public:
    typedef list<T> PointList;

    BezierCurve() : mMaxSubdiv( 3 ) {}
    BezierCurve( size_t aMaxSubdiv )
      : mMaxSubdiv( aMaxSubdiv ) {}
    BezierCurve( const T &p1, const T &p2, const T &p3, size_t aMaxSubdiv = 3 )
      : mMaxSubdiv( aMaxSubdiv ) {
        subdivide( p1, p2, p3 );
      }
    BezierCurve( const T &p1, const T &p2, const T &p3, const T &p4, size_t aMaxSubdiv = 3 )
      : mMaxSubdiv( aMaxSubdiv ) {
        subdivide( p1, p2, p3, p4 );
      }

    void subdivide( const T &p1, const T &p2, const T &p3 ) {
      mPointList.clear();
      mPointList.push_back( p1 );
      recursiveSubdivide( p1, p2, p3, 1 );
      mPointList.push_back( p3 );
    }

    void subdivide( const T &p1, const T &p2, const T &p3, const T &p4 ) {
      mPointList.clear();
      mPointList.push_back( p1 );
      recursiveSubdivide( p1, p2, p3, p4, 1 );
      mPointList.push_back( p4 );
    }

    void setMaxSubdiv( size_t aMaxSubdiv ) { mMaxSubdiv = aMaxSubdiv; }
    size_t getMaxSubdiv() const { return mMaxSubdiv; }
    PointList &pointList() { return mPointList; }
    const PointList &pointList() const { return mPointList; }

  private:
    T midPoint( const T &p1, const T &p2 ) {
      return ( p1 + p2 ) / 2;
    }
    bool recursiveSubdivide( const T &p1, const T &p2, const T &p3, size_t l ) {
      if ( l > mMaxSubdiv )
        return false;

      T p12 = midPoint( p1, p2 ),
        p23 = midPoint( p2, p3 ),
        p123 = midPoint( p12, p23 );
      recursiveSubdivide( p1, p12, p123, l + 1 );
      mPointList.push_back( p123 );
      recursiveSubdivide( p123, p23, p3, l + 1 );
      return true;
    }

    bool recursiveSubdivide( const T &p1, const T &p2, const T &p3, const T &p4, size_t l ) {
      if ( l > mMaxSubdiv )
        return false;

      T p12 = midPoint( p1, p2 ),
        p23 = midPoint( p2, p3 ),
        p34 = midPoint( p3, p4 ),
        p123 = midPoint( p12, p23 ),
        p234 = midPoint( p23, p34 ),
        p1234 = midPoint( p123, p234 );
      recursiveSubdivide( p1, p12, p123, p1234, l + 1 );
      mPointList.push_back( p1234 );
      recursiveSubdivide( p1234, p234, p34, p4, l + 1 );
      return true;
    }


    PointList mPointList;
    size_t mMaxSubdiv;
  };
}

#endif
