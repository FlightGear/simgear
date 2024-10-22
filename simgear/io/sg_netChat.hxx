// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 1998, 2002 Steve Baker

/**
 * @file
 * @brief Network chat class
 *
 * This class adds support for 'chat' style protocols -
 * where one side sends a 'command', and the other sends
 * a response (examples would be the common internet
 * protocols - smtp, nntp, ftp, etc..).
 *
 * The `handle_buffer_read()` method looks at the input
 * stream for the current 'terminator' (usually `\r\n`
 * for single-line responses, `\r\n.\r\n` for multi-line
 * output), calling `found_terminator()` on its receipt.
 *
 * Say you build an nntp client using this class.
 * At the start of the connection, you'll have
 * terminator set to `\r\n`, in order to process
 * the single-line greeting.  Just before issuing a
 * 'LIST' command you'll set it to `\r\n.\r\n`.
 * The output of the LIST command will be accumulated
 * (using your own 'collect_incoming_data' method)
 * up to the terminator, and then control will be
 * returned to you - by calling your found_terminator()
*/

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
