// sg_socket.hxx -- Socket I/O routines
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


#ifndef _SG_SOCKET_HXX
#define _SG_SOCKET_HXX


#ifndef __cplusplus
# error This library requires C++
#endif

#include <simgear/compiler.h>

#include <string>

#include <simgear/math/fg_types.hxx>

#include "iochannel.hxx"

FG_USING_STD(string);


#define SG_MAX_SOCKET_QUEUE 32


class SGSocket : public SGIOChannel {

    string hostname;
    string port_str;

    char save_buf[ 2 * SG_IO_MAX_MSG_SIZE ];
    int save_len;

    int sock;
    short unsigned int port;
    int sock_style;			// SOCK_STREAM or SOCK_DGRAM

    // make a server (master listening) socket
    int make_server_socket();

    // make a client socket
    int make_client_socket();

    // int_list client_connections;

public:

    SGSocket( const string& host, const string& port, const string& style );
    ~SGSocket();

    // If specified as a server (in direction for now) open the master
    // listening socket.  If specified as a client (out direction),
    // open a connection to a server.
    bool open( SGProtocolDir dir );

    // read data from socket
    int read( char *buf, int length );

    // read data from socket
    int readline( char *buf, int length );

    // write data to a socket
    int write( char *buf, int length );

    // write null terminated string to a socket
    int writestring( char *str );

    // close file
    bool close();

    inline string get_hostname() const { return hostname; }
    inline string get_port_str() const { return port_str; }
};


#endif // _SG_SOCKET_HXX


