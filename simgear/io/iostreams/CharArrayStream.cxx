// -*- coding: utf-8 -*-
//
// CharArrayStream.cxx --- IOStreams classes for reading from, and writing to
//                         char arrays
//
// Copyright (C) 2017  Florent Rougon
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
// MA  02110-1301  USA.

#include <simgear_config.h>

#include <ios>                  // std::streamsize
#include <istream>              // std::istream and std::iostream
#include <ostream>              // std::ostream
#include <type_traits>          // std::make_unsigned()
#include <cstddef>              // std::size_t, std::ptrdiff_t
#include <cassert>

#include "CharArrayStream.hxx"

using traits = std::char_traits<char>;


namespace simgear
{

// ***************************************************************************
// *                        CharArrayStreambuf class                         *
// ***************************************************************************

CharArrayStreambuf::CharArrayStreambuf(char* buf, std::size_t bufSize)
  : _buf(buf),
    _bufSize(bufSize)
{
  setg(_buf, _buf, _buf + _bufSize);
  setp(_buf, _buf + _bufSize);
}

char* CharArrayStreambuf::data() const
{
  return _buf;
}

std::size_t CharArrayStreambuf::size() const
{
  return _bufSize;
}

int CharArrayStreambuf::underflow()
{
  return (gptr() == egptr()) ? traits::eof() : traits::to_int_type(*gptr());
}

int CharArrayStreambuf::overflow(int c)
{
  // cf. §27.7.1, footnote 309 of the C++11 standard
  if (traits::eq_int_type(c, traits::eof())) {
    return traits::not_eof(c);
  } else {
    // This class never writes beyond the end of the array (_buf + _bufSize)
    return traits::eof();
  }
}

std::streamsize CharArrayStreambuf::xsgetn(char* dest, std::streamsize n)
{
  assert(n >= 0);
  std::ptrdiff_t avail = egptr() - gptr();
  // Compute min(avail, n). The cast is safe, because in its branch, one has
  // 0 <= n < avail, which is of type std::ptrdiff_t.
  std::ptrdiff_t nbChars = ( (n >= avail) ?
                             avail : static_cast<std::ptrdiff_t>(n) );
  std::copy(gptr(), gptr() + nbChars, dest);
  // eback() == _buf and egptr() == _buf + _bufSize
  // I don't use gbump(), because it takes an int...
  setg(eback(), gptr() + nbChars, egptr());

  // Cast safe because 0 <= nbChars <= n, which is of type std::streamsize
  return static_cast<std::streamsize>(nbChars); // number of chars copied
}

std::streamsize CharArrayStreambuf::xsputn(const char* s, std::streamsize n)
{
  assert(n >= 0);
  std::ptrdiff_t availSpace = epptr() - pptr();
  // Compute min(availSpace, n). The cast is safe, because in its branch, one
  // has 0 <= n < availSpace, which is of type std::ptrdiff_t.
  std::ptrdiff_t nbChars = ( (n >= availSpace) ?
                             availSpace : static_cast<std::ptrdiff_t>(n) );
  std::copy(s, s + nbChars, pptr());
  // epptr() == _buf + _bufSize
  // I don't use pbump(), because it takes an int...
  setp(pptr() + nbChars, epptr());

  // Cast safe because 0 <= nbChars <= n, which is of type std::streamsize
  return static_cast<std::streamsize>(nbChars); // number of chars copied
}

std::streamsize CharArrayStreambuf::showmanyc()
{
  // It is certain that underflow() will return EOF if gptr() == egptr().
  return -1;
}

std::streampos CharArrayStreambuf::seekoff(std::streamoff off,
                                           std::ios_base::seekdir way,
                                           std::ios_base::openmode which)
{
  bool positionInputSeq = false;
  bool positionOutputSeq = false;
  char* ptr = nullptr;

  // cf. §27.8.2.4 of the C++11 standard
  if ((which & std::ios_base::in) == std::ios_base::in) {
    positionInputSeq = true;
    ptr = gptr();
  }

  if ((which & std::ios_base::out) == std::ios_base::out) {
    positionOutputSeq = true;
    ptr = pptr();
  }

  if ((!positionInputSeq && !positionOutputSeq) ||
      (positionInputSeq && positionOutputSeq &&
       way != std::ios_base::beg && way != std::ios_base::end)) {
    return std::streampos(std::streamoff(-1));
  }

  // If we reached this point and (positionInputSeq && positionOutputSeq),
  // then (way == std::ios_base::beg || way == std::ios_base::end) and
  // therefore 'ptr' won't be used.
  std::streamoff refOffset;
  static_assert(sizeof(std::streamoff) >= sizeof(std::ptrdiff_t),
                "Unexpected: sizeof(std::streamoff) < sizeof(std::ptrdiff_t)");
  static_assert(sizeof(std::streamoff) >= sizeof(std::size_t),
                "Unexpected: sizeof(std::streamoff) < sizeof(std::size_t)");

  if (way == std::ios_base::beg) {
    refOffset = 0;
  } else if (way == std::ios_base::cur) {
    refOffset = static_cast<std::streamoff>(ptr - _buf);
  } else {
    assert(way == std::ios_base::end);
    refOffset = static_cast<std::streamoff>(_bufSize);
  }

  // Offset, relatively to _buf, where we are supposed to seek
  std::streamoff totalOffset = refOffset + off;
  typedef typename std::make_unsigned<std::streamoff>::type uStreamOff;

  if (totalOffset < 0 || static_cast<uStreamOff>(totalOffset) > _bufSize) {
    return std::streampos(std::streamoff(-1));
  } else {
    // Safe because 0 <= totalOffset <= _bufSize, which is an std::size_t
    char* newPtr = _buf + static_cast<std::size_t>(totalOffset);

    if (positionInputSeq) {
      // eback() == _buf and egptr() == _buf + _bufSize
      setg(eback(), newPtr, egptr());
    }

    if (positionOutputSeq) {
      // epptr() == _buf + _bufSize
      setp(newPtr, epptr());
    }

    // C++11's §27.8.2.4 item 12 (for stringbuf) would return refOffset. This
    // makes no sense IMHO, in particular when 'way' is std::ios_base::beg or
    // std::ios_base::end. Return the new offset (from the beginning of
    // '_buf') instead. Note that this doesn't violate anything, because
    // §27.6.3.4.2 grants full freedom as to the semantics of seekoff() to
    // classes derived from basic_streambuf.
    //
    // My interpretation is consistent with items 13 and 14 of §27.8.2.4
    // concerning seekpos(), whereas item 12 is not (if item 12 were followed
    // to the letter, seekoff() would always return 0 on success when
    // way == std::ios_base::beg, and therefore items 13 and 14 would be
    // incompatible).
    return std::streampos(totalOffset);
  }
}

std::streampos CharArrayStreambuf::seekpos(std::streampos pos,
                                           std::ios_base::openmode which)
{
  return seekoff(std::streamoff(pos), std::ios_base::beg, which);
}

// ***************************************************************************
// *                       ROCharArrayStreambuf class                        *
// ***************************************************************************
ROCharArrayStreambuf::ROCharArrayStreambuf(const char* buf, std::size_t bufSize)
  : CharArrayStreambuf(const_cast<char*>(buf), bufSize)
{ }

const char* ROCharArrayStreambuf::data() const
{
  return const_cast<const char*>(CharArrayStreambuf::data());
}

int ROCharArrayStreambuf::overflow(int c)
{
  return traits::eof();         // indicate failure
}

std::streamsize ROCharArrayStreambuf::xsputn(const char* s, std::streamsize n)
{
  return 0;                     // number of chars written
}

// ***************************************************************************
// *                         CharArrayIStream class                          *
// ***************************************************************************

CharArrayIStream::CharArrayIStream(const char* buf, std::size_t bufSize)
  : std::istream(nullptr),
    _streamBuf(buf, bufSize)
{
  // Associate _streamBuf to 'this' and clear the error state flags
  rdbuf(&_streamBuf);
}

const char* CharArrayIStream::data() const
{
  return _streamBuf.data();
}

std::size_t CharArrayIStream::size() const
{
  return _streamBuf.size();
}

// ***************************************************************************
// *                         CharArrayOStream class                          *
// ***************************************************************************

CharArrayOStream::CharArrayOStream(char* buf, std::size_t bufSize)
  : std::ostream(nullptr),
    _streamBuf(buf, bufSize)
{
  // Associate _streamBuf to 'this' and clear the error state flags
  rdbuf(&_streamBuf);
}

char* CharArrayOStream::data() const
{
  return _streamBuf.data();
}

std::size_t CharArrayOStream::size() const
{
  return _streamBuf.size();
}

// ***************************************************************************
// *                         CharArrayIOStream class                         *
// ***************************************************************************

CharArrayIOStream::CharArrayIOStream(char* buf, std::size_t bufSize)
  : std::iostream(nullptr),
    _streamBuf(buf, bufSize)
{
  // Associate _streamBuf to 'this' and clear the error state flags
  rdbuf(&_streamBuf);
}

char* CharArrayIOStream::data() const
{
  return _streamBuf.data();
}

std::size_t CharArrayIOStream::size() const
{
  return _streamBuf.size();
}

} // of namespace simgear
