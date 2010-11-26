// lowlevel.cxx -- routines to handle lowlevel compressed binary IO of
//                 various datatypes
//
// Shamelessly adapted from plib (plib.sourceforge.net) January 2001
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


#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include <string.h>		// for memcpy()

#include "lowlevel.hxx" 


static int  read_error = false ;
static int write_error = false ;

void sgClearReadError() { read_error = false; }
void sgClearWriteError() { write_error = false; }
int sgReadError() { return  read_error ; }
int sgWriteError() { return write_error ; }


void sgReadChar ( gzFile fd, char *var )
{
    if ( gzread ( fd, var, sizeof(char) ) != sizeof(char) ) {
        read_error = true ;
    }
}


void sgWriteChar ( gzFile fd, const char var )
{
    if ( gzwrite ( fd, (void *)(&var), sizeof(char) ) != sizeof(char) ) {
        write_error = true ;
    }
}


void sgReadFloat ( gzFile fd, float *var )
{
    union { float v; uint32_t u; } buf;
    if ( gzread ( fd, &buf.u, sizeof(float) ) != sizeof(float) ) {
        read_error = true ;
    }
    if ( sgIsBigEndian() ) {
        sgEndianSwap( &buf.u );
    }
    *var = buf.v;
}


void sgWriteFloat ( gzFile fd, const float var )
{
    union { float v; uint32_t u; } buf;
    buf.v = var;
    if ( sgIsBigEndian() ) {
        sgEndianSwap( &buf.u );
    }
    if ( gzwrite ( fd, (void *)(&buf.u), sizeof(float) ) != sizeof(float) ) {
        write_error = true ;
    }
}


void sgReadDouble ( gzFile fd, double *var )
{
    union { double v; uint64_t u; } buf;
    if ( gzread ( fd, &buf.u, sizeof(double) ) != sizeof(double) ) {
        read_error = true ;
    }
    if ( sgIsBigEndian() ) {
        sgEndianSwap( &buf.u );
    }
    *var = buf.v;
}


void sgWriteDouble ( gzFile fd, const double var )
{
    union { double v; uint64_t u; } buf;
    buf.v = var;
    if ( sgIsBigEndian() ) {
        sgEndianSwap( &buf.u );
    }
    if ( gzwrite ( fd, (void *)(&buf.u), sizeof(double) ) != sizeof(double) ) {
        write_error = true ;
    }
}


void sgReadUInt ( gzFile fd, unsigned int *var )
{
    if ( gzread ( fd, var, sizeof(unsigned int) ) != sizeof(unsigned int) ) {
        read_error = true ;
    }
    if ( sgIsBigEndian() ) {
        sgEndianSwap( (uint32_t *)var);
    }
}


void sgWriteUInt ( gzFile fd, const unsigned int var )
{
    if ( sgIsBigEndian() ) {
        sgEndianSwap( (uint32_t *)&var);
    }
    if ( gzwrite ( fd, (void *)(&var), sizeof(unsigned int) )
	 != sizeof(unsigned int) )
    {
        write_error = true ;
    }
}


void sgReadInt ( gzFile fd, int *var )
{
    if ( gzread ( fd, var, sizeof(int) ) != sizeof(int) ) {
        read_error = true ;
    }
    if ( sgIsBigEndian() ) {
        sgEndianSwap( (uint32_t *)var);
    }
}


void sgWriteInt ( gzFile fd, const int var )
{
    if ( sgIsBigEndian() ) {
        sgEndianSwap( (uint32_t *)&var);
    }
    if ( gzwrite ( fd, (void *)(&var), sizeof(int) ) != sizeof(int) ) {
        write_error = true ;
    }
}


void sgReadLong ( gzFile fd, int32_t *var )
{
    if ( gzread ( fd, var, sizeof(int32_t) ) != sizeof(int32_t) ) {
        read_error = true ;
    }
    if ( sgIsBigEndian() ) {
        sgEndianSwap( (uint32_t *)var);
    }
}


void sgWriteLong ( gzFile fd, const int32_t var )
{
    if ( sgIsBigEndian() ) {
        sgEndianSwap( (uint32_t *)&var);
    }
    if ( gzwrite ( fd, (void *)(&var), sizeof(int32_t) )
         != sizeof(int32_t) )
    {
        write_error = true ;
    }
}


void sgReadLongLong ( gzFile fd, int64_t *var )
{
    if ( gzread ( fd, var, sizeof(int64_t) ) != sizeof(int64_t) ) {
        read_error = true ;
    }
    if ( sgIsBigEndian() ) {
        sgEndianSwap( (uint64_t *)var);
    }
}


void sgWriteLongLong ( gzFile fd, const int64_t var )
{
    if ( sgIsBigEndian() ) {
        sgEndianSwap( (uint64_t *)&var);
    }
    if ( gzwrite ( fd, (void *)(&var), sizeof(int64_t) )
         != sizeof(int64_t) )
    {
        write_error = true ;
    }
}


void sgReadUShort ( gzFile fd, unsigned short *var )
{
    if ( gzread ( fd, var, sizeof(unsigned short) ) != sizeof(unsigned short) ){
        read_error = true ;
    }
    if ( sgIsBigEndian() ) {
        sgEndianSwap( (uint16_t *)var);
    }
}


void sgWriteUShort ( gzFile fd, const unsigned short var )
{
    if ( sgIsBigEndian() ) {
        sgEndianSwap( (uint16_t *)&var);
    }
    if ( gzwrite ( fd, (void *)(&var), sizeof(unsigned short) )
	 != sizeof(unsigned short) )
    {
        write_error = true ;
    }
}


void sgReadShort ( gzFile fd, short *var )
{
    if ( gzread ( fd, var, sizeof(short) ) != sizeof(short) ) {
        read_error = true ;
    }
    if ( sgIsBigEndian() ) {
        sgEndianSwap( (uint16_t *)var);
    }
}


void sgWriteShort ( gzFile fd, const short var )
{
    if ( sgIsBigEndian() ) {
        sgEndianSwap( (uint16_t *)&var);
    }
    if ( gzwrite ( fd, (void *)(&var), sizeof(short) ) != sizeof(short) ) {
        write_error = true ;
    }
}


void sgReadFloat ( gzFile fd, const unsigned int n, float *var )
{
    if ( gzread ( fd, var, sizeof(float) * n ) != (int)(sizeof(float) * n) ) {
        read_error = true ;
    }
    if ( sgIsBigEndian() ) {
        for ( unsigned int i = 0; i < n; ++i ) {
            sgEndianSwap( (uint32_t *)var++);
        }
    }
}


void sgWriteFloat ( gzFile fd, const unsigned int n, const float *var )
{
    if ( sgIsBigEndian() ) {
        float *swab = new float[n];
        float *ptr = swab;
        memcpy( swab, var, sizeof(float) * n );
        for ( unsigned int i = 0; i < n; ++i ) {
            sgEndianSwap( (uint32_t *)ptr++);
        }
        var = swab;
    }
    if ( gzwrite ( fd, (void *)var, sizeof(float) * n )
	 != (int)(sizeof(float) * n) )
    {
        write_error = true ;
    }
}

void sgReadDouble ( gzFile fd, const unsigned int n, double *var )
{
    if ( gzread ( fd, var, sizeof(double) * n ) != (int)(sizeof(double) * n) ) {
        read_error = true ;
    }
    if ( sgIsBigEndian() ) {
        for ( unsigned int i = 0; i < n; ++i ) {
            sgEndianSwap( (uint64_t *)var++);
        }
    }
}


void sgWriteDouble ( gzFile fd, const unsigned int n, const double *var )
{
    if ( sgIsBigEndian() ) {
        double *swab = new double[n];
        double *ptr = swab;
        memcpy( swab, var, sizeof(double) * n );
        for ( unsigned int i = 0; i < n; ++i ) {
            sgEndianSwap( (uint64_t *)ptr++);
        }
        var = swab;
    }
    if ( gzwrite ( fd, (void *)var, sizeof(double) * n )
	 != (int)(sizeof(double) * n) )
    {
        write_error = true ;
    }
}

void sgReadBytes ( gzFile fd, const unsigned int n, void *var ) 
{
    if ( n == 0) return;
    if ( gzread ( fd, var, n ) != (int)n ) {
        read_error = true ;
    }
}

void sgWriteBytes ( gzFile fd, const unsigned int n, const void *var ) 
{
    if ( n == 0) return;
    if ( gzwrite ( fd, (void *)var, n ) != (int)n ) {
        write_error = true ;
    }
}


void sgReadUShort ( gzFile fd, const unsigned int n, unsigned short *var )
{
    if ( gzread ( fd, var, sizeof(unsigned short) * n )
	 != (int)(sizeof(unsigned short) * n) )
    {
        read_error = true ;
    }
    if ( sgIsBigEndian() ) {
        for ( unsigned int i = 0; i < n; ++i ) {
            sgEndianSwap( (uint16_t *)var++);
        }
    }
}


void sgWriteUShort ( gzFile fd, const unsigned int n, const unsigned short *var )
{
    if ( sgIsBigEndian() ) {
        unsigned short *swab = new unsigned short[n];
        unsigned short *ptr = swab;
        memcpy( swab, var, sizeof(unsigned short) * n );
        for ( unsigned int i = 0; i < n; ++i ) {
            sgEndianSwap( (uint16_t *)ptr++);
        }
        var = swab;
    }
    if ( gzwrite ( fd, (void *)var, sizeof(unsigned short) * n )
	 != (int)(sizeof(unsigned short) * n) )
    {
        write_error = true ;
    }
}



void sgReadShort ( gzFile fd, const unsigned int n, short *var )
{
    if ( gzread ( fd, var, sizeof(short) * n )
	 != (int)(sizeof(short) * n) )
    {
        read_error = true ;
    }
    if ( sgIsBigEndian() ) {
        for ( unsigned int i = 0; i < n; ++i ) {
            sgEndianSwap( (uint16_t *)var++);
        }
    }
}


void sgWriteShort ( gzFile fd, const unsigned int n, const short *var )
{
    if ( sgIsBigEndian() ) {
        short *swab = new short[n];
        short *ptr = swab;
        memcpy( swab, var, sizeof(short) * n );
        for ( unsigned int i = 0; i < n; ++i ) {
            sgEndianSwap( (uint16_t *)ptr++);
        }
        var = swab;
    }
    if ( gzwrite ( fd, (void *)var, sizeof(short) * n )
	 != (int)(sizeof(short) * n) )
    {
        write_error = true ;
    }
}


void sgReadUInt ( gzFile fd, const unsigned int n, unsigned int *var )
{
    if ( gzread ( fd, var, sizeof(unsigned int) * n )
	 != (int)(sizeof(unsigned int) * n) )
    {
        read_error = true ;
    }
    if ( sgIsBigEndian() ) {
        for ( unsigned int i = 0; i < n; ++i ) {
            sgEndianSwap( (uint32_t *)var++);
        }
    }
}


void sgWriteUInt ( gzFile fd, const unsigned int n, const unsigned int *var )
{
    if ( sgIsBigEndian() ) {
        unsigned int *swab = new unsigned int[n];
        unsigned int *ptr = swab;
        memcpy( swab, var, sizeof(unsigned int) * n );
        for ( unsigned int i = 0; i < n; ++i ) {
            sgEndianSwap( (uint32_t *)ptr++);
        }
        var = swab;
    }
    if ( gzwrite ( fd, (void *)var, sizeof(unsigned int) * n )
	 != (int)(sizeof(unsigned int) * n) )
    {
        write_error = true ;
    }
}



void sgReadInt ( gzFile fd, const unsigned int n, int *var )
{
    if ( gzread ( fd, var, sizeof(int) * n )
	 != (int)(sizeof(int) * n) )
    {
        read_error = true ;
    }
    if ( sgIsBigEndian() ) {
        for ( unsigned int i = 0; i < n; ++i ) {
            sgEndianSwap( (uint32_t *)var++);
        }
    }
}


void sgWriteInt ( gzFile fd, const unsigned int n, const int *var )
{
    if ( sgIsBigEndian() ) {
        int *swab = new int[n];
        int *ptr = swab;
        memcpy( swab, var, sizeof(int) * n );
        for ( unsigned int i = 0; i < n; ++i ) {
            sgEndianSwap( (uint32_t *)ptr++);
        }
        var = swab;
    }
    if ( gzwrite ( fd, (void *)var, sizeof(int) * n )
	 != (int)(sizeof(int) * n) )
    {
        write_error = true ;
    }
}



#define MAX_ENTITY_NAME_LENGTH 1024

void sgReadString ( gzFile fd, char **var )
{
    int i ;
    char s [ MAX_ENTITY_NAME_LENGTH ] ;

    for ( i = 0 ; i < MAX_ENTITY_NAME_LENGTH ; i++ ) {
	int c = gzgetc ( fd ) ;
	s [ i ] = c ;

	if ( c == '\0' )
	    break ;
    }

    if ( i >= MAX_ENTITY_NAME_LENGTH-1 )
	s [ MAX_ENTITY_NAME_LENGTH-1 ] = '\0' ;


    if ( s[0] == '\0' )
	*var = NULL ;
    else {
	*var = new char [ strlen(s)+1 ] ;
	strcpy ( *var, s ) ;
    }
}


void sgWriteString ( gzFile fd, const char *var )
{
    if ( var != NULL ) {
	if ( gzwrite ( fd, (void *)var, strlen(var) + 1 ) == 
	     (int)(strlen(var) + 1) )
	    return ;
    } else {
	gzputc( fd, 0 );
    }
}


