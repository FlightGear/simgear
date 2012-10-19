// lowlevel.hxx -- routines to handle lowlevel compressed binary IO of
//                 various datatypes
//
// Shamelessly adapted from plib  January 2001
//
// Original version Copyright (C) 2000  the plib team
// Local changes Copyright (C) 2000  Curtis L. Olson  - http://www.flightgear.org/~curt
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
//


#ifndef _SG_LOWLEVEL_HXX
#define _SG_LOWLEVEL_HXX

#include <stdio.h>
#include <zlib.h>

#include <simgear/compiler.h>
#include <simgear/misc/stdint.hxx>

#include <simgear/math/SGMath.hxx>

// Note that output is written in little endian form (and converted as
// necessary for big endian machines)

void sgReadChar ( gzFile fd, char *var ) ;
void sgWriteChar ( gzFile fd, const char var ) ;
void sgReadFloat ( gzFile fd, float *var ) ;
void sgWriteFloat ( gzFile fd, const float var ) ;
void sgReadDouble ( gzFile fd, double *var ) ;
void sgWriteDouble ( gzFile fd, const double var ) ;
void sgReadUInt ( gzFile fd, unsigned int *var ) ;
void sgWriteUInt ( gzFile fd, const unsigned int var ) ;
void sgReadInt ( gzFile fd, int *var ) ;
void sgWriteInt ( gzFile fd, const int var ) ;
void sgReadLong ( gzFile fd, int32_t *var ) ;
void sgWriteLong ( gzFile fd, const int32_t var ) ;
void sgReadLongLong ( gzFile fd, int64_t *var ) ;
void sgWriteLongLong ( gzFile fd, const int64_t var ) ;
void sgReadUShort ( gzFile fd, unsigned short *var ) ;
void sgWriteUShort ( gzFile fd, const unsigned short var ) ;
void sgReadShort ( gzFile fd, short *var ) ;
void sgWriteShort ( gzFile fd, const short var ) ;

void sgReadFloat ( gzFile fd, const unsigned int n, float *var ) ;
void sgWriteFloat ( gzFile fd, const unsigned int n, const float *var ) ;
void sgReadDouble ( gzFile fd, const unsigned int n, double *var ) ;
void sgWriteDouble ( gzFile fd, const unsigned int n, const double *var ) ;
void sgReadUInt ( gzFile fd, const unsigned int n, unsigned int *var ) ;
void sgWriteUInt ( gzFile fd, const unsigned int n, const unsigned int *var ) ;
void sgReadInt ( gzFile fd, const unsigned int n, int *var ) ;
void sgWriteInt ( gzFile fd, const unsigned int n, const int *var ) ;
void sgReadUShort ( gzFile fd, const unsigned int n, unsigned short *var ) ;
void sgWriteUShort ( gzFile fd, const unsigned int n, const unsigned short *var ) ;
void sgReadShort ( gzFile fd, const unsigned int n, short *var ) ;
void sgWriteShort ( gzFile fd, const unsigned int n, const short *var ) ;
void sgReadBytes ( gzFile fd, const unsigned int n, void *var ) ;
void sgWriteBytes ( gzFile fd, const unsigned int n, const void *var ) ;

void sgReadString ( gzFile fd, char **var ) ;
void sgWriteString ( gzFile fd, const char *var ) ;

inline void sgReadVec2  ( gzFile fd, SGVec2f& var ) {
    sgReadFloat  ( fd, 2, var.data() ) ;
}
inline void sgWriteVec2 ( gzFile fd, const SGVec2f& var ) {
    sgWriteFloat ( fd, 2, var.data() ) ;
}

inline void sgReadVec3  ( gzFile fd, SGVec3f& var ) {
    sgReadFloat  ( fd, 3, var.data() ) ;
}
inline void sgWriteVec3 ( gzFile fd, const SGVec3f& var ) {
    sgWriteFloat ( fd, 3, var.data() ) ;
}

inline void sgReaddVec3  ( gzFile fd, SGVec3d& var ) {
    sgReadDouble  ( fd, 3, var.data() ) ;
}
inline void sgWritedVec3 ( gzFile fd, const SGVec3d& var ) {
    sgWriteDouble ( fd, 3, var.data() ) ;
}

inline void sgReadVec4  ( gzFile fd, SGVec4f& var ) {
    sgReadFloat  ( fd, 4, var.data() ) ;
}
inline void sgWriteVec4 ( gzFile fd, const SGVec4f& var ) {
    sgWriteFloat ( fd, 4, var.data() ) ;
}

inline void sgReadMat4  ( gzFile fd, SGMatrixf& var ) {
    sgReadFloat  ( fd, 16, (float *)var.data() ) ;
}
inline void sgWriteMat4 ( gzFile fd, const SGMatrixf& var ) {
    sgWriteFloat ( fd, 16, (float *)var.data() ) ;
}

inline void sgReadGeod  ( gzFile fd, SGGeod& var ) {
    double data[3];
    sgReadDouble ( fd, 3, data );
    var = SGGeod::fromDegM( data[0], data[1], data[2] );
}
inline void sgWriteGeod ( gzFile fd, const SGGeod& var ) {
    sgWriteDouble( fd, var.getLongitudeDeg() );
    sgWriteDouble( fd, var.getLatitudeDeg() );
    sgWriteDouble( fd, var.getElevationM() );
}

void sgClearReadError();
void sgClearWriteError();
int sgReadError();
int sgWriteError();

#endif // _SG_LOWLEVEL_HXX
