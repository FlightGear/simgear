// placementtrans.hxx -- class for carrying transforms for placing models in the world
//
// Written by Mathias Froehlich, started April 2005.
//
// Copyright (C) 2005 Mathias Froehlich
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
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//


#ifndef _SG_PLACEMENTTRANS_HXX
#define _SG_PLACEMENTTRANS_HXX

#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   

#include <simgear/compiler.h>
#include <simgear/constants.h>

#include <plib/sg.h>
#include <plib/ssg.h>

class ssgPlacementTransform : public ssgTransform
{
public:
  
  ssgPlacementTransform(void);
  virtual ~ssgPlacementTransform(void);

//   using ssgTransform::addKid(ssgEntity*);

  virtual ssgBase *clone(int clone_flags);
protected:
  void copy_from(ssgPlacementTransform *src, int clone_flags);

private:
//   virtual void setTransform(sgVec3 xyz);
//   virtual void setTransform(sgCoord *xform);
//   virtual void setTransform(sgCoord *xform, float sx, float sy, float sz);
//   virtual void setTransform(sgMat4 xform);
public:

  void setTransform(sgdVec3 off);
  void setTransform(sgdVec3 off, sgMat4 rot);
  void setSceneryCenter(sgdVec3 xyz);

private:

  //////////////////////////////////////////////////////////////////
  // private data                                                 //
  //////////////////////////////////////////////////////////////////
  
  sgdVec3 _placement_offset;
  sgdVec3 _scenery_center;
    
};

#endif // _SG_LOCATION_HXX
