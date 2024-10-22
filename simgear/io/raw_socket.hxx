// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 1998, 2002 Steve Baker
// SPDX-FileCopyrightText: 2000 Curtis L. Olson - http://www.flightgear.org/~curt
// SPDX-FileCopyrightText: 2010 James Turner

/**
 * @file
 * @brief Adapted from PLIB netSocket by James Turner
 */

#ifndef SG_IO_SOCKET_HXX
#define SG_IO_SOCKET_HXX

//#include <errno.h>

//#if defined(__APPLE__) || defined(__FreeBSD__)
//#  include <netinet/in.h>
//#endif

struct sockaddr_in;
struct sockaddr;
     
namespace simgear
{

/*
 * Socket address, internet style.
 */
class IPAddress final
{
    mutable struct sockaddr_in* addr;
public:
  IPAddress () : addr(0) {}
  IPAddress ( const char* host, int port ) ;
  ~IPAddress();   // non-virtual is intentional
  
  static bool lookupNonblocking(const char* host, IPAddress& addr);
  
  IPAddress( const IPAddress& other );
  const IPAddress& operator=(const IPAddress& other);

  bool isValid () const;
  void set ( const char* host, int port ) ;
  const char* getHost () const ;
  unsigned int getPort() const ;
  void setPort(int port);
  
  unsigned int getIP () const ;
  unsigned int getFamily () const ;
  static const char* getLocalHost () ;
  
  unsigned int getAddrLen() const;
  sockaddr* getAddr() const;
};


/*
 * Socket type
 */
class Socket
{
  int handle ;
  
public:
  
  Socket();
  virtual ~Socket();

  static int initSockets();

  int getHandle () const { return handle; }
  void setHandle (int handle) ;
  
  bool  open        ( bool stream=true ) ;
  void  close       ( void ) ;
  int   bind        ( const char* host, int port ) ;
  int   listen	    ( int backlog ) ;
  int   accept      ( IPAddress* addr ) ;
  int   connect     ( const char* host, int port ) ;
  int   connect     ( IPAddress* addr ) ;
  int   send	    ( const void * buffer, int size, int flags = 0 ) ;
  int   sendto      ( const void * buffer, int size, int flags, const IPAddress* to ) ;
  int   recv	    ( void * buffer, int size, int flags = 0 ) ;
  int   recvfrom    ( void * buffer, int size, int flags, IPAddress* from ) ;

  void setBlocking ( bool blocking ) ;
  void setBroadcast ( bool broadcast ) ;

  static bool isNonBlockingError () ;
  static int errorNumber();

  static int select ( Socket** reads, Socket** writes, int timeout ) ;
} ;

//const char* netFormat ( const char* fmt, ... ) ;

} // of namespace simgear

#endif // SG_IO_SOCKET_HXX

