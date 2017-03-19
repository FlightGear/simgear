// -*- coding: utf-8 -*-
//
// EmbeddedResource.cxx ---  Class for pointing to/accessing an embedded resource
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

#include <string>
#include <iosfwd>
#include <ios>                  // std::streamsize
#include <ostream>
#include <memory>               // std::unique_ptr
#include <utility>              // std::move()
#include <algorithm>            // std::min()
#include <limits>               // std::numeric_limits
#include <cstddef>              // std::size_t, std::ptrdiff_t

#include <simgear/structure/exception.hxx>
#include <simgear/io/iostreams/CharArrayStream.hxx>
#include <simgear/io/iostreams/zlibstream.hxx>
#include "EmbeddedResource.hxx"

using std::string;
using std::unique_ptr;

// Inspired by <http://stackoverflow.com/a/21174979/4756009>
template<typename Derived, typename Base>
static unique_ptr<Derived> static_unique_ptr_cast(unique_ptr<Base> p)
{
  auto d = static_cast<Derived *>(p.release());
  return unique_ptr<Derived>(d);
}


namespace simgear
{

// ***************************************************************************
// *                     AbstractEmbeddedResource class                      *
// ***************************************************************************

AbstractEmbeddedResource::AbstractEmbeddedResource(const char *data,
                                                   std::size_t size)
  : _data(data),
    _size(size)
{ }

const char *AbstractEmbeddedResource::rawPtr() const
{
  return _data;
}

std::size_t AbstractEmbeddedResource::rawSize() const
{
  return _size;
}

string AbstractEmbeddedResource::str() const
{
  if (_size > std::numeric_limits<string::size_type>::max()) {
    throw sg_range_exception(
      "Resource too large to fit in an std::string (size: " +
      std::to_string(_size) + " bytes)");
  }

  return string(_data, _size);
}

// ***************************************************************************
// *                        RawEmbeddedResource class                        *
// ***************************************************************************

RawEmbeddedResource::RawEmbeddedResource(const char *data, std::size_t size)
  : AbstractEmbeddedResource(data, size)
{ }

AbstractEmbeddedResource::CompressionType
RawEmbeddedResource::compressionType() const
{
  return AbstractEmbeddedResource::CompressionType::NONE;
}

string RawEmbeddedResource::compressionDescr() const
{
  return string("none");
}

unique_ptr<std::streambuf> RawEmbeddedResource::streambuf() const
{
  // This is a read-only variant of CharArrayStreambuf
  return unique_ptr<std::streambuf>(
    new ROCharArrayStreambuf(rawPtr(), rawSize()));
}

unique_ptr<std::istream> RawEmbeddedResource::istream() const
{
  return unique_ptr<std::istream>(new CharArrayIStream(rawPtr(), rawSize()));
}

// ***************************************************************************
// *                       ZlibEmbeddedResource class                        *
// ***************************************************************************

ZlibEmbeddedResource::ZlibEmbeddedResource(const char *data,
                                           std::size_t compressedSize,
                                           std::size_t uncompressedSize)
  : AbstractEmbeddedResource(data, compressedSize),
    _uncompressedSize(uncompressedSize),
    _inBuf(nullptr),
    _inBufSize(262144),         // adjusted below in the constructor body
    _outBuf(nullptr),
    _outBufSize(262144),
    _putbackSize(0)             // default for best performance
{
  static_assert(262144 <= std::numeric_limits<std::size_t>::max(),
                "The std::size_t type is unexpectedly small.");
  // No need to use an input buffer (where compressed data chunks are put for
  // zlib to read and decompress) larger than the whole compressed resource!
  _inBufSize = std::min(rawSize(), _inBufSize);
}

AbstractEmbeddedResource::CompressionType
ZlibEmbeddedResource::compressionType() const
{ return AbstractEmbeddedResource::CompressionType::ZLIB; }

string ZlibEmbeddedResource::compressionDescr() const
{ return string("zlib"); }

std::size_t ZlibEmbeddedResource::uncompressedSize() const
{ return _uncompressedSize; }

char* ZlibEmbeddedResource::getInputBufferStart()
{ return _inBuf; }

void ZlibEmbeddedResource::setInputBufferStart(char* inBuf)
{ _inBuf = inBuf; }

std::size_t ZlibEmbeddedResource::getInputBufferSize()
{ return _inBufSize; }

void ZlibEmbeddedResource::setInputBufferSize(std::size_t size)
{ _inBufSize = size; }

char* ZlibEmbeddedResource::getOutputBufferStart()
{ return _outBuf; }

void ZlibEmbeddedResource::setOutputBufferStart(char* outBuf)
{ _outBuf = outBuf; }

std::size_t ZlibEmbeddedResource::getOutputBufferSize()
{ return _outBufSize; }

void ZlibEmbeddedResource::setOutputBufferSize(std::size_t size)
{ _outBufSize = size; }

std::size_t ZlibEmbeddedResource::getPutbackSize()
{ return _putbackSize; }

void ZlibEmbeddedResource::setPutbackSize(std::size_t size)
{ _putbackSize = size; }

unique_ptr<std::streambuf> ZlibEmbeddedResource::streambuf() const
{
  unique_ptr<CharArrayIStream> rawReaderIStream(
    new CharArrayIStream(rawPtr(), rawSize()));

  return unique_ptr<std::streambuf>(
    new ZlibDecompressorIStreambuf(
      std::move(rawReaderIStream),
      SGPath(),                   // rawReaderIStream isn't bound to a file
      ZLibCompressionFormat::ZLIB,
      _inBuf, _inBufSize, _outBuf, _outBufSize, _putbackSize));
}

unique_ptr<std::istream> ZlibEmbeddedResource::istream() const
{
  unique_ptr<CharArrayIStream> rawReaderIStream(
    new CharArrayIStream(rawPtr(), rawSize()));

  return unique_ptr<std::istream>(
    new ZlibDecompressorIStream(
      std::move(rawReaderIStream),
      SGPath(),                   // rawReaderIStream isn't bound to a file
      ZLibCompressionFormat::ZLIB,
      _inBuf, _inBufSize, _outBuf, _outBufSize, _putbackSize));
}

std::string ZlibEmbeddedResource::str() const
{
  static constexpr std::size_t bufSize = 65536;
  static_assert(bufSize <= std::numeric_limits<std::streamsize>::max(),
                "Type std::streamsize is unexpectedly small");
  static_assert(bufSize <= std::numeric_limits<string::size_type>::max(),
                "Type std::string::size_type is unexpectedly small");
  unique_ptr<char[]> buf(new char[bufSize]);

  auto decompressor =
    static_unique_ptr_cast<ZlibDecompressorIStream>(istream());
  std::streamsize nbCharsRead;
  string result;

  if (_uncompressedSize > std::numeric_limits<string::size_type>::max()) {
    throw sg_range_exception(
      "Resource too large to fit in an std::string (uncompressed size: "
      + std::to_string(_uncompressedSize) + " bytes)");
  } else {
    result.reserve(static_cast<string::size_type>(_uncompressedSize));
  }

  do {
    decompressor->read(buf.get(), bufSize);
    nbCharsRead = decompressor->gcount();

    if (nbCharsRead > 0) {
      result.append(buf.get(), nbCharsRead);
    }
  } while (*decompressor);

  // decompressor->fail() would *not* indicate an error, due to the semantics
  // of std::istream::read().
  if (decompressor->bad()) {
    throw sg_io_exception("Error while extracting a compressed resource");
  }

  return result;
}

// ***************************************************************************
// *                       Stream insertion operators                        *
// ***************************************************************************
std::ostream& operator<<(std::ostream& os,
                         const RawEmbeddedResource& resource)
{  // This won't escape double quotes, backslashes, etc. in resource.str().
  return os << "RawEmbeddedResource:\n"
    "  compressionType = \"" << resource.compressionDescr() << "\"\n"
    "  rawPtr = " << (void*) resource.rawPtr() << "\n"
    "  rawSize = " << resource.rawSize();
}

std::ostream& operator<<(std::ostream& os,
                         const ZlibEmbeddedResource& resource)
{  // This won't escape double quotes, backslashes, etc. in resource.str().
  return os << "ZlibEmbeddedResource:\n"
    "  compressionType = \"" << resource.compressionDescr() << "\"\n"
    "  rawPtr = " << (void*) resource.rawPtr() << "\n"
    "  rawSize = " << resource.rawSize() << "\n"
    "  uncompressedSize = " << resource.uncompressedSize();
}

} // of namespace simgear
