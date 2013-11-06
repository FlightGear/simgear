/*
    NetChannel -  copied from PLIB

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
 
     For further information visit http://plib.sourceforge.net

     $Id: netChannel.h 1899 2004-03-21 17:41:07Z sjbaker $
*/

/****
* NAME
*   netChannel - network channel class
*
* DESCRIPTION
*   netChannel is adds event-handling to the low-level
*   netSocket class.  Otherwise, it can be treated as
*   a normal non-blocking socket object.
*
*   The direct interface between the netPoll() loop and
*   the channel object are the handleReadEvent and
*   handleWriteEvent methods. These are called
*   whenever a channel object 'fires' that event.
*
*   The firing of these low-level events can tell us whether
*   certain higher-level events have taken place, depending on
*   the timing and state of the connection.
*
* AUTHORS
*   Sam Rushing <rushing@nightmare.com> - original version for Medusa
*   Dave McClurg <dpm@efn.org> - modified for use in PLIB
*
* CREATION DATE
*   Dec-2000
*
****/

#ifndef SG_NET_CHANNEL_H
#define SG_NET_CHANNEL_H

#include <simgear/io/raw_socket.hxx>

#include <string>
#include <vector>

namespace simgear
{

class NetChannelPoller;
    
class NetChannel : public Socket
{
  bool closed, connected, accepting, write_blocked, should_delete, resolving_host ;
  std::string host;
  int port;
  
    friend class NetChannelPoller;
    NetChannelPoller* poller;
public:

  NetChannel () ;
  virtual ~NetChannel () ;

  void setHandle (int s, bool is_connected = true);
  bool isConnected () const { return connected; }
  bool isClosed () const { return closed; }
  void shouldDelete () { should_delete = true ; }

  // --------------------------------------------------
  // socket methods
  // --------------------------------------------------
  
  bool  open    ( void ) ;
  void  close   ( void ) ;
  int   listen  ( int backlog ) ;
  int   connect ( const char* host, int port ) ;
  int   send    ( const void * buf, int size, int flags = 0 ) ;
  int   recv    ( void * buf, int size, int flags = 0 ) ;

  // poll() eligibility predicates
  virtual bool readable (void) { return (connected || accepting); }
  virtual bool writable (void) { return (!connected || write_blocked); }
  
  // --------------------------------------------------
  // event handlers
  // --------------------------------------------------
  
  void handleReadEvent (void);
  void handleWriteEvent (void);
  int handleResolve (void);
  
// These are meant to be overridden.
  virtual void handleClose (void) {
  }
  
  virtual void handleRead (void);
  virtual void handleWrite (void);
  virtual void handleAccept (void);
  virtual void handleError (int error);

};

class NetChannelPoller
{
    typedef std::vector<NetChannel*> ChannelList;
    ChannelList channels;
public:
    void addChannel(NetChannel* channel);
    void removeChannel(NetChannel* channel);
    
    bool hasChannels() const { return !channels.empty(); }
    
    bool poll(unsigned int timeout = 0);
    void loop(unsigned int timeout = 0);
};

} // of namespace simgear

#endif // SG_NET_CHANNEL_H
