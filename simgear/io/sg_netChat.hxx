/*
    Copied from PLIB into SimGear

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

     $Id: netChat.h 1568 2002-09-02 06:05:49Z sjbaker $
*/

/****
* NAME
*   netChat - network chat class
*
* DESCRIPTION
*   This class adds support for 'chat' style protocols -
*   where one side sends a 'command', and the other sends
*   a response (examples would be the common internet
*   protocols - smtp, nntp, ftp, etc..).
*
*   The handle_buffer_read() method looks at the input
*   stream for the current 'terminator' (usually '\r\n'
*   for single-line responses, '\r\n.\r\n' for multi-line
*   output), calling found_terminator() on its receipt.
*
* EXAMPLE
*   Say you build an nntp client using this class.
*   At the start of the connection, you'll have
*   terminator set to '\r\n', in order to process
*   the single-line greeting.  Just before issuing a
*   'LIST' command you'll set it to '\r\n.\r\n'.
*   The output of the LIST command will be accumulated
*   (using your own 'collect_incoming_data' method)
*   up to the terminator, and then control will be
*   returned to you - by calling your found_terminator()
*
* AUTHORS
*   Sam Rushing <rushing@nightmare.com> - original version for Medusa
*   Dave McClurg <dpm@efn.org> - modified for use in PLIB
*
* CREATION DATE
*   Dec-2000
*
****/

#ifndef SG_NET_CHAT_H
#define SG_NET_CHAT_H

#include <simgear/io/sg_netBuffer.hxx>

namespace simgear
{

class NetChat : public NetBufferChannel
{
  std::string terminator;
  int bytesToCollect;

protected:
  virtual void handleBufferRead (NetBuffer& buffer) ;

public:

  NetChat () :
    bytesToCollect(-1) 
  {}

  void setTerminator(const std::string& t);
  const char* getTerminator() const;

  /**
   * set byte count to collect - 'foundTerminator' will be called once
   * this many bytes have been collected
   */
  void setByteCount(int bytes);

  bool push (const char* s);
  
  virtual void collectIncomingData	(const char* s, int n) {}
  virtual void foundTerminator (void) {}
};

} // of namespace simgear

#endif // SG_NET_CHAT_H
