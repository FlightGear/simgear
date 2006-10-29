// vasi.hxx -- a class to hold some critical vasi data
//
// Written by Curtis Olson, started December 2003.
//
// Copyright (C) 2003  Curtis L. Olson  - http://www.flightgear.org/~curt
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
//
// $Id$


#ifndef _SG_VASI_HXX
#define _SG_VASI_HXX

#ifndef __cplusplus
# error This library requires C++
#endif

#include <simgear/compiler.h>

#include <osg/io_utils>
#include <osg/ref_ptr>
#include <osg/Array>
#include <osg/NodeCallback>
#include <osg/NodeVisitor>
#include <osg/Vec3>

/// Callback that updates the colors of a VASI according to the view direction
/// Notet that we need the eyepoint which is only available during culling
/// So this will be a cull callback ...
class SGVasiUpdateCallback : public osg::NodeCallback {
public:
  SGVasiUpdateCallback(osg::Vec4Array* vasiColorArray,
                       const osg::Vec3& referencePoint,
                       const osg::Vec3& glideSlopeUp,
                       const osg::Vec3& glideSlopeDir) :
    mVasiColorArray(vasiColorArray),
    mReferencePoint(referencePoint),
    mGlideSlopeUp(glideSlopeUp),
    mGlideSlopeDir(glideSlopeDir)
  {
    mGlideSlopeUp.normalize();
    mGlideSlopeDir.normalize();
  }
  virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
  {
    // Rerieve the eye point relative to the vasi reference point
    osg::Vec3 eyePoint = nv->getEyePoint() - mReferencePoint;
    // Now project the eye point vector into the plane defined by the
    // glideslope direction and the up direction
    osg::Vec3 normal = mGlideSlopeUp^mGlideSlopeDir;
    normal.normalize();
    osg::Vec3 projEyePoint = eyePoint - normal * (eyePoint*normal);

    double projEyePointLength = projEyePoint.length();
    if (fabs(projEyePointLength) < 1e-3)
      set_color(3);
    else {
      double aSinAngle = projEyePoint*mGlideSlopeUp/projEyePointLength;
      if (aSinAngle < -1)
        aSinAngle = -1;
      if (1 < aSinAngle)
        aSinAngle = 1;
      
      double angle = asin(aSinAngle)*SGD_RADIANS_TO_DEGREES;
      set_color(angle);
    }

    // call the base implementation
    osg::NodeCallback::operator()(node, nv);
  }

  // color the vasi/papi correctly based on angle in degree
  void set_color( double angle_deg ) {
    unsigned count = mVasiColorArray->size();
    double trans = 0.05;
    double color = 1;
    double ref;
    
    if ( count == 12 ) {
      // PAPI configuration
      
      // papi D
      ref = 3.5;
      if ( angle_deg < ref - trans ) {
        color = 0.0;
      } else if ( angle_deg < ref + trans ) {
        color = 1.0 - (ref + trans - angle_deg) * (1 / (2 * trans) );
      } else {
        color = 1.0;
      }
      for ( unsigned i = 0; i < 3; ++i ) {
        (*mVasiColorArray)[i][1] = color;
        (*mVasiColorArray)[i][2] = color;
      }
      
      // papi C
      ref = 3.167;
      if ( angle_deg < ref - trans ) {
        color = 0.0;
      } else if ( angle_deg < ref + trans ) {
        color = 1.0 - (ref + trans - angle_deg) * (1 / (2 * trans) );
      } else {
        color = 1.0;
      }
      for ( unsigned i = 3; i < 6; ++i ) {
        (*mVasiColorArray)[i][1] = color;
        (*mVasiColorArray)[i][2] = color;
      }
      
      // papi B
      ref = 2.833;
      if ( angle_deg < ref - trans ) {
        color = 0.0;
      } else if ( angle_deg < ref + trans ) {
        color = 1.0 - (ref + trans - angle_deg) * (1 / (2 * trans) );
      } else {
        color = 1.0;
      }
      for ( unsigned i = 6; i < 9; ++i ) {
        (*mVasiColorArray)[i][1] = color;
        (*mVasiColorArray)[i][2] = color;
      }
      
      // papi A
      ref = 2.5;
      if ( angle_deg < ref - trans ) {
        color = 0.0;
      } else if ( angle_deg < ref + trans ) {
        color = 1.0 - (ref + trans - angle_deg) * (1 / (2 * trans) );
      } else {
        color = 1.0;
      }
      for ( unsigned i = 9; i < 12; ++i ) {
        (*mVasiColorArray)[i][1] = color;
        (*mVasiColorArray)[i][2] = color;
      }
    } else if ( count == 36 ) {
      // probably vasi, first 18 are downwind bar (2.5 deg)
      ref = 2.5;
      if ( angle_deg < ref - trans ) {
        color = 0.0;
      } else if ( angle_deg < ref + trans ) {
        color = 1.0 - (ref + trans - angle_deg) * (1 / (2 * trans) );
      } else {
        color = 1.0;
      }
      for ( unsigned i = 0; i < 18; ++i ) {
        (*mVasiColorArray)[i][1] = color;
        (*mVasiColorArray)[i][2] = color;
      }
      
      // last 6 are upwind bar (3.0 deg)
      ref = 3.0;
      if ( angle_deg < ref - trans ) {
        color = 0.0;
      } else if ( angle_deg < ref + trans ) {
        color = 1.0 - (ref + trans - angle_deg) * (1 / (2 * trans) );
      } else {
        color = 1.0;
      }
      for ( unsigned i = 18; i < 36; ++i ) {
        (*mVasiColorArray)[i][1] = color;
        (*mVasiColorArray)[i][2] = color;
      }
    } else {
      // fail safe
      cout << "unknown vasi/papi configuration, count = " << count << endl;
      for ( unsigned i = 0; i < count; ++i ) {
        (*mVasiColorArray)[i][1] = color;
        (*mVasiColorArray)[i][2] = color;
      }
    }
    // Finally mark the color array dirty
    mVasiColorArray->dirty();
  }

private:
  osg::ref_ptr<osg::Vec4Array> mVasiColorArray;
  osg::Vec3 mReferencePoint;
  osg::Vec3 mGlideSlopeUp;
  osg::Vec3 mGlideSlopeDir;
};

#endif // _SG_VASI_HXX
