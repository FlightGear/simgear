// sg_socket.cxx -- Socket I/O routines
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

#if !defined(_MSC_VER) && !defined(__MINGW32__)
#  include <sys/time.h>		// select()
#  include <sys/types.h>	// socket(), bind(), select(), accept()
#  include <sys/socket.h>	// socket(), bind(), listen(), accept()
#  include <netinet/in.h>	// struct sockaddr_in
#  include <netdb.h>		// gethostbyname()
#  include <unistd.h>		// select(), fsync()/fdatasync(), fcntl()
#  include <fcntl.h>		// fcntl()
#endif

#if defined( sgi )
#include <strings.h>
#endif

#include <simgear/debug/logstream.hxx>

#include "sg_socket.hxx"


SGSocket::SGSocket( const string& host, const string& port, 
		    const string& style ) :
    hostname(host),
    port_str(port),
    save_len(0)
{
#if defined(_MSC_VER) || defined(__MINGW32__)
    if (!wsock_init && !wsastartup()) {
    	SG_LOG( SG_IO, SG_ALERT, "Winsock not available");
    }
#endif

    if ( style == "udp" ) {
	sock_style = SOCK_DGRAM;
    } else if ( style == "tcp" ) {
	sock_style = SOCK_STREAM;
    } else {
	sock_style = SOCK_DGRAM;
	SG_LOG( SG_IO, SG_ALERT,
		"Error: SGSocket() unknown style = " << style );
    }

    set_type( sgSocketType );
}


SGSocket::~SGSocket() {
}


SGSocket::SocketType SGSocket::make_server_socket () {
    struct sockaddr_in name;

#if defined( __CYGWIN__ ) || defined( __CYGWIN32__ ) || defined( sgi ) || defined( _MSC_VER ) || defined(__MINGW32__) || defined( __APPLE__ )
    int length;
#else
    socklen_t length;
#endif
     
    // Create the socket.
    sock = socket (PF_INET, sock_style, 0);
    if (sock == INVALID_SOCKET) {
        SG_LOG( SG_IO, SG_ALERT, 
                "Error: socket() failed in make_server_socket()" );
        return INVALID_SOCKET;
    }
     
    // Give the socket a name.
    name.sin_family = AF_INET;
    name.sin_addr.s_addr = INADDR_ANY;
    name.sin_port = htons(port); // set port to zero to let system pick
    name.sin_addr.s_addr = htonl (INADDR_ANY);
    if (bind (sock, (struct sockaddr *) &name, sizeof (name)) != 0) {
	    SG_LOG( SG_IO, SG_ALERT,
		    "Error: bind() failed in make_server_socket()" );
        return INVALID_SOCKET;
    }

    // Find the assigned port number
    length = sizeof(struct sockaddr_in);
    if ( getsockname(sock, (struct sockaddr *) &name, &length) ) {
	SG_LOG( SG_IO, SG_ALERT,
		"Error: getsockname() failed in make_server_socket()" );
        return INVALID_SOCKET;
    }
    port = ntohs(name.sin_port);

    return sock;
}


SGSocket::SocketType SGSocket::make_client_socket () {
    struct sockaddr_in name;
    struct hostent *hp;
     
    SG_LOG( SG_IO, SG_INFO, "Make client socket()" );

    // Create the socket.
    sock = socket (PF_INET, sock_style, 0);
    if (sock == INVALID_SOCKET) {
        SG_LOG( SG_IO, SG_ALERT, 
                "Error: socket() failed in make_server_socket()" );
        return INVALID_SOCKET;
    }
     
    // specify address family
    name.sin_family = AF_INET;

    // get the hosts official name/info
    hp = gethostbyname( hostname.c_str() );
    if (hp == NULL) {
	SG_LOG( SG_IO, SG_ALERT, "Error: hostname lookup failed" );
	return INVALID_SOCKET;
    }

    // Connect this socket to the host and the port specified on the
    // command line
#if defined( __CYGWIN__ ) || defined( __CYGWIN32__ )
    bcopy(hp->h_addr, (char *)(&(name.sin_addr.s_addr)), hp->h_length);
#else
    bcopy(hp->h_addr, &(name.sin_addr.s_addr), hp->h_length);
#endif
    name.sin_port = htons(port);

    if ( connect(sock, (struct sockaddr *) &name, 
		 sizeof(struct sockaddr_in)) != 0 )
    {
	closesocket(sock);
	SG_LOG( SG_IO, SG_ALERT, 
		"Error: connect() failed in make_client_socket()" );
	return INVALID_SOCKET;
    }

    return sock;
}


// Wrapper functions
size_t SGSocket::readsocket( int fd, void *buf, size_t count ) {
#if defined(_MSC_VER) || defined(__MINGW32__)
    return ::recv( fd, (char *)buf, count, 0 );
#else
    return ::read( fd, buf, count );
#endif
}

size_t SGSocket::writesocket( int fd, const void *buf, size_t count ) {
#if defined(_MSC_VER) || defined(__MINGW32__)
    return ::send( fd, (const char*)buf, count, 0 );
#else
    return ::write( fd, buf, count );
#endif
}

#if !defined(_MSC_VER) && !defined(__MINGW32__)
int SGSocket::closesocket( int fd ) {
    return ::close( fd );
}
#endif


// If specified as a server (in direction for now) open the master
// listening socket.  If specified as a client (out direction), open a
// connection to a server.
bool SGSocket::open( const SGProtocolDir d ) {
    set_dir( d );

    if ( port_str == "" || port_str == "any" ) {
	port = 0; 
    } else {
	port = atoi( port_str.c_str() );
    }
    
    // client_connections.clear();

    if ( get_dir() == SG_IO_IN ) {
	// this means server for now

	// Setup socket to listen on.  Set "port" before making this
	// call.  A port of "0" indicates that we want to let the os
	// pick any available port.
	sock = make_server_socket();
	if ( sock == INVALID_SOCKET ) {
	    SG_LOG( SG_IO, SG_ALERT, "socket creation failed" );
	    return false;
	}

	SG_LOG( SG_IO, SG_INFO, "socket is connected to port = " << port );

	if ( sock_style == SOCK_DGRAM ) {
	    // Non-blocking UDP
	    nonblock();
	} else {
	    // Blocking TCP
	    // Specify the maximum length of the connection queue
	    listen( sock, SG_MAX_SOCKET_QUEUE );
	}

    } else if ( get_dir() == SG_IO_OUT ) {
	// this means client for now

	sock = make_client_socket();
	// TODO: check for error.

	if ( sock_style == SOCK_DGRAM ) {
	    // Non-blocking UDP
	    nonblock();
	}
    } else if ( get_dir() == SG_IO_BI && sock_style == SOCK_STREAM ) {
	// this means server for TCP sockets

	// Setup socket to listen on.  Set "port" before making this
	// call.  A port of "0" indicates that we want to let the os
	// pick any available port.
	sock = make_server_socket();
	// TODO: check for error.

	SG_LOG( SG_IO, SG_INFO, "socket is connected to port = " << port );

	// Blocking TCP
	// Specify the maximum length of the connection queue
	listen( sock, SG_MAX_SOCKET_QUEUE );
    } else {
	SG_LOG( SG_IO, SG_ALERT, 
		"Error:  bidirection mode not available for UDP sockets." );
	return false;
    }

    if ( sock < 0 ) {
	SG_LOG( SG_IO, SG_ALERT, "Error opening socket: " << hostname
		<< ":" << port );
	return false;
    }

    // extra SOCK_STREAM stuff
    msgsock = INVALID_SOCKET;
    first_read = false;

    return true;
}


// read data from socket (server)
// read a block of data of specified size
int SGSocket::read( char *buf, int length ) {
    if ( sock == INVALID_SOCKET ) {
	return 0;
    }

    int result = 0;
    // check for potential input
    fd_set ready;
    FD_ZERO(&ready);
    FD_SET(sock, &ready);
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 0;

    // test for any input available on sock (returning immediately, even if
    // nothing)
    select(32, &ready, 0, 0, &tv);

    if ( FD_ISSET(sock, &ready) ) {
	// cout << "data ready" << endl;

	if ( sock_style == SOCK_STREAM ) {
	    if ( msgsock == INVALID_SOCKET ) {
		msgsock = accept(sock, 0, 0);
		closesocket(sock);
		sock = msgsock;
	    } else {
		result = readsocket( sock, buf, length );
	    }
	} else {
	    result = readsocket( sock, buf, length );
	}

	if ( result != length ) {
	    SG_LOG( SG_IO, SG_INFO, 
		    "Warning: read() not enough bytes." );
	}
    }

    return result;
}


// read a line of data, length is max size of input buffer
int SGSocket::readline( char *buf, int length ) {
    if ( sock == INVALID_SOCKET ) {
	return 0;
    }

    // cout << "sock = " << sock << endl;

    // check for potential input
    fd_set ready;
    FD_ZERO(&ready);
    FD_SET(sock, &ready);
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 0;

    // test for any input read on sock (returning immediately, even if
    // nothing)
    int result = select(32, &ready, 0, 0, &tv);
    // cout << "result = " << result << endl;

    if ( FD_ISSET(sock, &ready) ) {
	// cout << "fd change state\n";
	// read a chunk, keep in the save buffer until we have the
	// requested amount read

	if ( sock_style == SOCK_STREAM ) {
	    // cout << "sock_stream\n";
	    if ( msgsock == INVALID_SOCKET ) {
		// cout << "msgsock == invalid\n";
		msgsock = sock;
		sock = accept(msgsock, 0, 0);
	    } else {
		// cout << "ready to read\n";
		char *buf_ptr = save_buf + save_len;
		result = readsocket( sock, buf_ptr, SG_IO_MAX_MSG_SIZE
				     - save_len );
		// cout << "read result = " << result << endl;

		if ( result > 0 ) {
		    first_read = true;
		}

		save_len += result;

		// Try and detect that the remote end died.  This
		// could cause problems so if you see connections
		// dropping for unexplained reasons, LOOK HERE!
		if ( result == 0 && save_len == 0 && first_read == true ) {
		    SG_LOG( SG_IO, SG_ALERT, 
			    "Connection closed by foreign host." );
                    close();
		}
	    }
	} else {
	    char *buf_ptr = save_buf + save_len;
	    result = readsocket( sock, buf_ptr, SG_IO_MAX_MSG_SIZE - save_len );
	    save_len += result;
	}

	// cout << "current read = " << buf_ptr << endl;
	// cout << "current save_buf = " << save_buf << endl;
	// cout << "save_len = " << save_len << endl;
    } else {
	// cout << "no data ready\n";
    }

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
int SGSocket::write( const char *buf, const int length ) {
    if ( sock == INVALID_SOCKET ) {
	return 0;
    }

    bool error_condition = false;

    if ( writesocket(sock, buf, length) < 0 ) {
	SG_LOG( SG_IO, SG_ALERT, "Error writing to socket: " << port );
	error_condition = true;
    }

#if 0 
    // check for any new client connection requests
    fd_set ready;
    FD_ZERO(&ready);
    FD_SET(sock, &ready);
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 0;

    // test for any input on sock (returning immediately, even if
    // nothing)
    select(32, &ready, 0, 0, &tv);

    // any new connections?
    if ( FD_ISSET(sock, &ready) ) {
	int msgsock = accept(sock, 0, 0);
	if ( msgsock < 0 ) {
	    SG_LOG( SG_IO, SG_ALERT, 
		    "Error: accept() failed in write()" );
	    return 0;
	} else {
	    client_connections.push_back( msgsock );
	}
    }

    SG_LOG( SG_IO, SG_INFO, "Client connections = " << 
	    client_connections.size() );
    for ( int i = 0; i < (int)client_connections.size(); ++i ) {
	int msgsock = client_connections[i];

	// read and junk any possible incoming messages.
	// char junk[ SG_IO_MAX_MSG_SIZE ];
	// std::read( msgsock, junk, SG_IO_MAX_MSG_SIZE );

	// write the interesting data to the socket
	if ( writesocket(msgsock, buf, length) == SOCKET_ERROR ) {
	    SG_LOG( SG_IO, SG_ALERT, "Error writing to socket: " << port );
	    error_condition = true;
	} else {
#ifdef _POSIX_SYNCHRONIZED_IO
	    // fdatasync(msgsock);
#else
	    // fsync(msgsock);
#endif
	}
    }
#endif

    if ( error_condition ) {
	return 0;
    }

    return length;
}


// write null terminated string to socket (server)
int SGSocket::writestring( const char *str ) {
    if ( sock == INVALID_SOCKET ) {
	return 0;
    }

    int length = strlen( str );
    return write( str, length );
}


// close the port
bool SGSocket::close() {
    if ( sock == INVALID_SOCKET ) {
	return 0;
    }

    closesocket( sock );
    if ( sock_style == SOCK_STREAM && msgsock != INVALID_SOCKET ) {
	sock = msgsock;
	msgsock = INVALID_SOCKET;
    }
    return true;
}


// configure the socket as non-blocking
bool SGSocket::nonblock() {
    if ( sock == INVALID_SOCKET ) {
	return 0;
    }

#if defined(_MSC_VER) || defined(__MINGW32__)
    u_long arg = 1;
    if (ioctlsocket( sock, FIONBIO, &arg ) != 0) {
        int error_code = WSAGetLastError();
        SG_LOG( SG_IO, SG_ALERT, 
                "Error " << error_code << ": unable to set non-blocking mode"
);
            return false;
    }
#else
    fcntl( sock, F_SETFL, O_NONBLOCK );
#endif
    return true;
}

#if defined(_MSC_VER) || defined(__MINGW32__)

bool SGSocket::wsock_init = false;

bool
SGSocket::wsastartup() {
    WORD wVersionRequested;
    WSADATA wsaData;

    //wVersionRequested = MAKEWORD( 2, 2 );
    wVersionRequested = MAKEWORD( 1, 1 );
    int err = WSAStartup( wVersionRequested, &wsaData );
    if (err != 0)
    {
        SG_LOG( SG_IO, SG_ALERT, "Error: Couldn't load winsock" );
        return false;
    }

#if 0
    if ( LOBYTE( wsaData.wVersion ) != 2 ||
        HIBYTE( wsaData.wVersion ) != 2 ) {
        SG_LOG( SG_IO, SG_ALERT, "Couldn't load a suitable winsock");
        WSACleanup( );
        return false;
    }
#endif
    wsock_init = true;
    return true;
}
#endif
