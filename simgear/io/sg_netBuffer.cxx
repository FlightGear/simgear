// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 1998, 2002 Steve Baker

/**
 * @file
 * @brief Copied from PLIB into SimGear
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
