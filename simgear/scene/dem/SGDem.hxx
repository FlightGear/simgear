// SGDem.hxx -- read, write DEM heiarchy
//
// Written by Peter Sadrozinski, started August 2016.
//
// Copyright (C) 2001 - 2003  Curtis L. Olson  - http://www.flightgear.org/~curt
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


#ifndef __SG_DEM_HXX__
#define __SG_DEM_HXX__

#include <map>

#include <simgear/compiler.h>
#include <simgear/misc/sg_path.hxx>
#include <simgear/scene/dem/SGDemRoot.hxx>

class SGDem : public SGReferenced
{
public:
    SGDem() {};
    ~SGDem() {};

    int addRoot( const SGPath& root );
    int createRoot( const SGPath& root );
    SGDemRoot* getRoot( unsigned int i ) {
        if ( i < demRoots.size() ) {
            return &demRoots[i];
        } else {
            return NULL;
        }
    }

    unsigned int getNumRoots( void ) {
        return demRoots.size();
    }

    // todo : move to session
    // unsigned short getAlt( const SGDemSession& s, const SGGeod& loc ) const;

    // find a Dem to satisfy a session - must have the level, and extents
    SGDemRoot* findDem( unsigned wo, unsigned so, unsigned eo, unsigned no, int lvl );

    // open a session from a dem level - tiles will be read and reference counted until closed
    //SGDemSession openSession( const SGGeod& min, const SGGeod& max, int level, bool cache );
    SGDemSession openSession( unsigned wo, unsigned so, unsigned eo, unsigned no, int level, bool cache );

    // open a session from an bare directory
    SGDemSession openSession( const SGGeod& min, const SGGeod& max, const SGPath& input );

    // static helpers
    static int      floorWithEpsilon( double x );
    static unsigned normalizeLongitude( unsigned offset );
    static unsigned longitudeDegToOffset( double lon );
    static double   offsetToLongitudeDeg( unsigned offset );
    static unsigned latitudeDegToOffset( double lat );
    static double   offsetToLatitudeDeg( unsigned offset );
    static unsigned roundDown( unsigned offset, unsigned roundTo );
    static unsigned roundUp( unsigned offset, unsigned roundTo );

private:
    std::vector<SGDemRoot>  demRoots;
};

typedef SGSharedPtr<SGDem> SGDemPtr;

#endif /* __SG_DEM_HXX__ */
