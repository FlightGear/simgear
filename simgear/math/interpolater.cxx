// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 1998 Curtis L. Olson - http://www.flightgear.org/~curt

/**
 * @file
 * @brief Routines to handle linear interpolation from a table of x,y. The table must be sorted by "x" in ascending order
 */

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include <simgear/compiler.h>

#include <string>

#include <simgear/debug/logstream.hxx>
#include <simgear/io/iostreams/sgstream.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/props/props.hxx>

#include "interpolater.hxx"

// Constructor -- starts with an empty table.
SGInterpTable::SGInterpTable()
{
}

SGInterpTable::SGInterpTable(const SGPropertyNode* interpolation) 
{
    if (!interpolation)
        return;
    std::vector<SGPropertyNode_ptr> entries = interpolation->getChildren("entry");
    for (unsigned i = 0; i < entries.size(); ++i)
        addEntry(entries[i]->getDoubleValue("ind", 0.0),
                 entries[i]->getDoubleValue("dep", 0.0));
}

// Constructor -- loads the interpolation table from the specified
// file
SGInterpTable::SGInterpTable( const std::string& file )
{
    sg_gzifstream in( SGPath::fromUtf8(file) );
    if ( !in.is_open() ) {
        SG_LOG( SG_GENERAL, SG_ALERT, "Cannot open file: " << file );
        return;
    }

    in >> skipcomment;
    while ( in ) {
        double ind, dep;
        in >> ind >> dep;
        in >> std::skipws;
        _table[ind] = dep;
    }
}


// Constructor -- loads the interpolation table from the specified
// file
SGInterpTable::SGInterpTable( const SGPath& file )
{
    sg_gzifstream in( file );
    if ( !in.is_open() ) {
        SG_LOG( SG_GENERAL, SG_ALERT, "Cannot open file: " << file );
        return;
    }

    in >> skipcomment;
    while ( in ) {
      double ind, dep;
      in >> ind >> dep;
      in >> std::skipws;
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


