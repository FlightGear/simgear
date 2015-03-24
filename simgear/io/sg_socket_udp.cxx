// sg_socket.cxx -- Socket I/O routines
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

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include <simgear/compiler.h>

#include <simgear/debug/logstream.hxx>

#include "sg_socket_udp.hxx"

#include <cstring>
#include <cstdlib> // for atoi
#include <algorithm>

using std::string;

SGSocketUDP::SGSocketUDP( const string& host, const string& port ) :
    hostname(host),
    port_str(port),
    save_len(0)
{
    set_valid( false );
}


SGSocketUDP::~SGSocketUDP() {
}


// If specified as a server (in direction for now) open the master
// listening socket.  If specified as a client (out direction), open a
// connection to a server.
bool SGSocketUDP::open( const SGProtocolDir d ) {
    set_dir( d );

    if ( ! sock.open( false ) ) {	// open a UDP socket
	SG_LOG( SG_IO, SG_ALERT, "error opening socket" );
	return false;
    }

    if ( port_str == "" || port_str == "any" ) {
	port = 0; 
    } else {
	port = atoi( port_str.c_str() );
    }
    
    // client_connections.clear();

    if ( get_dir() == SG_IO_IN ) {
	// this means server

	// bind ...
	if ( sock.bind( hostname.c_str(), port ) == -1 ) {
	    SG_LOG( SG_IO, SG_ALERT, "error binding to port" << port_str );
	    return false;
	}
    } else if ( get_dir() == SG_IO_OUT ) {
	// this means client

	// connect ...
	if ( sock.connect( hostname.c_str(), port ) == -1 ) {
	    SG_LOG( SG_IO, SG_ALERT,
		    "error connecting to " << hostname << port_str );
	    return false;
	}
    } else {
	SG_LOG( SG_IO, SG_ALERT, 
		"Error:  bidirection mode not available for UDP sockets." );
	return false;
    }

    set_valid( true );

    return true;
}


// read data from socket (server)
// read a block of data of specified size
int SGSocketUDP::read( char *buf, int length ) {
    if ( ! isvalid() ) {
	return 0;
    }

    if (length <= 0) {
        return 0;
    }
    int result;
    // prevent buffer overflow
    int maxsize = std::min(length - 1, SG_IO_MAX_MSG_SIZE);

    if ( (result = sock.recv(buf, maxsize, 0)) >= 0 ) {
	buf[result] = '\0';
	// printf("msg received = %s\n", buf);
    }

    return result;
}


// read a line of data, length is max size of input buffer
int SGSocketUDP::readline( char *buf, int length ) {
    if ( ! isvalid() ) {
	return 0;
    }

    if (length <= 0) {
        return 0;
    }
    // cout << "sock = " << sock << endl;

    char *buf_ptr = save_buf + save_len;
    // prevent buffer overflow (size of save_buf is 2 * SG_IO_MAX_MSG_SIZE)
    int maxsize = save_len < SG_IO_MAX_MSG_SIZE ?
    SG_IO_MAX_MSG_SIZE : 2 * SG_IO_MAX_MSG_SIZE - save_len;
    int result = sock.recv(buf_ptr, maxsize, 0);
    // printf("msg received = %s\n", buf);
    save_len += result;

    // look for the end of line in save_buf
    int i;
    for ( i = 0; i < save_len && save_buf[i] != '\n'; ++i );
    if ( save_buf[i] == '\n' ) {
	result = i + 1;
    } else {
	// no end of line yet
	// cout << "no eol found" << endl;
	return 0;
    }
    // cout << "line length = " << result << endl;

    // we found an end of line

    // copy to external buffer
    // prevent buffer overflow
    result = std::min(result,length - 1);
    strncpy( buf, save_buf, result );
    buf[result] = '\0';
    // cout << "sg_socket line = " << buf << endl;
    
    // shift save buffer
    for ( i = result; i < save_len; ++i ) {
	save_buf[ i - result ] = save_buf[i];
    }
    save_len -= result;

    return result;
}


// write data to socket (client)
int SGSocketUDP::write( const char *buf, const int length ) {
    if ( ! isvalid() ) {
	return 0;
    }

    if ( sock.send( buf, length, 0 ) < 0 ) {
	SG_LOG( SG_IO, SG_WARN, "Error writing to socket: " << port );
	return 0;
    }

    return length;
}


// write null terminated string to socket (server)
int SGSocketUDP::writestring( const char *str ) {
    if ( !isvalid() ) {
	return 0;
    }

    int length = strlen( str );
    return write( str, length );
}


// close the port
bool SGSocketUDP::close() {
    if ( !isvalid() ) {
	return 0;
    }

    sock.close();

    return true;
}


// configure the socket as non-blocking
bool SGSocketUDP::setBlocking( bool value ) {
    sock.setBlocking( value );

    return true;
}
