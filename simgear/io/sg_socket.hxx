/**
 * \file sg_socket.hxx
 * Socket I/O routines.
 */

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

#include STL_STRING

#include <simgear/math/sg_types.hxx>
#include <simgear/io/iochannel.hxx>

SG_USING_STD(string);

#if defined(_MSC_VER) || defined(__MINGW32__)
#  include <winsock2.h>
#endif

#define SG_MAX_SOCKET_QUEUE 32


/**
 * A socket I/O class based on SGIOChannel.
 */
class SGSocket : public SGIOChannel {
public:
#if defined(_MSC_VER) || defined(__MINGW32__)
    typedef SOCKET SocketType;
#else
    typedef int SocketType;
#   define INVALID_SOCKET (-1)
#endif

private:
    string hostname;
    string port_str;

    char save_buf[ 2 * SG_IO_MAX_MSG_SIZE ];
    int save_len;

    SocketType sock;
    SocketType msgsock;
    short unsigned int port;
    int sock_style;			// SOCK_STREAM or SOCK_DGRAM

    bool first_read;

    // make a server (master listening) socket
    SocketType make_server_socket();

    // make a client socket
    SocketType make_client_socket();

    // wrapper functions
    size_t readsocket( int fd, void *buf, size_t count );
    size_t writesocket( int fd, const void *buf, size_t count );
#if !defined(_MSC_VER) && !defined(__MINGW32__)
    int closesocket(int fd);
#endif

#if defined(_MSC_VER) || defined(__MINGW32__)
    // Ensure winsock has been initialised.
    static bool wsock_init;
    static bool wsastartup();
#endif

public:

    /**
     * Create an instance of SGSocket.
     *
     * When calling the constructor you need to provide a host name, a
     * port number, and a socket style. The convention used by the
     * SGSocket class is that the server side listens and the client
     * side sends. For a server socket, the host name should be
     * empty. For a server, the port number is optional, if you do not
     * specify a port, the system will assign one. For a client
     * socket, you need to specify both a destination host and
     * destination port. For both client and server sockets you must
     * specify the socket type. Type must be either udp or tcp. Here's
     * a quick breakdown of the major differences between UDP and TCP
     * type sockets.
     *
     * TCP sockets are the type where you establish a connection and
     * then can read and write to the socket from both ends. If one
     * end of TCP socket connect quits, the other end will likely
     * segfault if it doesn't take special precautions.  But, the nice
     * thing about TCP connections is that the underlying protocol
     * guarantees that your message will get through. This imposes a
     * certain performance overhead though on the communication
     * because the protocol must resend failed messages. TCP sockets
     * are good for sending periodic command/response type messages
     * where performance isn't a big issues, but reliability is.
     *
     * UDP sockets on the other hand are a lower level protocol and
     * don't have the same sort of connection as TCP sockets. With UDP
     * sockets, the server end just sits and listens for incoming
     * packets from anywhere. The client end sends it's message and
     * forgets about it. It doesn't care if there isn't even a server
     * out there listening and all the packets are getting
     * lost. Although systems/networks usually do a pretty good job
     * (statistically) of getting your UDP packets to their
     * destination, there is no guarantee that any particular packet
     * will make it. But, because of this low level implementation and
     * lack of error checking, UDP packets are much faster and
     * efficient. UDP packets are good for sending positional
     * information to synchronize two applications. In this case, you
     * want the information to arrive as quickly as possible, and if
     * you lose a packet, you'd rather get new updated information
     * rather than have the system waste time resending a packet that
     * is becoming older and older with every retry.
     * @param host name of host if direction is SG_IO_OUT or SG_IO_BI
     * @param port port number if we care to choose one.
     * @param style specify "udp" or "tcp"
     */
    SGSocket( const string& host, const string& port, const string& style );

    /** Destructor */
    ~SGSocket();

    // If specified as a server (in direction for now) open the master
    // listening socket.  If specified as a client (out direction),
    // open a connection to a server.
    bool open( const SGProtocolDir d );

    // read data from socket
    int read( char *buf, int length );

    // read data from socket
    int readline( char *buf, int length );

    // write data to a socket
    int write( const char *buf, const int length );

    // write null terminated string to a socket
    int writestring( const char *str );

    // close file
    bool close();

    /**
     * Enable non-blocking mode.
     * @return success/failure
     */
    bool nonblock();

    /** @return the remote host name */
    inline string get_hostname() const { return hostname; }

    /** @return the port number (in string form) */
    inline string get_port_str() const { return port_str; }
};


#endif // _SG_SOCKET_HXX
