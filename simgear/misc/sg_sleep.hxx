
// Written by James Turner, started July 2010.
//
// Copyright (C) 2010  Curtis L. Olson - http://www.flightgear.org/~curt
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
// $Id$


#ifndef _SG_SLEEP_HXX
#define _SG_SLEEP_HXX


#include <simgear/compiler.h>


namespace simgear
{

void sleepForSeconds(int seconds);

void sleepForMSec(int msec);

} // of namespace simgear

#endif // _SG_SLEEP_HXX


