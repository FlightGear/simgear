// localconsts.hxx -- various constant that are shared 
//
// Written by Curtis Olson, started September 2000.
//
// Copyright (C) 2000  Curtis L. Olson  - http://www.flightgear.org/~curt
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
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the
// Free Software Foundation, Inc., 59 Temple Place - Suite 330,
// Boston, MA  02111-1307, USA.
//
// $Id$


#ifndef _SG_LOCAL_CONSTS_HXX
#define _SG_LOCAL_CONSTS_HXX


// Value of earth flattening parameter from ref [8] 
//
//      Note: FP = f
//            E  = 1-f
//            EPS = sqrt(1-(1-f)^2)
//

static const double FP =   0.003352813178;
static const double E  =   0.996647186;
static const double EPS =  0.081819221;
static const double INVG = 0.031080997;


#endif // _SG_LOCAL_CONSTS_HXX
