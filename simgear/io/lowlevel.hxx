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
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// $Id$
//


#ifndef _SG_LOWLEVEL_HXX
#define _SG_LOWLEVEL_HXX


#include <stdio.h>
#include <zlib.h>

#include <plib/sg.h>

#ifdef _MSC_VER
typedef __int64 int64;
typedef __int64 uint64;
#else
typedef long long int64;
typedef unsigned long long uint64;
#endif

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
void sgReadLong ( gzFile fd, long int *var ) ;
void sgWriteLong ( gzFile fd, const long int var ) ;
void sgReadLongLong ( gzFile fd, int64 *var ) ;
void sgWriteLongLong ( gzFile fd, const int64 var ) ;
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

inline void sgReadVec2  ( gzFile fd, sgVec2 var ) {
    sgReadFloat  ( fd, 2, var ) ;
}
inline void sgWriteVec2 ( gzFile fd, const sgVec2 var ) {
    sgWriteFloat ( fd, 2, var ) ;
}

inline void sgReadVec3  ( gzFile fd, sgVec3 var ) {
    sgReadFloat  ( fd, 3, var ) ;
}
inline void sgWriteVec3 ( gzFile fd, const sgVec3 var ) {
    sgWriteFloat ( fd, 3, var ) ;
}

inline void sgReaddVec3  ( gzFile fd, sgdVec3 var ) {
    sgReadDouble  ( fd, 3, var ) ;
}
inline void sgWritedVec3 ( gzFile fd, const sgdVec3 var ) {
    sgWriteDouble ( fd, 3, var ) ;
}

inline void sgReadVec4  ( gzFile fd, sgVec4 var ) {
    sgReadFloat  ( fd, 4, var ) ;
}
inline void sgWriteVec4 ( gzFile fd, const sgVec4 var ) {
    sgWriteFloat ( fd, 4, var ) ;
}

inline void sgReadMat4  ( gzFile fd, sgMat4 var ) {
    sgReadFloat  ( fd, 16, (float *)var ) ;
}
inline void sgWriteMat4 ( gzFile fd, const sgMat4 var ) {
    sgWriteFloat ( fd, 16, (float *)var ) ;
}

void sgClearReadError();
void sgClearWriteError();
int sgReadError();
int sgWriteError();

inline bool sgIsLittleEndian() {
    static const int sgEndianTest = 1;
    return (*((char *) &sgEndianTest ) != 0);
}

inline bool sgIsBigEndian() {
    static const int sgEndianTest = 1;
    return (*((char *) &sgEndianTest ) == 0);
}

inline void sgEndianSwap(unsigned short *x) {
    *x =
        (( *x >>  8 ) & 0x00FF ) | 
        (( *x <<  8 ) & 0xFF00 ) ;
}
  
inline void sgEndianSwap(unsigned int *x) {
    *x =
        (( *x >> 24 ) & 0x000000FF ) | 
        (( *x >>  8 ) & 0x0000FF00 ) | 
        (( *x <<  8 ) & 0x00FF0000 ) | 
        (( *x << 24 ) & 0xFF000000 ) ;
}
  
inline void sgEndianSwap(uint64 *x) {
#ifndef _MSC_VER
    *x =
        (( *x >> 56 ) & 0x00000000000000FFULL ) | 
        (( *x >> 40 ) & 0x000000000000FF00ULL ) | 
        (( *x >> 24 ) & 0x0000000000FF0000ULL ) | 
        (( *x >>  8 ) & 0x00000000FF000000ULL ) | 
        (( *x <<  8 ) & 0x000000FF00000000ULL ) | 
        (( *x << 24 ) & 0x0000FF0000000000ULL ) |
        (( *x << 40 ) & 0x00FF000000000000ULL ) |
        (( *x << 56 ) & 0xFF00000000000000ULL ) ;
#else
    *x =
        (( *x >> 56 ) & 0x00000000000000FF ) | 
        (( *x >> 40 ) & 0x000000000000FF00 ) | 
        (( *x >> 24 ) & 0x0000000000FF0000 ) | 
        (( *x >>  8 ) & 0x00000000FF000000 ) | 
        (( *x <<  8 ) & 0x000000FF00000000 ) | 
        (( *x << 24 ) & 0x0000FF0000000000 ) |
        (( *x << 40 ) & 0x00FF000000000000 ) |
        (( *x << 56 ) & 0xFF00000000000000 ) ;
#endif
}

#endif // _SG_LOWLEVEL_HXX
