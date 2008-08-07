// sg_file.cxx -- File I/O routines
//
// Written by Curtis Olson, started November 1999.
//
// Copyright (C) 1999  Curtis L. Olson - http://www.flightgear.org/~curt
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


#include <simgear/compiler.h>

#include <string>

#if defined(_MSC_VER) || defined(__MINGW32__)
#  include <io.h>
#endif

#include <cstring>

#include <simgear/misc/stdint.hxx>
#include <simgear/debug/logstream.hxx>

#include "sg_file.hxx"

using std::string;


SGFile::SGFile(const string &file, int repeat_)
    : file_name(file), fp(-1), eof_flag(true), repeat(repeat_), iteration(0)
{
    set_type( sgFileType );
}


SGFile::~SGFile() {
}


// open the file based on specified direction
bool SGFile::open( const SGProtocolDir d ) {
    set_dir( d );

    if ( get_dir() == SG_IO_OUT ) {
#if defined(_MSC_VER) || defined(__MINGW32__)
        int mode = 00666;
#else
        mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
#endif
	fp = ::open( file_name.c_str(), O_WRONLY | O_CREAT | O_TRUNC, mode );
    } else if ( get_dir() == SG_IO_IN ) {
	fp = ::open( file_name.c_str(), O_RDONLY );
    } else {
	SG_LOG( SG_IO, SG_ALERT, 
		"Error:  bidirection mode not available for files." );
	return false;
    }

    if ( fp == -1 ) {
	SG_LOG( SG_IO, SG_ALERT, "Error opening file: "	<< file_name );
	return false;
    }

    eof_flag = false;
    return true;
}


// read a block of data of specified size
int SGFile::read( char *buf, int length ) {
    // read a chunk
    ssize_t result = ::read( fp, buf, length );
    if ( length > 0 && result == 0 ) {
        if (repeat < 0 || iteration < repeat - 1) {
            iteration++;
            // loop reading the file, unless it is empty
            off_t fileLen = ::lseek(fp, 0, SEEK_CUR);
            if (fileLen == 0) {
                eof_flag = true;
                return 0;
            } else {
                ::lseek(fp, 0, SEEK_SET);
                return ::read(fp, buf, length);
            }
        } else {
            eof_flag = true;
        }
    }
    return result;
}


// read a line of data, length is max size of input buffer
int SGFile::readline( char *buf, int length ) {
    // save our current position
    int pos = lseek( fp, 0, SEEK_CUR );

    // read a chunk
    ssize_t result = ::read( fp, buf, length );
    if ( length > 0 && result == 0 ) {
        if ((repeat < 0 || iteration < repeat - 1) && pos != 0) {
            iteration++;
            pos = ::lseek(fp, 0, SEEK_SET);
            result = ::read(fp, buf, length);
        } else {
            eof_flag = true;
        }
    }

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
	SG_LOG( SG_IO, SG_ALERT, "Error writing data: " << file_name );
    }

    return result;
}


// write null terminated string to a file
int SGFile::writestring( const char *str ) {
    int length = std::strlen( str );
    return write( str, length );
}


// close the port
bool SGFile::close() {
    if ( ::close( fp ) == -1 ) {
	return false;
    }

    eof_flag = true;
    return true;
}
