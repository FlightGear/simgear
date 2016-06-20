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

#ifdef _WIN32
#  include <io.h>
#endif

#include <cstring>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#if !defined(_MSC_VER)
# include <unistd.h>
#endif

#include <simgear/misc/stdint.hxx>
#include <simgear/debug/logstream.hxx>

#include "sg_file.hxx"


SGFile::SGFile(const SGPath &file, int repeat_, int extraoflags_ )
    : file_name(file), fp(-1), eof_flag(true), repeat(repeat_), iteration(0),
      extraoflags(extraoflags_)
{
    set_type( sgFileType );
}

SGFile::SGFile( int existingFd ) :
    fp(existingFd),
    eof_flag(false),
    repeat(1),
    iteration(0)
{
    set_type( sgFileType );
}

SGFile::~SGFile() {
}


// open the file based on specified direction
bool SGFile::open( const SGProtocolDir d ) {
    set_dir( d );

    std::string n = file_name.local8BitStr();
    if ( get_dir() == SG_IO_OUT ) {
#ifdef _WIN32
        int mode = _S_IREAD | _S_IWRITE;
#else
        mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
#endif
        fp = ::open( n.c_str(), O_WRONLY | O_CREAT | O_TRUNC | extraoflags, mode );
    } else if ( get_dir() == SG_IO_IN ) {
        fp = ::open( n.c_str(), O_RDONLY | extraoflags );
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

SGBinaryFile::SGBinaryFile( const SGPath& file, int repeat_ ) :
#ifdef _WIN32
    SGFile(file,repeat_, _O_BINARY)
#else
    SGFile(file,repeat_, 0)
#endif
{
}
