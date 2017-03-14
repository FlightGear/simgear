// -*- coding: utf-8 -*-
//
// CharArrayStream.hxx --- IOStreams classes for reading from, and writing to
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

#ifndef SG_CHAR_ARRAY_STREAM_HXX
#define SG_CHAR_ARRAY_STREAM_HXX

#include <istream>
#include <ostream>
#include <streambuf>
#include <ios>                  // std::streamsize, std::streampos...
#include <cstddef>              // std::size_t

// Contrary to std::stringstream and its (i/o)stringstream friends, the
// classes in this file allow one to work on an array of char (that could be
// for instance static data) without having to make a whole copy of it.
//
// There are five classes defined here in the 'simgear' namespace:
//   - CharArrayStreambuf    subclass of std::streambuf      stream buffer
//   - ROCharArrayStreambuf  subclass of CharArrayStreambuf  stream buffer
//   - CharArrayIStream      subclass of std::istream        input stream
//   - CharArrayOStream      subclass of std::ostream        output stream
//   - CharArrayIOStream     subclass of std::iostream       input/output stream
//
// The main class is CharArrayStreambuf. ROCharArrayStreambuf is a read-only
// subclass of CharArrayStreambuf. The other three are very simple convenience
// classes, using either CharArrayStreambuf or ROCharArrayStreambuf as their
// stream buffer class. One can easily work with CharArrayStreambuf or
// ROCharArrayStreambuf only, either directly or after attaching an instance
// to an std::istream, std::ostream or std::iostream instance (using for
// example constructors like std::istream(std::streambuf* sb) or the rdbuf()
// method of stream classes).

namespace simgear
{

// Input/output stream buffer class that reads from, and writes to a fixed
// buffer in memory specified in the constructor. This buffer must remain
// alive as long as the stream buffer object is used (the CharArrayStreambuf
// class works directly on that buffer without making any copy of it).
//
// Because reads and writes are directly performed on the buffer specified in
// the constructor, this stream buffer class has no caching behavior. You may
// use pubsync() if you like, but that is completely useless by design (it
// uses the default implementation in std::streambuf, which does nothing).
//
// CharArrayStreambuf may share similarities in features with
// std::strstreambuf (deprecated since C++98). However, at least one big
// difference is that CharArrayStreambuf does no dynamic memory allocation
// whatsoever. It works on a fixed-size-fixed-location buffer passed in the
// constructor, and nothing more. It does prevent overflowing the buffer,
// since it knows perfectly well where the buffer starts and ends.
class CharArrayStreambuf: public std::streambuf
{
public:
  explicit CharArrayStreambuf(char* buf, std::size_t bufSize);

  // Accessors for the buffer start pointer and size (same method names as for
  // std::string)
  char* data() const;
  std::size_t size() const;

protected:
  virtual int underflow() override;
  virtual int overflow(int c = std::char_traits<char>::eof()) override;
  // Optional override when subclassing std::streambuf. This is the most
  // efficient way of reading several characters.
  virtual std::streamsize xsgetn(char* dest, std::streamsize n) override;
  // Ditto for writing
  virtual std::streamsize xsputn(const char* s, std::streamsize n) override;
  virtual std::streamsize showmanyc() override;
  virtual std::streampos seekoff(
    std::streamoff off,
    std::ios_base::seekdir way,
    std::ios_base::openmode which = std::ios_base::in | std::ios_base::out)
    override;
  virtual std::streampos seekpos(
    std::streampos pos,
    std::ios_base::openmode which = std::ios_base::in | std::ios_base::out)
    override;

private:
  // These two define the buffer managed by the CharArrayStreambuf instance.
  char* const _buf;
  const std::size_t _bufSize;
};

// Read-only version of CharArrayStreambuf
class ROCharArrayStreambuf: public CharArrayStreambuf
{
public:
  explicit ROCharArrayStreambuf(const char* buf, std::size_t bufSize);

  // Accessor for the buffer start pointer (same method name as for
  // std::string)
  const char* data() const;

private:
  // Override methods pertaining to write access
  virtual int overflow(int c = std::char_traits<char>::eof()) override;
  virtual std::streamsize xsputn(const char* s, std::streamsize n) override;
};

// Convenience class: std::istream subclass based on ROCharArrayStreambuf
class CharArrayIStream: public std::istream
{
public:
  // Same parameters as for ROCharArrayStreambuf
  explicit CharArrayIStream(const char* buf, std::size_t bufSize);

  // Accessors for the underlying buffer start pointer and size
  const char* data() const;
  std::size_t size() const;

private:
  ROCharArrayStreambuf _streamBuf;
};

// Convenience class: std::ostream subclass based on CharArrayStreambuf
class CharArrayOStream: public std::ostream
{
public:
  // Same parameters as for CharArrayStreambuf
  explicit CharArrayOStream(char* buf, std::size_t bufSize);

  // Accessors for the underlying buffer start pointer and size
  char* data() const;
  std::size_t size() const;

private:
  CharArrayStreambuf _streamBuf;
};

// Convenience class: std::iostream subclass based on CharArrayStreambuf
class CharArrayIOStream: public std::iostream
{
public:
  // Same parameters as for CharArrayStreambuf
  explicit CharArrayIOStream(char* buf, std::size_t bufSize);

  // Accessors for the underlying buffer start pointer and size
  char* data() const;
  std::size_t size() const;

private:
  CharArrayStreambuf _streamBuf;
};

} // of namespace simgear

#endif  // of SG_CHAR_ARRAY_STREAM_HXX
