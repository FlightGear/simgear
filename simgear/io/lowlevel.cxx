// lowlevel.hxx -- routines to handle lowlevel compressed binary IO of
//                 various datatypes
//
// Shamelessly adapted from plib (plib.sourceforge.net) January 2001
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

#include "lowlevel.hxx" 

static int  read_error = false ;
static int write_error = false ;

void sgClearReadError() { read_error = false; }
void sgClearWriteError() { write_error = false; }
int sgReadError() { return  read_error ; }
int sgWriteError() { return write_error ; }


void sgReadChar ( gzFile fd, char *var )
{
    if ( gzread ( fd, var, sizeof(char) ) == sizeof(char) ) return ;
    read_error = true ;
}


void sgWriteChar ( gzFile fd, const char var )
{
    if ( gzwrite ( fd, (void *)(&var), sizeof(char) ) == sizeof(char) ) return ;
    write_error = true ;
}


void sgReadFloat ( gzFile fd, float *var )
{
    if ( gzread ( fd, var, sizeof(float) ) == sizeof(float) ) return ;
    read_error = true ;
}


void sgWriteFloat ( gzFile fd, const float var )
{
    if ( gzwrite ( fd, (void *)(&var), sizeof(float) ) == sizeof(float) )
	return ;
    write_error = true ;
}


void sgReadDouble ( gzFile fd, double *var )
{
    if ( gzread ( fd, var, sizeof(double) ) == sizeof(double) ) return ;
    read_error = true ;
}


void sgWriteDouble ( gzFile fd, const double var )
{
    if ( gzwrite ( fd, (void *)(&var), sizeof(double) ) == sizeof(double) )
	return ;
    write_error = true ;
}


void sgReadUInt ( gzFile fd, unsigned int *var )
{
    if ( gzread ( fd, var, sizeof(unsigned int) ) == sizeof(unsigned int) )
	return ;
    read_error = true ;
}


void sgWriteUInt ( gzFile fd, const unsigned int var )
{
    if ( gzwrite ( fd, (void *)(&var), sizeof(unsigned int) )
	 == sizeof(unsigned int) )
	return ;
    write_error = true ;
}


void sgReadInt ( gzFile fd, int *var )
{
    if ( gzread ( fd, var, sizeof(int) ) == sizeof(int) ) return ;
    read_error = true ;
}


void sgWriteInt ( gzFile fd, const int var )
{
    if ( gzwrite ( fd, (void *)(&var), sizeof(int) ) == sizeof(int) ) return ;
    write_error = true ;
}


void sgReadLong ( gzFile fd, long int *var )
{
    if ( gzread ( fd, var, sizeof(long int) ) == sizeof(long int) ) return ;
    read_error = true ;
}


void sgWriteLong ( gzFile fd, const long int var )
{
    if ( gzwrite ( fd, (void *)(&var), sizeof(long int) ) == sizeof(long int) )
	return ;
    write_error = true ;
}


void sgReadUShort ( gzFile fd, unsigned short *var )
{
    if ( gzread ( fd, var, sizeof(unsigned short) ) == sizeof(unsigned short) )
	return ;
    read_error = true ;
}


void sgWriteUShort ( gzFile fd, const unsigned short var )
{
    if ( gzwrite ( fd, (void *)(&var), sizeof(unsigned short) )
	 == sizeof(unsigned short) )
	return ;
    write_error = true ;
}


void sgReadShort ( gzFile fd, short *var )
{
    if ( gzread ( fd, var, sizeof(short) ) == sizeof(short) ) return ;
    read_error = true ;
}


void sgWriteShort ( gzFile fd, const short var )
{
    if ( gzwrite ( fd, (void *)(&var), sizeof(short) ) == sizeof(short) ) return ;
    write_error = true ;
}


void sgReadFloat ( gzFile fd, const unsigned int n, float *var )
{
    if ( gzread ( fd, var, sizeof(float) * n ) == (int)(sizeof(float) * n) )
	return ;
    read_error = true ;
}


void sgWriteFloat ( gzFile fd, const unsigned int n, const float *var )
{
    if ( gzwrite ( fd, (void *)var, sizeof(float) * n )
	 == (int)(sizeof(float) * n) )
	return ;
    write_error = true ;
}

void sgReadDouble ( gzFile fd, const unsigned int n, double *var )
{
    if ( gzread ( fd, var, sizeof(double) * n ) == (int)(sizeof(double) * n) )
	return ;
    read_error = true ;
}


void sgWriteDouble ( gzFile fd, const unsigned int n, const double *var )
{
    if ( gzwrite ( fd, (void *)var, sizeof(double) * n )
	 == (int)(sizeof(double) * n) ) return ;
    write_error = true ;
}

void sgReadBytes ( gzFile fd, const unsigned int n, void *var ) 
{
    if ( n == 0)
	return;
    if ( gzread ( fd, var, n ) == (int)n ) 
	return ;
    read_error = true ;
}

void sgWriteBytes ( gzFile fd, const unsigned int n, const void *var ) 
{
    if ( n == 0)
	return;
    if ( gzwrite ( fd, (void *)var, n ) == (int)n ) 
	return ;
    write_error = true ;
}


void sgReadUShort ( gzFile fd, const unsigned int n, unsigned short *var )
{
    if ( gzread ( fd, var, sizeof(unsigned short) * n )
	 == (int)(sizeof(unsigned short) * n) )
	return ;
    read_error = true ;
}


void sgWriteUShort ( gzFile fd, const unsigned int n, const unsigned short *var )
{
    if ( gzwrite ( fd, (void *)var, sizeof(unsigned short) * n )
	 == (int)(sizeof(unsigned short) * n) )
	return ;
    write_error = true ;
}



void sgReadShort ( gzFile fd, const unsigned int n, short *var )
{
    if ( gzread ( fd, var, sizeof(short) * n )
	 == (int)(sizeof(short) * n) )
	return ;
    read_error = true ;
}


void sgWriteShort ( gzFile fd, const unsigned int n, const short *var )
{
    if ( gzwrite ( fd, (void *)var, sizeof(short) * n )
	 == (int)(sizeof(short) * n) )
	return ;
    write_error = true ;
}


void sgReadUInt ( gzFile fd, const unsigned int n, unsigned int *var )
{
    if ( gzread ( fd, var, sizeof(unsigned int) * n )
	 == (int)(sizeof(unsigned int) * n) )
	return ;
    read_error = true ;
}


void sgWriteUInt ( gzFile fd, const unsigned int n, const unsigned int *var )
{
    if ( gzwrite ( fd, (void *)var, sizeof(unsigned int) * n )
	 == (int)(sizeof(unsigned int) * n) )
	return ;
    write_error = true ;
}



void sgReadInt ( gzFile fd, const unsigned int n, int *var )
{
    if ( gzread ( fd, var, sizeof(int) * n )
	 == (int)(sizeof(int) * n) )
	return ;
    read_error = true ;
}


void sgWriteInt ( gzFile fd, const unsigned int n, const int *var )
{
    if ( gzwrite ( fd, (void *)var, sizeof(int) * n )
	 == (int)(sizeof(int) * n) )
	return ;
    write_error = true ;
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


void sgReadVec2  ( gzFile fd, sgVec2 var ) { sgReadFloat  ( fd, 2, var ) ; }
void sgWriteVec2 ( gzFile fd, const sgVec2 var ) {
    sgWriteFloat ( fd, 2, var ) ;
}

void sgReadVec3  ( gzFile fd, sgVec3 var ) { sgReadFloat  ( fd, 3, var ) ; }
void sgWriteVec3 ( gzFile fd, const sgVec3 var ) {
    sgWriteFloat ( fd, 3, var ) ;
}

void sgReaddVec3  ( gzFile fd, sgdVec3 var ) { sgReadDouble  ( fd, 3, var ) ; }
void sgWritedVec3 ( gzFile fd, const sgdVec3 var ) {
    sgWriteDouble ( fd, 3, var ) ;
}

void sgReadVec4  ( gzFile fd, sgVec4 var ) { sgReadFloat  ( fd, 4, var ) ; }
void sgWriteVec4 ( gzFile fd, const sgVec4 var ) {
    sgWriteFloat ( fd, 4, var ) ;
}

void sgReadMat4  ( gzFile fd, sgMat4 var ) {
    sgReadFloat  ( fd, 16, (float *)var ) ;
}
void sgWriteMat4 ( gzFile fd, const sgMat4 var ) {
    sgWriteFloat ( fd, 16, (float *)var ) ;
}
