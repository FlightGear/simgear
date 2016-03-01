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
     Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
*/

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include <simgear/compiler.h>
#include "raw_socket.hxx"

#if defined(_WIN32) && !defined(__CYGWIN__)
# define WINSOCK // use winsock convetions, otherwise standard POSIX
#endif

#include <cstdlib>
#include <ctime>
#include <cstring>
#include <cassert>
#include <cstdio> // for snprintf
#include <errno.h>

#if defined(WINSOCK)
#  include <winsock2.h>
#  include <ws2tcpip.h>
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

#include <map>

#include <simgear/debug/logstream.hxx>
#include <simgear/structure/exception.hxx>
#include <simgear/threads/SGThread.hxx>

using std::string;

namespace {

class Resolver : public SGThread
{
public:
    static Resolver* instance()
    {
        if (!static_instance) {
            static_instance = new Resolver;
            atexit(&Resolver::cleanup);
            static_instance->start();
        }

        return static_instance;
    }

    static void cleanup()
    {
        static_instance->shutdown();
        static_instance->join();
    }

    Resolver() :
        _done(false)
    {
    }

    void shutdown()
    {
        _lock.lock();
        _done = true;
        _wait.signal();
        _lock.unlock();
    }

    simgear::IPAddress* lookup(const string& host)
    {
        simgear::IPAddress* result = NULL;
        _lock.lock();
        AddressCache::iterator it = _cache.find(host);
        if (it == _cache.end()) {
            _cache[host] = NULL; // mark as needing looked up
            _wait.signal(); // if the thread was sleeping, poke it
        } else {
            result = it->second;
        }
        _lock.unlock();
        return result;
    }

    simgear::IPAddress* lookupSync(const string& host)
    {
        simgear::IPAddress* result = NULL;
        _lock.lock();
        AddressCache::iterator it = _cache.find(host);
        if (it == _cache.end()) {
            _lock.unlock();
            result = new simgear::IPAddress;
            bool ok = lookupHost(host.c_str(), *result);
            _lock.lock();
            if (ok) {
                _cache[host] = result; // mark as needing looked up
            } else {
                delete result;
                result = NULL;
            }
        } else { // found in cache, easy
            result = it->second;
        }
        _lock.unlock();
        return result;
    }
protected:
    /**
     * run method waits on a condition (_wait), and when awoken,
     * finds any unresolved entries in _cache, resolves them, and goes
     * back to sleep.
     */
    virtual void run()
    {
        _lock.lock();
        while (!_done) {
            AddressCache::iterator it;

            for (it = _cache.begin(); it != _cache.end(); ++it) {
                if (it->second == NULL) {
                    string h = it->first;

                    _lock.unlock();
                    simgear::IPAddress* addr = new simgear::IPAddress;
                // may take seconds or even minutes!
                    lookupHost(h.c_str(), *addr);
                    _lock.lock();

                // cahce may have changed while we had the lock released -
                // so iterators may be invalid: restart the traversal
                    it = _cache.begin();
                    _cache[h] = addr;
                } // of found un-resolved entry
            } // of un-resolved address iteration
            _wait.wait(_lock);
        } // of thread run loop
        _lock.unlock();
    }
private:
    static Resolver* static_instance;

    /**
     * The actual synchronous, blocking host lookup function
     * do *not* call this with any locks (mutexs) held, since depending
     * on local system configuration / network availability, it
     * may block for seconds or minutes.
     */
    bool lookupHost(const char* host, simgear::IPAddress& addr)
    {
      struct addrinfo hints;
      memset(&hints, 0, sizeof(struct addrinfo));
      hints.ai_family = AF_INET;
      bool ok = false;

      struct addrinfo* result0 = NULL;
      int err = getaddrinfo(host, NULL, &hints, &result0);
      if (err) {
        SG_LOG(SG_IO, SG_WARN, "getaddrinfo failed for '" << host << "' : " << gai_strerror(err));
        return false;
      } else {
          struct addrinfo* result;
          for (result = result0; result != NULL; result = result->ai_next) {
              if (result->ai_family != AF_INET) { // only accept IP4 for the moment
                  continue;
              }

              if (result->ai_addrlen != addr.getAddrLen()) {
                  SG_LOG(SG_IO, SG_ALERT, "mismatch in socket address sizes: got " <<
                      result->ai_addrlen << ", expected " << addr.getAddrLen());
                  continue;
              }

              memcpy(addr.getAddr(), result->ai_addr, result->ai_addrlen);
              ok = true;
              break;
          } // of getaddrinfo results iteration
      } // of getaddrinfo succeeded

      freeaddrinfo(result0);
      return ok;
    }

    SGMutex _lock;
    SGWaitCondition _wait;

    typedef std::map<string, simgear::IPAddress*> AddressCache;
    AddressCache _cache;
    bool _done;
};

Resolver* Resolver::static_instance = NULL;

} // of anonymous namespace

namespace simgear
{

IPAddress::IPAddress ( const char* host, int port )
{
  set ( host, port ) ;
}

IPAddress::IPAddress( const IPAddress& other ) :
  addr(NULL)
{
  if (other.addr) {
    addr = (struct sockaddr_in*) malloc(sizeof(struct sockaddr_in));
    memcpy(addr, other.addr, sizeof(struct sockaddr_in));
  }
}

const IPAddress& IPAddress::operator=(const IPAddress& other)
{
  if (addr) {
    free(addr);
    addr = NULL;
  }

  if (other.addr) {
    addr = (struct sockaddr_in*) malloc(sizeof(struct sockaddr_in));
    memcpy(addr, other.addr, sizeof(struct sockaddr_in));
  }

  return *this;
}

void IPAddress::set ( const char* host, int port )
{
  addr = (struct sockaddr_in*) malloc(sizeof(struct sockaddr_in));
  memset(addr, 0, sizeof(struct sockaddr_in));

  addr->sin_family = AF_INET ;
  addr->sin_port = htons (port);

  /* Convert a string specifying a host name or one of a few symbolic
  ** names to a numeric IP address.  This usually calls getaddrinfo()
  ** to do the work; the names "" and "<broadcast>" are special.
  */

  if (!host || host[0] == '\0') {
    addr->sin_addr.s_addr = INADDR_ANY;
    return;
  }

  if (strcmp(host, "<broadcast>") == 0) {
    addr->sin_addr.s_addr = INADDR_BROADCAST;
    return;
  }

// check the cache
  IPAddress* cached = Resolver::instance()->lookupSync(host);
  if (cached) {
      memcpy(addr, cached->getAddr(), cached->getAddrLen());
  }

  addr->sin_port = htons (port); // fix up port after getaddrinfo
}

IPAddress::~IPAddress()
{
  if (addr) {
    free (addr);
  }
}

bool IPAddress::lookupNonblocking(const char* host, IPAddress& addr)
{
    IPAddress* cached = Resolver::instance()->lookup(host);
    if (!cached) {
        return false;
    }

    addr = *cached;
    return true;
}

/* Create a string object representing an IP address.
   This is always a string of the form 'dd.dd.dd.dd' (with variable
   size numbers). */

const char* IPAddress::getHost () const
{
    if (!addr) {
        return NULL;
    }
    
  static char buf [32];
	long x = ntohl(addr->sin_addr.s_addr);
	sprintf(buf, "%d.%d.%d.%d",
		(int) (x>>24) & 0xff, (int) (x>>16) & 0xff,
		(int) (x>> 8) & 0xff, (int) (x>> 0) & 0xff );
  return buf;
}

unsigned int IPAddress::getIP () const
{
    if (!addr) {
        return 0;
    }
    
	return addr->sin_addr.s_addr;
}

unsigned int IPAddress::getPort() const
{
    if (!addr) {
        return 0;
    }
    
  return ntohs(addr->sin_port);
}

void IPAddress::setPort(int port)
{
    if (!addr) {
        return;
    }
    
    addr->sin_port = htons(port);
}

unsigned int IPAddress::getFamily () const
{
    if (!addr) {
        return 0;
    }
    
	return addr->sin_family;
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
    if (!addr) {
        return false;
    }
    
    return (addr->sin_addr.s_addr == INADDR_BROADCAST);
}

unsigned int IPAddress::getAddrLen() const
{
    return sizeof(struct sockaddr_in);
}

struct sockaddr* IPAddress::getAddr() const
{
    if (addr == NULL) {
        addr = (struct sockaddr_in*) malloc(sizeof(struct sockaddr_in));
        memset(addr, 0, sizeof(struct sockaddr_in));
    }

    return (struct sockaddr*) addr;
}

bool IPAddress::isValid() const
{
    return (addr != NULL);
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

#ifdef SO_NOSIGPIPE
  // Do not generate SIGPIPE signal (which immediately terminates the program),
  // instead ::send() will return -1 and errno will be set to EPIPE.
  // SO_NOSIGPIPE should be available on Mac/BSD systems, but is not available
  // within Posix/Linux.
  // This only works for calls to ::send() but not for ::write():
  // http://freebsd.1045724.n5.nabble.com/is-setsockopt-SO-NOSIGPIPE-work-tp4011054p4011055.html
  int set = 1;
  setsockopt(handle, SOL_SOCKET, SO_NOSIGPIPE, (void*)&set, sizeof(set));
#endif

#ifndef MSG_NOSIGNAL
# define MSG_NOSIGNAL 0
#endif
  // TODO supress SIGPIPE if neither SO_NOSIGPIPE nor MSG_NOSIGNAL is available
  // http://krokisplace.blogspot.co.at/2010/02/suppressing-sigpipe-in-library.html

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
  if( (result = ::bind(handle, addr.getAddr(), addr.getAddrLen() ) ) < 0 ) {
    SG_LOG(SG_IO, SG_ALERT, "bind(" << addr.getHost() << ":" << addr.getPort() << ") failed. Errno " << errno << " (" << strerror(errno) << ")");
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
  else if( (result = ::bind(handle,addr.getAddr(), addr.getAddrLen())) < 0 ) {
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
    socklen_t addr_len = addr->getAddrLen(); ;
    return ::accept(handle, addr->getAddr(), &addr_len);
  }
}


int Socket::connect ( const char* host, int port )
{
  IPAddress addr ( host, port ) ;
  return connect ( &addr );
}


int Socket::connect ( IPAddress* addr )
{
  assert ( handle != -1 ) ;
  if ( addr->getBroadcast() ) {
      setBroadcast( true );
  }
  return ::connect(handle, addr->getAddr(), addr->getAddrLen() );
}


int Socket::send (const void * buffer, int size, int flags)
{
  assert ( handle != -1 ) ;
  return ::send (handle, (const char*)buffer, size, flags | MSG_NOSIGNAL);
}


int Socket::sendto ( const void * buffer, int size,
                        int flags, const IPAddress* to )
{
  assert ( handle != -1 ) ;
  return ::sendto( handle,
                   (const char*)buffer,
                   size,
                   flags | MSG_NOSIGNAL,
                   (const sockaddr*)to->getAddr(),
                   to->getAddrLen() );
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
  socklen_t fromlen = (socklen_t) from->getAddrLen() ;
  return ::recvfrom(handle,(char*)buffer,size,flags, from->getAddr(),&fromlen);
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

int Socket::errorNumber()
{
#if defined(WINSOCK)
    return WSAGetLastError();
#else
    return errno;
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

