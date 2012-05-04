/*
     simgear::Socket, adapted from PLIB netSocket by James Turner
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
class IPAddress
{
    mutable struct sockaddr_in* addr;
public:
  IPAddress () : addr(0) {}
  IPAddress ( const char* host, int port ) ;
  ~IPAddress();
  
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

  bool getBroadcast () const ;
  
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
  
  Socket () ;
  virtual ~Socket () ;

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
  
  static int select ( Socket** reads, Socket** writes, int timeout ) ;
} ;

//const char* netFormat ( const char* fmt, ... ) ;

} // of namespace simgear

#endif // SG_IO_SOCKET_HXX

