/**
 * \file sg_dem.hxx
 * Routines to read and write dem files.
 */

// Written by Peter Sadrozinski, started August 2016.
//
// Copyright (C) 2016  Peter Sadrozinski
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// $Id$


#ifndef _SG_DEM_HXX
#define _SG_DEM_HXX

#include <zlib.h> // for gzFile

#include <simgear/compiler.h>
#include <simgear/constants.h>
#include <simgear/math/sg_types.hxx>
#include <simgear/math/SGMath.hxx>

#include <vector>
#include <string>
#include <boost/array.hpp>

// a dem is a signle directory of tiles, and an information file.
// info file structure is:
// tilew, tileh, resx, resy
// tile width and height are in degrees
// resx and resy specify the number of rows and columns in each tile
// SRTM-3 is 1x1, 1201x1201
// SRTM-1 is 1x1, 3601x3601
// etc....

class SGReadDemSession {
public:
    // open session for reading DEM
    SGReadDemSession( const std::string& p, SGDemLevel& l, SGGeod& min, const SGGeod& max );
    ~SGReadDemSession();

    unsigned short getAlt( double lon, double lat );
    
private:    
    getTileName( int lon, int lat );
    
    std::string path;
};

class SGWriteDemSession {
public:
    // open session for writing DEM
    SGWriteDemSession( const std::string& p, SGDemLevel& l, int tilew, int tileh, int resx, int resy );
    ~SGWriteDemSession();
    
private:
    getTileName( int lon, int lat );

    std::string path;
};

class SGDemTile {
};    

#endif // _SG_DEM_HXX
