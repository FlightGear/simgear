/**
 * \file sg_socket_udp.hxx
 * UDP Socket I/O routines.
 */

// Written by Curtis Olson, started November 2001.
//
// Copyright (C) 2001  Curtis L. Olson - http://www.flightgear.org/~curt
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


#ifndef _SG_SOCKET_UDP_HXX
#define _SG_SOCKET_UDP_HXX


#include <simgear/compiler.h>

#include <string>

#include <simgear/math/sg_types.hxx>
#include <simgear/io/iochannel.hxx>
#include <simgear/io/raw_socket.hxx>

/**
 * A UDP socket I/O class based on SGIOChannel and plib/net.
 */
class SGSocketUDP : public SGIOChannel {

private:

    simgear::Socket sock;

    std::string hostname;
    std::string port_str;

    char save_buf[ 2 * SG_IO_MAX_MSG_SIZE ];
    int save_len;

    short unsigned int port;

public:

    /**
     * Create an instance of SGSocketUDP.
     *
     * When calling the constructor you need to provide a host name, and a
     * port number. The convention used by the
     * SGSocketUDP class is that the server side listens and the client
     * side sends. For a server socket, the host name should be
     * empty. For a server, the port number is optional, if you do not
     * specify a port, the system will assign one. For a client
     * socket, you need to specify both a destination host and
     * destination port.
     *
     * UDP sockets are a lower level protocol than TCP sockets and are
     * "connectionless" in the sense that either client or server can
     * exist, or not exist, startup, quit, etc. in any order and
     * whenever both ends are alive, the communication succeeds. With
     * UDP sockets, the server end just sits and listens for incoming
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
     * @param style specify "udp" or "tcp" */
    SGSocketUDP( const std::string& host, const std::string& port );

    /** Destructor */
    ~SGSocketUDP();

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
     * Set blocking true or false
     * @return success/failure
     */
    bool setBlocking( bool value );

    /** @return the remote host name */
    inline std::string get_hostname() const { return hostname; }

    /** @return the port number (in string form) */
    inline std::string get_port_str() const { return port_str; }
};


#endif // _SG_SOCKET_UDP_HXX
