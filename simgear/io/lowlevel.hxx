// lowlevel.hxx -- routines to handle lowlevel compressed binary IO of
//                 various datatypes
//
// Shamelessly adapted from plib  January 2001
//
// Original version Copyright (C) 2000  the plib team
// Local changes Copyright (C) 2000  Curtis L. Olson  - curt@flightgear.org
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

void sgReadVec2 ( gzFile fd, sgVec2 var ) ;
void sgWriteVec2 ( gzFile fd, const sgVec2 var ) ;
void sgReadVec3 ( gzFile fd, sgVec3 var ) ;
void sgWriteVec3 ( gzFile fd, const sgVec3 var ) ;
void sgReaddVec3 ( gzFile fd, sgdVec3 var ) ;
void sgWritedVec3 ( gzFile fd, const sgdVec3 var ) ;
void sgReadVec4 ( gzFile fd, sgVec4 var ) ;
void sgWriteVec4 ( gzFile fd, const sgVec4 var ) ;

void sgReadMat4 ( gzFile fd, sgMat4 var ) ;
void sgWriteMat4 ( gzFile fd, const sgMat4 var ) ;

void sgClearReadError();
void sgClearWriteError();
int sgReadError();
int sgWriteError();


#endif // _SG_LOWLEVEL_HXX
