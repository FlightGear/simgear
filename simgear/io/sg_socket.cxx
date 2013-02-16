// sg_socket.cxx -- Socket I/O routines
//
// Written by Curtis Olson, started November 1999.
// Modified by Bernie Bright <bbright@bigpond.net.au>, May 2002.
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

#include <iostream>
#include <cstring>
#include <cstdlib> // for atoi

#include <simgear/debug/logstream.hxx>

#include "sg_socket.hxx"

bool SGSocket::init = false;
using std::string;

SGSocket::SGSocket( const string& host, const string& port_, 
		    const string& style ) :
    hostname(host),
    port_str(port_),
    save_len(0),
    client(0),
    is_tcp(false),
    is_server(false),
    first_read(false),
    timeout(0)
{
    if (!init)
    {
      simgear::Socket::initSockets();
      init = true;
    }

    if ( style == "tcp" )
    {
	is_tcp = true;
    }
    else if ( style != "udp" )
    {
	SG_LOG( SG_IO, SG_ALERT,
		"Error: SGSocket() unknown style = " << style );
    }

    set_type( sgSocketType );
}


SGSocket::~SGSocket()
{
    this->close();
}


bool
SGSocket::make_server_socket()
{
    if (!sock.open( is_tcp ))
    {
        SG_LOG( SG_IO, SG_ALERT, 
                "Error: socket() failed in make_server_socket()" );
	return false;
    }

    if (sock.bind( hostname.c_str(), port ) < 0)
    {
	SG_LOG( SG_IO, SG_ALERT,
		"Error: bind() failed in make_server_socket()" );
	sock.close();
        return false;
    }

    return true;
}


bool
SGSocket::make_client_socket()
{
    if (!sock.open( is_tcp ))
    {
        SG_LOG( SG_IO, SG_ALERT, 
                "Error: socket() failed in make_client_socket()" );
	return false;
    }

    if (sock.connect( hostname.c_str(), port ) < 0)
    {
	SG_LOG( SG_IO, SG_ALERT, 
		"Error: connect() failed in make_client_socket()" );
	sock.close();
        return false;
    }

    return true;
}

// If specified as a server (in direction for now) open the master
// listening socket.  If specified as a client (out direction), open a
// connection to a server.
bool
SGSocket::open( SGProtocolDir direction )
{
    set_dir( direction );

    is_server = is_tcp &&
	(direction == SG_IO_IN || direction == SG_IO_BI);

    if ( port_str == "" || port_str == "any" ) {
	port = 0; 
    } else {
	port = atoi( port_str.c_str() );
    }
    
    if (direction == SG_IO_IN)
    {
	// this means server for now

	// Setup socket to listen on.  Set "port" before making this
	// call.  A port of "0" indicates that we want to let the os
	// pick any available port.
	if (!make_server_socket())
	{
	    SG_LOG( SG_IO, SG_ALERT, "SG_IO_IN socket creation failed" );
	    return false;
	}

	if ( !is_tcp )
	{
	    // Non-blocking UDP
	    nonblock();
	}
	else
	{
	    // Blocking TCP
	    // Specify the maximum length of the connection queue
	    sock.listen( SG_MAX_SOCKET_QUEUE );
	}

    }
    else if (direction == SG_IO_OUT)
    {
	// this means client for now

	if (!make_client_socket())
	{
	    SG_LOG( SG_IO, SG_ALERT, "SG_IO_OUT socket creation failed" );
	    return false;
	}

	if ( !is_tcp )
	{
	    // Non-blocking UDP
	    nonblock();
	}
    }
    else if (direction == SG_IO_BI && is_tcp)
    {
	// this means server for TCP sockets

	// Setup socket to listen on.  Set "port" before making this
	// call.  A port of "0" indicates that we want to let the os
	// pick any available port.
	if (!make_server_socket())
	{
	    SG_LOG( SG_IO, SG_ALERT, "SG_IO_BI socket creation failed" );
	    return false;
	}
	// Blocking TCP
	// Specify the maximum length of the connection queue
	sock.listen( SG_MAX_SOCKET_QUEUE );
    }
    else
    {
	SG_LOG( SG_IO, SG_ALERT, 
		"Error:  bidirection mode not available for UDP sockets." );
	return false;
    }

    first_read = false;

    return true;
}


// read data from socket (server)
// read a block of data of specified size
int
SGSocket::read( char *buf, int length )
{
    if (sock.getHandle() == -1 &&
	(client == 0 || client->getHandle() == -1))
    {
	return 0;
    }

    // test for any input available on sock (returning immediately, even if
    // nothing)
    int result = poll();

    if (result > 0)
    {
       if (is_tcp && is_server)
	{
            result = client->recv( buf, length );
        }
        else
        {
            result = sock.recv( buf, length );
        }

	if ( result != length )
	{
	    SG_LOG( SG_IO, SG_INFO, 
		    "Warning: read() not enough bytes." );
	}
    }

    return result;
}


// read a line of data, length is max size of input buffer
int
SGSocket::readline( char *buf, int length )
{
    if (sock.getHandle() == -1 &&
	(client == 0 || client->getHandle() == -1))
    {
	return 0;
    }

    // test for any input read on sock (returning immediately, even if
    // nothing)
    int result = this->poll();

    if (result > 0)
    {
	// read a chunk, keep in the save buffer until we have the
	// requested amount read

       if (is_tcp && is_server)
	{
	    char *buf_ptr = save_buf + save_len;
	    result = client->recv( buf_ptr, SG_IO_MAX_MSG_SIZE - save_len );
		
	    if ( result > 0 )
	    {
		first_read = true;
	    }

	    save_len += result;

	    // Try and detect that the remote end died.  This
	    // could cause problems so if you see connections
	    // dropping for unexplained reasons, LOOK HERE!
	    if (result == 0 && save_len == 0 && first_read == true)
	    {
		SG_LOG( SG_IO, SG_ALERT, 
			"Connection closed by foreign host." );
		delete client;
		client = 0;
	    }
	}
	else
	{
	    char *buf_ptr = save_buf + save_len;
	    result = sock.recv( buf_ptr, SG_IO_MAX_MSG_SIZE - save_len );
	    save_len += result;
	}
    }

    // look for the end of line in save_buf
    int i;
    for ( i = 0; i < save_len && save_buf[i] != '\n'; ++i )
	;
    if (( i < save_len ) && ( save_buf[i] == '\n' )) {
	result = i + 1;
    } else {
	// no end of line yet
	return 0;
    }

    // we found an end of line

    // check buffer size
    int copy_length = result;
    if (copy_length >= length) {
	SG_LOG( SG_IO, SG_ALERT, 
	        "Alert: readline() has line exceeding the buffer size." );
	copy_length = length-1;
    }
    // copy to external buffer
    strncpy( buf, save_buf, copy_length );
    buf[copy_length] = '\0';

    // shift save buffer
    //memmove( save_buf+, save_buf+, ? );
    for ( i = result; i < save_len; ++i ) {
	save_buf[ i - result ] = save_buf[i];
    }
    save_len -= result;

    return result;
}


// write data to socket (client)
int
SGSocket::write( const char *buf, const int length )
{
    simgear::Socket* s = client == 0 ? &sock : client;
    if (s->getHandle() == -1)
    {
	return 0;
    }

    bool error_condition = false;

    if ( s->send( buf, length ) < 0 )
    {
	SG_LOG( SG_IO, SG_WARN, "Error writing to socket: " << port );
	error_condition = true;
    }

    if ( error_condition ) {
	return 0;
    }

    return length;
}


// write null terminated string to socket (server)
int
SGSocket::writestring( const char *str )
{
    int length = strlen( str );
    return this->write( str, length );
}


// close the port
bool
SGSocket::close()
{
    delete client;
    client = 0;

    sock.close();
    return true;
}


// configure the socket as non-blocking
bool
SGSocket::nonblock()
{
    if (sock.getHandle() == -1) {
	return false;
    }

    sock.setBlocking( false );
    return true;
}

int
SGSocket::poll()
{
    simgear::Socket* readers[2];

    readers[0] = client != 0 ? client : &sock;
    readers[1] = 0;

    simgear::Socket* writers[1];
    writers[0] = 0;

    int result = simgear::Socket::select( readers, writers, timeout );

    if (result > 0 && is_server && client == 0)
    {
	// Accept a new client connection
	simgear::IPAddress addr;
	int new_fd = sock.accept( &addr );
	SG_LOG( SG_IO, SG_INFO, "Accepted connection from "
		<< addr.getHost() << ":" << addr.getPort() );
	client = new simgear::Socket();
	client->setHandle( new_fd );
	return 0;
    }

    return result;
}
