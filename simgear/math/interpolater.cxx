//
// interpolater.cxx -- routines to handle linear interpolation from a table of
//                     x,y   The table must be sorted by "x" in ascending order
//
// Written by Curtis Olson, started April 1998.
//
// Copyright (C) 1998  Curtis L. Olson  - http://www.flightgear.org/~curt
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


#include <simgear/compiler.h>

#include STL_STRING

#include <simgear/debug/logstream.hxx>
#include <simgear/misc/sgstream.hxx>

#include "interpolater.hxx"

SG_USING_STD(string);

// Constructor -- starts with an empty table.
SGInterpTable::SGInterpTable()
{
}


// Constructor -- loads the interpolation table from the specified
// file
SGInterpTable::SGInterpTable( const string& file ) 
{
    SG_LOG( SG_MATH, SG_INFO, "Initializing Interpolator for " << file );

    sg_gzifstream in( file );
    if ( !in.is_open() ) {
        SG_LOG( SG_GENERAL, SG_ALERT, "Cannot open file: " << file );
        return;
    }

    in >> skipcomment;
    while ( in ) {
      double ind, dep;
      in >> ind >> dep;
      in >> skipws;
      _table[ind] = dep;
    }
}


// Add an entry to the table.
void SGInterpTable::addEntry (double ind, double dep)
{
  _table[ind] = dep;
}

// Given an x value, linearly interpolate the y value from the table
double SGInterpTable::interpolate(double x) const
{
  // Empty table??
  if (_table.empty())
    return 0;
  
  // Find the table bounds for the requested input.
  Table::const_iterator upBoundIt = _table.upper_bound(x);
  // points to a value outside the map. That is we are out of range.
  // use the last entry
  if (upBoundIt == _table.end())
    return _table.rbegin()->second;
  
  // points to the first key must be lower
  // use the first entry
  if (upBoundIt == _table.begin())
    return upBoundIt->second;
  
  // we know that we do not stand at the beginning, so it is safe to do so
  Table::const_iterator loBoundIt = upBoundIt;
  --loBoundIt;
  
  // Just do linear interpolation.
  double loBound = loBoundIt->first;
  double upBound = upBoundIt->first;
  double loVal = loBoundIt->second;
  double upVal = upBoundIt->second;
  
  // division by zero should not happen since the std::map
  // has sorted out duplicate entries before. Also since we have a
  // map, we know that we get different first values for different iterators
  return loVal + (upVal - loVal)*(x - loBound)/(upBound - loBound);
}


// Destructor
SGInterpTable::~SGInterpTable() {
}


