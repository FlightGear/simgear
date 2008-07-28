// sg_serial.cxx -- Serial I/O routines
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

#include <cstdlib>
#include <cstring>

#include <simgear/compiler.h>

#include <string>

#include <simgear/debug/logstream.hxx>
#include <simgear/serial/serial.hxx>

#include "sg_serial.hxx"

using std::string;


SGSerial::SGSerial( const string& device_name, const string& baud_rate ) :
    save_len(0)
{
    set_type( sgSerialType );
    device = device_name;
    baud = baud_rate;
}


SGSerial::~SGSerial() {
}


// open the serial port based on specified direction
bool SGSerial::open( const SGProtocolDir d ) {
    set_dir( d );

    if ( ! port.open_port( device ) ) {
	SG_LOG( SG_IO, SG_ALERT, "Error opening device: " << device );
	return false;
    }

    // cout << "fd = " << port.fd << endl;

    if ( ! port.set_baud( std::atoi( baud.c_str() ) ) ) {
	SG_LOG( SG_IO, SG_ALERT, "Error setting baud: " << baud );
	return false;
    }

    return true;
}


// Read data from port.  If we don't get enough data, save what we did
// get in the save buffer and return 0.  The save buffer will be
// prepended to subsequent reads until we get as much as is requested.

int SGSerial::read( char *buf, int length ) {
    int result;

    // read a chunk, keep in the save buffer until we have the
    // requested amount read

    char *buf_ptr = save_buf + save_len;
    result = port.read_port( buf_ptr, length - save_len );
    
    if ( result + save_len == length ) {
        std::strncpy( buf, save_buf, length );
	save_len = 0;

	return length;
    }
    
    return 0;
}


// read data from port
int SGSerial::readline( char *buf, int length ) {
    int result;

    // read a chunk, keep in the save buffer until we have the
    // requested amount read

    char *buf_ptr = save_buf + save_len;
    result = port.read_port( buf_ptr, SG_IO_MAX_MSG_SIZE - save_len );
    save_len += result;

    // look for the end of line in save_buf
    int i;
    for ( i = 0; i < save_len && save_buf[i] != '\n'; ++i );
    if ( save_buf[i] == '\n' ) {
	result = i + 1;
    } else {
	// no end of line yet
	return 0;
    }

    // we found an end of line

    // copy to external buffer
    std::strncpy( buf, save_buf, result );
    buf[result] = '\0';
    SG_LOG( SG_IO, SG_INFO, "fg_serial line = " << buf );

    // shift save buffer
    for ( i = result; i < save_len; ++i ) {
	save_buf[ i - result ] = save_buf[i];
    }
    save_len -= result;

    return result;
}


// write data to port
int SGSerial::write( const char *buf, const int length ) {
    int result = port.write_port( buf, length );

    if ( result != length ) {
	SG_LOG( SG_IO, SG_WARN, "Error writing data: " << device );
    }

    return result;
}


// write null terminated string to port
int SGSerial::writestring( const char *str ) {
    int length = std::strlen( str );
    return write( str, length );
}


// close the port
bool SGSerial::close() {
    if ( ! port.close_port() ) {
	return false;
    }

    return true;
}
