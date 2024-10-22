// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 1998, 2002 Steve Baker

/**
 * @file
 * @brief Copied from PLIB into SimGear
*/

#include <simgear/io/sg_netChat.hxx>

#include <cstring>
#include <cstdlib>
#include <algorithm>

namespace  simgear {

void
NetChat::setTerminator(const std::string& t)
{
  terminator = t;
  bytesToCollect = -1;
}

const char*
NetChat::getTerminator() const
{
  return terminator.c_str();
}


void
NetChat::setByteCount(int count)
{
    terminator.clear();
    bytesToCollect = count;
}

// return the size of the largest prefix of needle at the end
// of haystack

#define MAX(a,b) (((a)>(b))?(a):(b))

static int
find_prefix_at_end(const NetBuffer& haystack, const std::string& needle)
{
  const char* hd = haystack.getData();
  int hl = haystack.getLength();
  int nl = needle.length();
  
  for (int i = MAX (nl-hl, 0); i < nl; i++) {
    //if (haystack.compare (needle, hl-(nl-i), nl-i) == 0) {
    if (memcmp(needle.c_str(), &hd[hl-(nl-i)], nl-i) == 0) {
      return (nl-i);
    }
  }
  return 0;
}

static int
find_terminator(const NetBuffer& haystack, const std::string& needle)
{
  if( !needle.empty() )
  {
    const char* data = haystack.getData();
    const char* ptr = strstr(data,needle.c_str());
    if (ptr != NULL)
      return(ptr-data);
  }
  return -1;
}

bool NetChat::push (const char* s)
{
  return bufferSend ( s, strlen(s) ) ;
}

void
NetChat::handleBufferRead (NetBuffer& in_buffer)
{
  // Continue to search for terminator in in_buffer,
  // while calling collect_incoming_data.  The while loop is
  // necessary because we might read several data+terminator combos
  // with a single recv().
  
  while (in_buffer.getLength()) {      
    // special case where we're not using a terminator
    if ( terminator.empty() ) {
        if ( bytesToCollect > 0) {
            const int toRead = std::min(in_buffer.getLength(), bytesToCollect);
            collectIncomingData(in_buffer.getData(), toRead);
            in_buffer.remove(0, toRead);
            bytesToCollect -= toRead;
            if (bytesToCollect ==  0) { // read all requested bytes
                foundTerminator();
            }
        } else { // read the whole lot
            collectIncomingData (in_buffer.getData(),in_buffer.getLength());
            in_buffer.remove ();
        }
        
        continue;
    }
    
    int index = find_terminator ( in_buffer, terminator ) ;
    
    // 3 cases:
    // 1) end of buffer matches terminator exactly:
    //    collect data, transition
    // 2) end of buffer matches some prefix:
    //    collect data to the prefix
    // 3) end of buffer does not match any prefix:
    //    collect data
    
    if (index != -1) {
      // we found the terminator
      collectIncomingData ( in_buffer.getData(), index ) ;
      in_buffer.remove (0, index + terminator.length());
      foundTerminator();
    } else {
      // check for a prefix of the terminator
      int num = find_prefix_at_end (in_buffer, terminator);
      if (num) {
        int bl = in_buffer.getLength();
        // we found a prefix, collect up to the prefix
        collectIncomingData ( in_buffer.getData(), bl - num ) ;
        in_buffer.remove (0, bl - num);
        break;
      } else {
        // no prefix, collect it all
        collectIncomingData (in_buffer.getData(), in_buffer.getLength());
        in_buffer.remove();
      }
    }
  }
}

} // of namespace simgear


