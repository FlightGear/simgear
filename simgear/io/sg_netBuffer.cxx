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

     $Id: netBuffer.cxx 1568 2002-09-02 06:05:49Z sjbaker $
*/

#include <simgear_config.h>
#include "sg_netBuffer.hxx"

#include <cassert>
#include <cstring>

#include <simgear/debug/logstream.hxx>

namespace simgear {
  
NetBuffer::NetBuffer( int _max_length )
{
  length = 0 ;
  max_length = _max_length ;
  data = new char [ max_length+1 ] ;  //for null terminator
}

NetBuffer::~NetBuffer ()
{
  delete[] data ;
}

void NetBuffer::remove ()
{
  length = 0 ;
}

void NetBuffer::remove (int pos, int n)
{
  assert (pos>=0 && pos<length && (pos+n)<=length) ;
  //if (pos>=0 && pos<length && (pos+n)<=length)
  {
    memmove(&data[pos],&data[pos+n],length-(pos+n)) ;
    length -= n ;
  }
}

bool NetBuffer::append (const char* s, int n)
{
  if ((length+n)<=max_length)
  {
    memcpy(&data[length],s,n) ;
    length += n ;
    return true ;
  }
  return false ;
}

bool NetBuffer::append (int n)
{
  if ((length+n)<=max_length)
  {
    length += n ;
    return true ;
  }
  return false ;
}

NetBufferChannel::NetBufferChannel (int in_buffer_size, int out_buffer_size) :
    in_buffer (in_buffer_size),
    out_buffer (out_buffer_size),
    should_close (0)
{ /* empty */
}

void NetBufferChannel::handleClose ( void )
{
  in_buffer.remove () ;
  out_buffer.remove () ;
  should_close = 0 ;
  NetChannel::handleClose () ;
}


bool NetBufferChannel::bufferSend (const char* msg, int msg_len)
{
  if ( out_buffer.append(msg,msg_len) )
    return true ;
    
  SG_LOG(SG_IO, SG_WARN, "NetBufferChannel: output buffer overflow!" ) ;
  return false ;
}

void NetBufferChannel::handleBufferRead (NetBuffer& buffer)
{
  /* do something here */
  buffer.remove();
}
  
void
NetBufferChannel::handleRead (void)
{
  int max_read = in_buffer.getMaxLength() - in_buffer.getLength() ;
  if (max_read)
  {
    char* data = in_buffer.getData() + in_buffer.getLength() ;
    int num_read = recv (data, max_read) ;
    if (num_read > 0)
    {
      in_buffer.append (num_read) ;
      //ulSetError ( UL_DEBUG, "netBufferChannel: %d read", num_read ) ;
    }
  }
  if (in_buffer.getLength())
  {
    handleBufferRead (in_buffer);
  }
}

void
NetBufferChannel::handleWrite (void)
{
  if (out_buffer.getLength())
  {
    if (isConnected())
    {
      int length = out_buffer.getLength() ;
      if (length>512)
        length=512;
      int num_sent = NetChannel::send (
        out_buffer.getData(), length);
      if (num_sent > 0)
      {
        out_buffer.remove (0, num_sent);
        //ulSetError ( UL_DEBUG, "netBufferChannel: %d sent", num_sent ) ;
      }
    }
  }
  else if (should_close)
  {
    close();
  }
}

} // of namespace simgear
