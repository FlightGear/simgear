/*
     simgear::Socket, adapted from PLIB Socket by James Turner
     Copyright (C) 2010  James Turner
     
     PLIB - A Suite of Portable Game Libraries
     Copyright (C) 1998,2002  Steve Baker
 
     This library is free software; you can redistribute it and/or
     modify it under the terms of the GNU Library General Public
     License as published by the Free Software Foundation; either
     version 2 of the License, or (at your option) any later version.
 
     This library is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
     Library General Public License for more details.
 
     You should have received a copy of the GNU Library General Public
     License along with this library; if not, write to the Free Software
     Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
*/

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include <simgear/compiler.h>

#include "sg_socket.hxx"

#if defined(_WIN32) && !defined(__CYGWIN__)
# define WINSOCK // use winsock convetions, otherwise standard POSIX
#endif

#include <cstdlib>
#include <ctime>
#include <cstring>
#include <cassert>
#include <cstdio> // for snprintf
#include <iostream>

#if defined(WINSOCK)
#  include <winsock.h>
#  include <stdarg.h>
#else
#  include <sys/types.h>
#  include <sys/socket.h>
#  include <netinet/in.h>
#  include <arpa/inet.h>
#  include <sys/time.h>  
#  include <unistd.h>
#  include <netdb.h>
#  include <fcntl.h>
#endif

#if defined(_MSC_VER) && !defined(socklen_t)
#define socklen_t int
#endif

#include <simgear/debug/logstream.hxx>
#include <simgear/structure/exception.hxx>

namespace simgear
{
                                                                                       
IPAddress::IPAddress ( const char* host, int port )
{
  set ( host, port ) ;
}

IPAddress::IPAddress ( struct sockaddr* addr, size_t len )
{
    memset(&_raw, 0, sizeof(struct sockaddr_in));
    if ((addr->sa_family != AF_INET) ||
        (len != sizeof(struct sockaddr_in))) 
    {
        std::cout << "address size mismatch" << std::endl;
        return;
    }
    
    memcpy(&_raw, addr, len);
}

void IPAddress::setPort(unsigned int port)
{
    _raw.sin_port = htons(port);
}

void IPAddress::set ( const char* host, int port )
{
  memset(&_raw, 0, sizeof(struct sockaddr_in));

  _raw.sin_family = AF_INET ;
  _raw.sin_port = htons (port);

  /* Convert a string specifying a host name or one of a few symbolic
  ** names to a numeric IP address.  This usually calls gethostbyname()
  ** to do the work; the names "" and "<broadcast>" are special.
  */

  if (!host || host[0] == '\0') {
    _raw.sin_addr.s_addr = INADDR_ANY;
    return;
  }
  
  if (strcmp(host, "<broadcast>") == 0) {
    _raw.sin_addr.s_addr = INADDR_BROADCAST;
    return;
  }
  
  _raw.sin_addr.s_addr = inet_addr ( host ) ;
  if (_raw.sin_addr.s_addr != INADDR_NONE) {
    return;
  }
// finally, try gethostbyname
    struct hostent *hp = gethostbyname ( host ) ;
    if (!hp) {
      SG_LOG(SG_IO, SG_WARN, "gethostbyname failed for " << host);
      _raw.sin_addr.s_addr = INADDR_ANY ;
      return;
    }
    
    memcpy ( (char *) &_raw.sin_addr, hp->h_addr, hp->h_length ) ;
}


/* Create a string object representing an IP address.
   This is always a string of the form 'dd.dd.dd.dd' (with variable
   size numbers). */

const char* IPAddress::getHost () const
{
#if 0
  const char* buf = inet_ntoa ( sin_addr ) ;
#else
  static char buf [32];
	long x = ntohl(_raw.sin_addr.s_addr);
	sprintf(buf, "%d.%d.%d.%d",
		(int) (x>>24) & 0xff, (int) (x>>16) & 0xff,
		(int) (x>> 8) & 0xff, (int) (x>> 0) & 0xff );
#endif
  return buf;
}

unsigned int IPAddress::getIP () const 
{ 
	return _raw.sin_addr.s_addr; 
}

unsigned int IPAddress::getPort() const
{
  return ntohs(_raw.sin_port);
}

unsigned int IPAddress::getFamily () const 
{ 
	return _raw.sin_family; 
}

const char* IPAddress::getLocalHost ()
{
  //gethostbyname(gethostname())

  char buf[256];
  memset(buf, 0, sizeof(buf));
  gethostname(buf, sizeof(buf)-1);
  const hostent *hp = gethostbyname(buf);

  if (hp && *hp->h_addr_list)
  {
    in_addr     addr = *((in_addr*)*hp->h_addr_list);
    const char* host = inet_ntoa(addr);

    if ( host )
      return host ;
  }

  return "127.0.0.1" ;
}


bool IPAddress::getBroadcast () const
{
  return (_raw.sin_addr.s_addr == INADDR_BROADCAST);
}


Socket::Socket ()
{
  handle = -1 ;
}


Socket::~Socket ()
{
  close () ;
}


void Socket::setHandle (int _handle)
{
  close () ;
  handle = _handle ;
}


bool Socket::open ( bool stream )
{
  close () ;
  handle = ::socket ( AF_INET, (stream? SOCK_STREAM: SOCK_DGRAM), 0 ) ;

  // Jan 26, 2010: Patch to avoid the problem of the socket resource not
  // yet being available if the program is restarted quickly after being
  // killed.
  //
  // Reference: http://www.unixguide.net/network/socketfaq/4.5.shtml
  // --
  // Also required for joining multicast domains
  if ( stream ) {
    int opt_boolean = 1;
#if defined(_WIN32) || defined(__CYGWIN__)
    setsockopt( handle, SOL_SOCKET, SO_REUSEADDR, (char *)&opt_boolean,
		sizeof(opt_boolean) );
#else
    setsockopt( handle, SOL_SOCKET, SO_REUSEADDR, &opt_boolean,
		sizeof(opt_boolean) );
#endif
  }

  return (handle != -1);
}


void Socket::setBlocking ( bool blocking )
{
  assert ( handle != -1 ) ;

#if defined(WINSOCK)
    u_long nblocking = blocking? 0: 1;
  ::ioctlsocket(handle, FIONBIO, &nblocking);
#else

  int delay_flag = ::fcntl (handle, F_GETFL, 0);

  if (blocking)
    delay_flag &= (~O_NDELAY);
  else
    delay_flag |= O_NDELAY;

  ::fcntl (handle, F_SETFL, delay_flag);
#endif
}


void Socket::setBroadcast ( bool broadcast )
{
  assert ( handle != -1 ) ;
  int result;
  if ( broadcast ) {
      int one = 1;
#if defined(_WIN32) || defined(__CYGWIN__)
      result = ::setsockopt( handle, SOL_SOCKET, SO_BROADCAST, (char*)&one, sizeof(one) );
#else
      result = ::setsockopt( handle, SOL_SOCKET, SO_BROADCAST, &one, sizeof(one) );
#endif
  } else {
      result = ::setsockopt( handle, SOL_SOCKET, SO_BROADCAST, NULL, 0 );
  }
  
  if ( result < 0 ) {
      throw sg_exception("Socket::setBroadcast failed");
  }
}


int Socket::bind ( const char* host, int port )
{
  int result;
  assert ( handle != -1 ) ;
  IPAddress addr ( host, port ) ;

#if !defined(WINSOCK)
  if( (result = ::bind(handle,(const sockaddr*)&addr,sizeof(IPAddress))) < 0 ) {
    SG_LOG(SG_IO, SG_ALERT, "bind(" << host << ":" << port << ") failed. Errno " << errno << " (" << strerror(errno) << ")");
    return result;
  }
#endif

  // 224.0.0.0 - 239.255.255.255 are multicast   
  // Usage of 239.x.x.x is recommended for local scope
  // Reference: http://tools.ietf.org/html/rfc5771
  if( ntohl(addr.getIP()) >= 0xe0000000 && ntohl(addr.getIP()) <= 0xefffffff ) {

#if defined(WINSOCK)
    struct sockaddr_in a;
    a.sin_addr.S_un.S_addr = INADDR_ANY;
    a.sin_family = AF_INET;
    a.sin_port = htons(port);
      
    if( (result = ::bind(handle,(const sockaddr*)&a,sizeof(a))) < 0 ) {
      SG_LOG(SG_IO, SG_ALERT, "bind(any:" << port << ") failed. Errno " << errno << " (" << strerror(errno) << ")");
      return result;
    }
#endif

    struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = addr.getIP();
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    if( (result=::setsockopt(getHandle(), IPPROTO_IP, IP_ADD_MEMBERSHIP, (const char *)&mreq, sizeof(mreq))) != 0 ) {
      SG_LOG(SG_IO, SG_ALERT, "setsockopt(IP_ADD_MEMBERSHIP) failed. Errno " << errno << " (" << strerror(errno) << ")");
      return result;
    }
  }
#if defined(WINSOCK)
  else if( (result = ::bind(handle,(const sockaddr*)&addr,sizeof(IPAddress))) < 0 ) {
    SG_LOG(SG_IO, SG_ALERT, "bind(" << host << ":" << port << ") failed. Errno " << errno << " (" << strerror(errno) << ")");
    return result;
  }
#endif

  return 0;
}


int Socket::listen ( int backlog )
{
  assert ( handle != -1 ) ;
  return ::listen(handle,backlog);
}


int Socket::accept ( IPAddress* addr )
{
  assert ( handle != -1 ) ;

  if ( addr == NULL )
  {
    return ::accept(handle,NULL,NULL);
  }
  else
  {
    socklen_t addr_len = addr->getAddressSize();
    return ::accept(handle,(sockaddr*)addr->getAddress(),&addr_len);
  }
}


int Socket::connect ( const char* host, int port )
{
  assert ( handle != -1 ) ;
  IPAddress addr ( host, port ) ;
  if ( addr.getBroadcast() ) {
      setBroadcast( true );
  }
  return ::connect(handle,(const sockaddr*) addr.getAddress(), addr.getAddressSize());
}

int Socket::connect ( const IPAddress& addr )
{
  assert ( handle != -1 ) ;
  if ( addr.getBroadcast() ) {
      setBroadcast( true );
  }
  
  return ::connect(handle,(const sockaddr*) addr.getAddress(), addr.getAddressSize());
}

int Socket::send (const void * buffer, int size, int flags)
{
  assert ( handle != -1 ) ;
  return ::send (handle, (const char*)buffer, size, flags);
}


int Socket::sendto ( const void * buffer, int size,
                        int flags, const IPAddress* to )
{
  assert ( handle != -1 ) ;
  return ::sendto(handle,(const char*)buffer,size,flags,
                         (const sockaddr*)to,sizeof(IPAddress));
}


int Socket::recv (void * buffer, int size, int flags)
{
  assert ( handle != -1 ) ;
  return ::recv (handle, (char*)buffer, size, flags);
}


int Socket::recvfrom ( void * buffer, int size,
                          int flags, IPAddress* from )
{
  assert ( handle != -1 ) ;
  socklen_t fromlen = (socklen_t) sizeof(IPAddress) ;
  return ::recvfrom(handle,(char*)buffer,size,flags,(sockaddr*)from,&fromlen);
}


void Socket::close (void)
{
  if ( handle != -1 )
  {
#if defined(WINSOCK)
    ::closesocket( handle );
#else
    ::close( handle );
#endif
    handle = -1 ;
  }
}


bool Socket::isNonBlockingError ()
{
#if defined(WINSOCK)
  int wsa_errno = WSAGetLastError();
  if ( wsa_errno != 0 )
  {
    WSASetLastError(0);
	SG_LOG(SG_IO, SG_WARN, "isNonBlockingError: WSAGetLastError():" << wsa_errno);
    switch (wsa_errno) {
    case WSAEWOULDBLOCK: // always == NET_EAGAIN?
    case WSAEALREADY:
    case WSAEINPROGRESS:
      return true;
    }
  }
  return false;
#else
  switch (errno) {
  case EWOULDBLOCK: // always == NET_EAGAIN?
  case EALREADY:
  case EINPROGRESS:
    return true;
  }
  return false;

#endif
}


//////////////////////////////////////////////////////////////////////
//
//	modified version by os
//
//////////////////////////////////////////////////////////////////////
int Socket::select ( Socket** reads, Socket** writes, int timeout )
{
  fd_set r,w;
  int	retval;
  
  FD_ZERO (&r);
  FD_ZERO (&w);

  int i, k ;
  int num = 0 ;

  if ( reads )
  {
    for ( i=0; reads[i]; i++ )
    {
      int fd = reads[i]->getHandle();
      FD_SET (fd, &r);
      num++;
    }
  }

  if ( writes )
  {
    for ( i=0; writes[i]; i++ )
    {
      int fd = writes[i]->getHandle();
      FD_SET (fd, &w);
      num++;
    }
  }

  if (!num)
    return num ;

  /* Set up the timeout */
  struct timeval tv ;
  tv.tv_sec = timeout/1000;
  tv.tv_usec = (timeout%1000)*1000;

  // It bothers me that select()'s first argument does not appear to
  // work as advertised... [it hangs like this if called with
  // anything less than FD_SETSIZE, which seems wasteful?]
  
  // Note: we ignore the 'exception' fd_set - I have never had a
  // need to use it.  The name is somewhat misleading - the only
  // thing I have ever seen it used for is to detect urgent data -
  // which is an unportable feature anyway.

  retval = ::select (FD_SETSIZE, &r, &w, 0, &tv);

  //remove sockets that had no activity

  num = 0 ;

  if ( reads )
  {
    for ( k=i=0; reads[i]; i++ )
    {
      int fd = reads[i]->getHandle();
      if ( FD_ISSET (fd, &r) )
      {
        reads[k++] = reads[i];
        num++;
      }
    }
    reads[k] = NULL ;
  }

  if ( writes )
  {
    for ( k=i=0; writes[i]; i++ )
    {
      int fd = writes[i]->getHandle();
      if ( FD_ISSET (fd, &w) )
      {
        writes[k++] = writes[i];
        num++;
      }
    }
    writes[k] = NULL ;
  }

  if (retval == 0) // timeout
    return (-2);
  if (retval == -1)// error
    return (-1);

  return num ;
}


/* Init/Exit functions */

static void netExit ( void )
{
#if defined(WINSOCK)
	/* Clean up windows networking */
	if ( WSACleanup() == SOCKET_ERROR ) {
		if ( WSAGetLastError() == WSAEINPROGRESS ) {
			WSACancelBlockingCall();
			WSACleanup();
		}
	}
#endif
}

int Socket::initSockets()
{
  assert ( sizeof(sockaddr_in) == sizeof(IPAddress) ) ;

#if defined(WINSOCK)
	/* Start up the windows networking */
	WORD version_wanted = MAKEWORD(1,1);
	WSADATA wsaData;

	if ( WSAStartup(version_wanted, &wsaData) != 0 ) {
		throw sg_exception("WinSock initialization failed");
	}
#endif

  atexit( netExit ) ;
	return(0);
}


} // of namespace simgear

