// sg_file.cxx -- File I/O routines
//
// Written by Curtis Olson, started November 1999.
//
// Copyright (C) 1999  Curtis L. Olson - curt@flightgear.org
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
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// $Id$


#include <simgear/compiler.h>

#include STL_STRING

#ifdef _MSC_VER
#  include <io.h>
#endif

#include <simgear/debug/logstream.hxx>

#include "sg_file.hxx"

FG_USING_STD(string);


SGFile::SGFile( const string &file) {
    set_type( sgFileType );
    file_name = file;
}


SGFile::~SGFile() {
}


// open the file based on specified direction
bool SGFile::open( const SGProtocolDir d ) {
    set_dir( d );

    if ( get_dir() == SG_IO_OUT ) {
#ifdef _MSC_VER
        int mode = 00666;
#else
        mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
#endif
	fp = ::open( file_name.c_str(), O_WRONLY | O_CREAT | O_TRUNC, mode );
    } else if ( get_dir() == SG_IO_IN ) {
	fp = ::open( file_name.c_str(), O_RDONLY );
    } else {
	FG_LOG( FG_IO, FG_ALERT, 
		"Error:  bidirection mode not available for files." );
	return false;
    }

    if ( fp == -1 ) {
	FG_LOG( FG_IO, FG_ALERT, "Error opening file: "	<< file_name );
	return false;
    }

    return true;
}


// read a block of data of specified size
int SGFile::read( char *buf, int length ) {
    // read a chunk
    return ::read( fp, buf, length );
}


// read a line of data, length is max size of input buffer
int SGFile::readline( char *buf, int length ) {
    // save our current position
    int pos = lseek( fp, 0, SEEK_CUR );

    // read a chunk
    int result = ::read( fp, buf, length );

    // find the end of line and reset position
    int i;
    for ( i = 0; i < result && buf[i] != '\n'; ++i );
    if ( buf[i] == '\n' ) {
	result = i + 1;
    } else {
	result = i;
    }
    lseek( fp, pos + result, SEEK_SET );
    
    // just in case ...
    buf[ result ] = '\0';

    return result;
}


// write data to a file
int SGFile::write( const char *buf, const int length ) {
    int result = ::write( fp, buf, length );
    if ( result != length ) {
	FG_LOG( FG_IO, FG_ALERT, "Error writing data: " << file_name );
    }

    return result;
}


// write null terminated string to a file
int SGFile::writestring( const char *str ) {
    int length = strlen( str );
    return write( str, length );
}


// close the port
bool SGFile::close() {
    if ( ::close( fp ) == -1 ) {
	return false;
    }

    return true;
}
