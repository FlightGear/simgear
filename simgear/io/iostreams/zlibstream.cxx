// -*- coding: utf-8 -*-
//
// zlibstream.cxx --- IOStreams classes for working with RFC 1950 and RFC 1952
//                    compression formats (respectively known as the zlib and
//                    gzip formats)
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

#include <string>
#include <ios>                  // std::streamsize
#include <istream>
#include <memory>               // std::unique_ptr
#include <utility>              // std::move()
#include <algorithm>
#include <stdexcept>
#include <unordered_map>
#include <limits>               // std::numeric_limits
#include <type_traits>          // std::make_unsigned(), std::underlying_type
#include <cstddef>              // std::size_t, std::ptrdiff_t
#include <cassert>

#include <zlib.h>

#include <simgear/debug/logstream.hxx>
#include <simgear/io/iostreams/zlibstream.hxx>
#include <simgear/misc/strutils.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/sg_inlines.h>
#include <simgear/structure/exception.hxx>

using std::string;
using simgear::enumValue;

using traits = std::char_traits<char>;

// Private utility function
static string zlibErrorMessage(const z_stream& zstream, int errorCode)
{
  string res;
  std::unordered_map<int, string> errorCodeToMessageMap = {
    {Z_OK,            "zlib: no error (code Z_OK)"},
    {Z_STREAM_END,    "zlib stream end"},
    {Z_NEED_DICT,     "zlib: Z_NEED_DICT"},
    {Z_STREAM_ERROR,  "zlib stream error"},
    {Z_DATA_ERROR,    "zlib data error"},
    {Z_MEM_ERROR,     "zlib memory error"},
    {Z_BUF_ERROR,     "zlib buffer error"},
    {Z_VERSION_ERROR, "zlib version error"}
  };

  if (errorCode == Z_ERRNO) {
    res = simgear::strutils::error_string(errno);
  } else if (zstream.msg != nullptr) {
    // Caution: this only works if the zstream structure hasn't been
    // deallocated!
    res = "zlib: " + string(zstream.msg);
  } else {
    try {
      res = errorCodeToMessageMap.at(errorCode);
    } catch (const std::out_of_range&) {
      res = string("unknown zlib error (code " + std::to_string(errorCode) +
                   ")");
    }
  }

  return res;
}

// Requirement: 'val' must be non-negative.
//
// Return:
//   - 'val' cast as BoundType if it's lower than the largest value BoundType
//     can represent;
//   - this largest value otherwise.
template<class BoundType, class T>
static BoundType clipCast(T val)
{
  typedef typename std::make_unsigned<T>::type uT;
  typedef typename std::make_unsigned<BoundType>::type uBoundType;
  assert(val >= 0); // otherwise, the comparison and cast to uT would be unsafe

  // Casts to avoid the signed-compare warning; they don't affect the values,
  // since both are non-negative.
  if (static_cast<uT>(val) <
      static_cast<uBoundType>( std::numeric_limits<BoundType>::max() )) {
    return static_cast<BoundType>(val);
  } else {
    return std::numeric_limits<BoundType>::max();
  }
}

// Requirement: 'size' must be non-negative.
//
// Return:
//   - 'size' if it is lower than or equal to std::numeric_limits<uInt>::max();
//   - std::numeric_limits<uInt>::max() cast as a T otherwise (this is always
//     possible in a lossless way, since in this case, one has
//     0 <= std::numeric_limits<uInt>::max() < size, and 'size' is of type T).
//
// Note: uInt is the type of z_stream.avail_in and z_stream.avail_out, hence
//       the function name.
template<class T>
static T zlibChunk(T size)
{
  typedef typename std::make_unsigned<T>::type uT;
  assert(size >= 0); // otherwise, the comparison and cast to uT would be unsafe

  if (static_cast<uT>(size) <= std::numeric_limits<uInt>::max()) {
    return size;
  } else {
    // In this case, we are sure that T can represent
    // std::numeric_limits<uInt>::max(), thus the cast is safe.
    return static_cast<T>(std::numeric_limits<uInt>::max());
  }
}

namespace simgear
{

// ***************************************************************************
// *                      ZlibAbstractIStreambuf class                       *
// ***************************************************************************

// Common initialization. Subclasses must complete the z_stream struct
// initialization with a call to deflateInit2() or inflateInit2(), typically.
ZlibAbstractIStreambuf::ZlibAbstractIStreambuf(std::istream& iStream,
                                               const SGPath& path,
                                               char* inBuf,
                                               std::size_t inBufSize,
                                               char *outBuf,
                                               std::size_t outBufSize,
                                               std::size_t putbackSize)
  : _iStream(iStream),
    _iStream_p(nullptr),
    _path(path),
    _inBuf(inBuf),
    _inBufSize(inBufSize),
    _outBuf(outBuf),
    _outBufSize(outBufSize),
    _putbackSize(putbackSize)
{
  assert(_inBufSize > 0);
  assert(_putbackSize < _outBufSize);

  if (_inBuf == nullptr) {
    _inBuf = new char[_inBufSize];
    _inBufMustBeFreed = true;
  }

  if (_outBuf == nullptr) {
    _outBuf = new char[_outBufSize];
    _outBufMustBeFreed = true;
  }

  _inBufEndPtr = _inBuf;
  // The input buffer is empty.
  _zstream.next_in = reinterpret_cast<unsigned char *>(_inBuf);
  _zstream.avail_in = 0;
  // ZLib's documentation says its init functions such as inflateInit2() might
  // consume stream input, therefore let's fill the input buffer now. This
  // way, constructors of derived classes just have to call the appropriate
  // ZLib init function: the data will already be in place.
  getInputData();

  // Force underflow() of the stream buffer on the first read. We could use
  // some other value, but I avoid nullptr in order to be sure we can always
  // reliably compare the three pointers with < and >, as well as compute the
  // difference between any two of them.
  setg(_outBuf, _outBuf, _outBuf);
}

ZlibAbstractIStreambuf::ZlibAbstractIStreambuf(
  std::unique_ptr<std::istream> iStream_p,
  const SGPath& path,
  char* inBuf,
  std::size_t inBufSize,
  char *outBuf,
  std::size_t outBufSize,
  std::size_t putbackSize)
  : ZlibAbstractIStreambuf(*iStream_p, path, inBuf, inBufSize,
                           outBuf, outBufSize, putbackSize)
{
  // Take ownership of the object. This is a way to ensure that the _iStream
  // reference stays valid as long as our instance is alive, and that the
  // corresponding std::istream object is automatically destroyed as soon as
  // our instance is itself destroyed.
  _iStream_p = std::move(iStream_p);
}

ZlibAbstractIStreambuf::~ZlibAbstractIStreambuf()
{
  if (_inBufMustBeFreed) {
    delete[] _inBuf;
  }

  if (_outBufMustBeFreed) {
    delete[] _outBuf;
  }
}

// Fill or refill the output buffer, and update the three pointers
// corresponding to eback(), gptr() and egptr().
int ZlibAbstractIStreambuf::underflow()
{
  if (_allFinished) {
    return traits::eof();
  }

  // According to the C++11 standard: “The public members of basic_streambuf
  // call this virtual function only if gptr() is null or gptr() >= egptr()”.
  // Still, it seems some people do the following or similar (maybe in case
  // underflow() is called “incorrectly”?). See for instance N. Josuttis, The
  // C++ Standard Library (1st edition), p. 584. One sure thing is that it
  // can't hurt (except performance, marginally), so let's do it.
  if (gptr() < egptr()) {
    return traits::to_int_type(*gptr());
  }

  assert(gptr() == egptr());
  assert(egptr() - eback() >= 0);
  std::size_t nbPutbackChars = std::min(
    static_cast<std::size_t>(egptr() - eback()), // OK because egptr() >= eback()
    _putbackSize);
  std::copy(egptr() - nbPutbackChars, egptr(),
            _outBuf + _putbackSize - nbPutbackChars);

  setg(_outBuf + _putbackSize - nbPutbackChars, // start of putback area
       _outBuf + _putbackSize,                  // start of obtained data
       fillOutputBuffer());                     // one-past-end of obtained data

  return (gptr() == egptr()) ? traits::eof() : traits::to_int_type(*gptr());
}

std::streampos ZlibAbstractIStreambuf::seekoff(std::streamoff off,
                                               std::ios_base::seekdir way,
                                               std::ios_base::openmode which)
{
    if (way != std::ios_base::cur || off < 0)    return -1;
    for(;;) {
        if (!off) break;
        char*   g = gptr();
        char*   eg = egptr();
        if (eg > g) {
            size_t  delta = (off < eg - g) ? off : eg - g;
            setg(g + delta, g + delta, eg);
            off -= delta;
        }
        else {
            if (underflow() == EOF) return -1;
        }
    }
    return 0;
}


// Simple utility method for fillOutputBuffer(), used to improve readability.
// Return the remaining space available in the output buffer, where zlib can
// write.
std::size_t
ZlibAbstractIStreambuf::fOB_remainingSpace(unsigned char* nextOutPtr) const
{
  std::ptrdiff_t remainingSpaceInOutBuf = (
    _outBuf + _outBufSize - reinterpret_cast<char*>(nextOutPtr));
  assert(remainingSpaceInOutBuf >= 0);

  return static_cast<std::size_t>(remainingSpaceInOutBuf);
}

// Simple method for dealing with the Z_BUF_ERROR code that may be returned
// by zlib's deflate() and inflate() methods.
[[ noreturn ]] void ZlibAbstractIStreambuf::handleZ_BUF_ERROR() const
{
  switch (operationType()) {
  case OperationType::DECOMPRESSION:
  {
    string message = (_path.isNull()) ?
      "Got Z_BUF_ERROR from zlib while decompressing a stream. The stream "
      "was probably incomplete."
      :
      "Got Z_BUF_ERROR from zlib during decompression. The compressed stream "
      "was probably incomplete.";
    // When _path.isNull(), sg_location(_path) is equivalent to sg_location()
    throw sg_io_exception(message, sg_location(_path));
  }
  case OperationType::COMPRESSION:
    throw std::logic_error(
      "Called ZlibAbstractIStreambuf::handleZ_BUF_ERROR() with "
      "operationType() == ZlibAbstractIStreambuf::OperationType::COMPRESSION");
  default:
    throw std::logic_error(
      "Unexpected operationType() in "
      "ZlibAbstractIStreambuf::handleZ_BUF_ERROR(): " +
      std::to_string(enumValue(operationType())));
  }
}

// Fill or refill the output buffer. Return a pointer to the first unused char
// in _outBuf (i.e., right after the data written by this method, if any;
// otherwise: _outBuf + _putbackSize).
char* ZlibAbstractIStreambuf::fillOutputBuffer()
{
  bool allInputRead = false;
  std::size_t remainingSpaceInOutBuf;
  int retCode;

  // We have to do these unpleasant casts, because zlib uses pointers to
  // unsigned char for its input and output buffers, while the IOStreams
  // library in the C++ standard uses plain char pointers...
  _zstream.next_out = reinterpret_cast<unsigned char*>(_outBuf + _putbackSize);
  remainingSpaceInOutBuf = fOB_remainingSpace(_zstream.next_out);

  while (remainingSpaceInOutBuf > 0) {
    _zstream.avail_out = clipCast<uInt>(remainingSpaceInOutBuf);

    if (_zstream.avail_in == 0 && !allInputRead) {
      // Get data from _iStream, store it in _inBuf
      allInputRead = getInputData();
    }

    // Make zlib process some data (compress or decompress it). This updates
    // _zstream.{avail,next}_{in,out} (4 fields of the z_stream struct).
    retCode = zlibProcessData();

    if (retCode == Z_BUF_ERROR) {
      handleZ_BUF_ERROR();            // doesn't return
    } else if (retCode == Z_STREAM_END) {
      //assert(_zstream.avail_in == 0); // all of _inBuf must have been used
      _allFinished = true;
      break;
    } else if (retCode < 0) {         // negative codes are errors
      throw sg_io_exception(::zlibErrorMessage(_zstream, retCode),
                            sg_location(_path));
    }

    remainingSpaceInOutBuf = fOB_remainingSpace(_zstream.next_out);
  }

  return reinterpret_cast<char*>(_zstream.next_out);
}

// This method provides input data to zlib:
//   - if data is already available in _inBuf, it updates _zstream.avail_in
//     accordingly (using the largest possible amount given _zstream.avail_in's
//     type, i.e., uInt);
//   - otherwise, it reads() as much data as possible into _inBuf from
//     _iStream and updates _zstream.avail_in to tell zlib about this new
//     available data (again: largest possible amount given the uInt type).
//
// This method must be called only when _zstream.avail_in == 0, which means
// zlib has read everything we fed it (and does *not* mean, by the way, that
// the input buffer starting at _inBuf is empty; this is because the buffer
// size might exceed what can be represented by an uInt).
//
// On return:
//   - either EOF has been reached for _iStream, and the return value is true;
//   - or _zstream.avail_in > 0, and the return value is false.
//
// Note: don't read more in the previous paragraph concerning the state on
//       return. In the first case, there is *no guarantee* about
//       _zstream.avail_in's value; in the second case, there is *no guarantee*
//       about whether EOF has been reached for _iStream.
bool ZlibAbstractIStreambuf::getInputData()
{
  bool allInputRead = false;

  assert(_zstream.avail_in == 0);
  std::ptrdiff_t alreadyAvailable =
    _inBufEndPtr - reinterpret_cast<char*>(_zstream.next_in);
  assert(alreadyAvailable >= 0);

  // Data already available?
  if (alreadyAvailable > 0) {
    _zstream.avail_in = clipCast<uInt>(alreadyAvailable);
    return allInputRead;
  }

  if (_inBufEndPtr == _inBuf + _inBufSize) { // buffer full, rewind
    _inBufEndPtr = _inBuf;
    _zstream.next_in = reinterpret_cast<unsigned char*>(_inBuf);
  }

  // Fill the input buffer (as much as possible)
  while (_inBufEndPtr < _inBuf + _inBufSize && !_iStream.eof()) {
    std::streamsize nbCharsToRead = clipCast<std::streamsize>(
      _inBuf + _inBufSize - _inBufEndPtr);
    _iStream.read(_inBufEndPtr, nbCharsToRead);

    if (_iStream.bad()) {
      string errMsg = simgear::strutils::error_string(errno);
      string msgStart = (_path.isNull()) ?
        "Error while reading from a stream" : "Read error";
      throw sg_io_exception(msgStart + ": " + errMsg, sg_location(_path));
    }

    // Could be zero if at EOF
    std::streamsize nbCharsRead = _iStream.gcount();
    // std::streamsize is a signed integral type!
    assert(0 <= nbCharsRead);
    _inBufEndPtr += nbCharsRead;
  }

  std::ptrdiff_t availableChars =
    _inBufEndPtr - reinterpret_cast<char*>(_zstream.next_in);
  // assert(availableChars >= 0);    <-- already done in clipCast<uInt>()
  _zstream.avail_in = clipCast<uInt>(availableChars);

  if (_iStream.eof()) {
    allInputRead = true;
  } else {
    // Trying to rewind a fully read std::istringstream with seekg() can lead
    // to a weird state, where the stream doesn't return any character but
    // doesn't report EOF either. Make sure we are not in this situation.
    assert(_zstream.avail_in > 0);
  }

  return allInputRead;
}

// Implementing this method is optional, but should provide better
// performance. It makes zlib write the data directly in the buffer starting
// at 'dest'. Without it, the data would go through an intermediate buffer
// (_outBuf) before being copied to its (hopefully) final destination.
std::streamsize ZlibAbstractIStreambuf::xsgetn(char* dest, std::streamsize n)
{
  // Despite the somewhat misleading footnote 296 of §27.5.3 of the C++11
  // standard, one can't assume std::size_t to be at least as large as
  // std::streamsize (64 bits std::streamsize in a 32 bits Windows program).
  std::streamsize remaining = n;
  char* origGptr = gptr();
  char* writePtr = dest;        // we'll need dest later -> work with a copy

  // First, let's take data present in our internal buffer (_outBuf)
  while (remaining > 0) {
    // Number of available chars in _outBuf
    std::ptrdiff_t avail = egptr() - gptr();
    if (avail == 0) {           // our internal buffer is empty
      break;
    }

    // We need an int for gbump(), at least in C++11.
    int chunkSize_i = clipCast<int>(avail);
    if (chunkSize_i > remaining) {
      chunkSize_i = static_cast<int>(remaining);
    }
    assert(chunkSize_i >= 0);

    std::copy(gptr(), gptr() + chunkSize_i, writePtr);
    gbump(chunkSize_i);
    writePtr += chunkSize_i;
    // This cast is okay because 0 <= chunkSize_i <= remaining, which is an
    // std::streamsize
    remaining -= static_cast<std::streamsize>(chunkSize_i);
  }

  if (remaining == 0) {
    // Everything we needed was already in _outBuf. The putback area is set up
    // as it should between eback() and the current gptr(), so we are fine to
    // return.
    return n;
  }

  // Now, let's make it so that the remaining data we need is directly written
  // to the destination area, without going through _outBuf.
  _zstream.next_out = reinterpret_cast<unsigned char*>(writePtr);
  bool allInputRead = false;
  int retCode;

  while (remaining > 0) {
    std::streamsize chunkSize_s = zlibChunk(remaining);
    // chunkSize_s > 0 and does fit in a zlib uInt: that's the whole point of
    // zlibChunk.
    _zstream.avail_out = static_cast<uInt>(chunkSize_s);

    if (_zstream.avail_in == 0 && !allInputRead) {
      allInputRead = getInputData();
    }

    // Make zlib process some data (compress or decompress). This updates
    // _zstream.{avail,next}_{in,out} (4 fields of the z_stream struct).
    retCode = zlibProcessData();
    // chunkSize_s - _zstream.avail_out is the number of chars written by zlib.
    // 0 <= _zstream.avail_out <= chunkSize_s, which is an std::streamsize.
    remaining -= chunkSize_s - static_cast<std::streamsize>(_zstream.avail_out);

    if (retCode == Z_BUF_ERROR) {
      handleZ_BUF_ERROR();            // doesn't return
    } else if (retCode == Z_STREAM_END) {
      //assert(_zstream.avail_in == 0); // all of _inBuf must have been used
      _allFinished = true;
      break;
    } else if (retCode < 0) {   // negative codes are errors
      throw sg_io_exception(::zlibErrorMessage(_zstream, retCode),
                            sg_location(_path));
    }
  }

  // Finally, copy chars to the putback area.
  std::size_t nbPutbackChars = xsgetn_preparePutbackArea(
    origGptr, dest, reinterpret_cast<char*>(_zstream.next_out));
  setg(_outBuf + _putbackSize - nbPutbackChars, // start of putback area
       _outBuf + _putbackSize,                  // the buffer for pending,
       _outBuf + _putbackSize);                 // available data is empty

  assert(remaining >= 0);
  assert(n - remaining >= 0);
  // Total number of chars copied.
  return n - remaining;
}

// Utility method for xsgetn(): copy some chars to the putback area
std::size_t ZlibAbstractIStreambuf::xsgetn_preparePutbackArea(
  char* origGptr, char* dest, char* writePtr)
{
  // There are two buffers containing characters we potentially have to copy
  // to the putback area: the one starting at _outBuf and the one starting at
  // dest. In the following diagram, ***** represents those from _outBuf,
  // which are right-aligned at origGptr[1], and the series of # represents
  // those in the [dest, writePtr) address range:
  //
  // |_outBuf  |eback() *****|origGptr      |_outBuf + _outBufSize
  //
  // |dest #################|writePtr
  //
  // Together, these two memory blocks logically form a contiguous stream of
  // chars we gave to the “client”: *****#################. All we have to do
  // now is copy the appropriate amount of chars from this logical stream to
  // the putback area, according to _putbackSize. These chars are the last
  // ones in said stream (i.e., right portion of *****#################), but
  // we have to copy them in order of increasing address to avoid possible
  // overlapping problems in _outBuf. This is because some of the chars to
  // copy may be located before _outBuf + _putbackSize (i.e., already be in
  // the putback area).
  //
  //   [1] This means that the last char represented by a star is at address
  //       origGptr-1.

  // It seems std::ptrdiff_t is the signed counterpart of std::size_t,
  // therefore this should always hold (even with equality).
  static_assert(sizeof(std::size_t) >= sizeof(std::ptrdiff_t),
                "Unexpected: sizeof(std::size_t) < sizeof(std::ptrdiff_t)");
  assert(writePtr - dest >= 0);
  std::size_t inDestBuffer = static_cast<std::size_t>(writePtr - dest);

  assert(origGptr - eback() >= 0);
  std::size_t nbPutbackChars = std::min(
    static_cast<std::size_t>(origGptr - eback()) + inDestBuffer,
    _putbackSize);
  std::size_t nbPutbackCharsToGo = nbPutbackChars;

  // Are there chars in _outBuf that need to be copied to the putback area?
  if (nbPutbackChars > inDestBuffer) {
    std::size_t chunkSize = nbPutbackChars - inDestBuffer; // yes, this number
    std::copy(origGptr - chunkSize, origGptr,
              _outBuf + _putbackSize - nbPutbackChars);
    nbPutbackCharsToGo -= chunkSize;
  }

  // Finally, copy those that are not in _outBuf
  std::copy(writePtr - nbPutbackCharsToGo, writePtr,
            _outBuf + _putbackSize - nbPutbackCharsToGo);

  return nbPutbackChars;
}

// ***************************************************************************
// *                     ZlibCompressorIStreambuf class                      *
// ***************************************************************************

ZlibCompressorIStreambuf::ZlibCompressorIStreambuf(
  std::istream& iStream,
  const SGPath& path,
  int compressionLevel,
  ZLibCompressionFormat format,
  ZLibMemoryStrategy memStrategy,
  char* inBuf,
  std::size_t inBufSize,
  char *outBuf,
  std::size_t outBufSize,
  std::size_t putbackSize)
  : ZlibAbstractIStreambuf(iStream, path, inBuf, inBufSize, outBuf, outBufSize,
                           putbackSize)
{
  zStreamInit(compressionLevel, format, memStrategy);
}

ZlibCompressorIStreambuf::ZlibCompressorIStreambuf(
  std::unique_ptr<std::istream> iStream_p,
  const SGPath& path,
  int compressionLevel,
  ZLibCompressionFormat format,
  ZLibMemoryStrategy memStrategy,
  char* inBuf,
  std::size_t inBufSize,
  char *outBuf,
  std::size_t outBufSize,
  std::size_t putbackSize)
  : ZlibCompressorIStreambuf(*iStream_p, path, compressionLevel, format,
                             memStrategy, inBuf, inBufSize, outBuf, outBufSize,
                             putbackSize)
{
  _iStream_p = std::move(iStream_p); // take ownership of the object
}

ZlibCompressorIStreambuf::~ZlibCompressorIStreambuf()
{
  int retCode = deflateEnd(&_zstream); // deallocate the z_stream struct
  if (retCode != Z_OK) {
    // In C++11, we can't throw exceptions from a destructor.
    SG_LOG(SG_IO, SG_ALERT, "ZlibCompressorIStreambuf: " <<
           ::zlibErrorMessage(_zstream, retCode));
  }
}

ZlibAbstractIStreambuf::OperationType
ZlibCompressorIStreambuf::operationType() const
{
  return OperationType::COMPRESSION;
}

void ZlibCompressorIStreambuf::zStreamInit(int compressionLevel,
                                          ZLibCompressionFormat format,
                                          ZLibMemoryStrategy memStrategy)
{
  int windowBits, memLevel;

  // Intentionally not listing ZLibCompressionFormat::AUTODETECT here (it is
  // only for decompression!)
  switch (format) {
  case ZLibCompressionFormat::ZLIB:
    windowBits = 15;
    break;
  case ZLibCompressionFormat::GZIP:
    windowBits = 31;
    break;
  default:
    throw std::logic_error("Unexpected compression format: " +
                           std::to_string(enumValue(format)));
  }

  switch (memStrategy) {
  case ZLibMemoryStrategy::FAVOR_MEMORY_OVER_SPEED:
    memLevel = 8;
    break;
  case ZLibMemoryStrategy::FAVOR_SPEED_OVER_MEMORY:
    memLevel = 9;
    break;
  default:
    throw std::logic_error("Unexpected memory strategy: " +
                           std::to_string(enumValue(memStrategy)));
  }

  _zstream.zalloc = Z_NULL;     // No custom memory allocation routines
  _zstream.zfree = Z_NULL;      // Ditto. Therefore, the 'opaque' field won't
  _zstream.opaque = Z_NULL;     // be used, actually.

  int retCode = deflateInit2(&_zstream, compressionLevel, Z_DEFLATED,
                             windowBits, memLevel, Z_DEFAULT_STRATEGY);
  if (retCode != Z_OK) {
    throw sg_io_exception(::zlibErrorMessage(_zstream, retCode),
                          sg_location(_path));
  }
}

int ZlibCompressorIStreambuf::zlibProcessData()
{
  // Compress as much data as possible given _zstream.avail_in and
  // _zstream.avail_out. The input data starts at _zstream.next_in, the output
  // at _zstream.next_out, and these four fields are all updated by this call
  // to reflect exactly the amount of data zlib has consumed and produced.
  return deflate(&_zstream, _zstream.avail_in ? Z_NO_FLUSH : Z_FINISH);
}

// ***************************************************************************
// *                    ZlibDecompressorIStreambuf class                     *
// ***************************************************************************

ZlibDecompressorIStreambuf::ZlibDecompressorIStreambuf(
  std::istream& iStream,
  const SGPath& path,
  ZLibCompressionFormat format,
  char* inBuf,
  std::size_t inBufSize,
  char *outBuf,
  std::size_t outBufSize,
  std::size_t putbackSize)
  : ZlibAbstractIStreambuf(iStream, path, inBuf, inBufSize, outBuf, outBufSize,
                           putbackSize)
{
  zStreamInit(format);
}

ZlibDecompressorIStreambuf::ZlibDecompressorIStreambuf(
  std::unique_ptr<std::istream> iStream_p,
  const SGPath& path,
  ZLibCompressionFormat format,
  char* inBuf,
  std::size_t inBufSize,
  char *outBuf,
  std::size_t outBufSize,
  std::size_t putbackSize)
  : ZlibDecompressorIStreambuf(*iStream_p, path, format, inBuf, inBufSize,
                               outBuf, outBufSize, putbackSize)
{
  _iStream_p = std::move(iStream_p); // take ownership of the object
}

ZlibDecompressorIStreambuf::~ZlibDecompressorIStreambuf()
{
  int retCode = inflateEnd(&_zstream); // deallocate the z_stream struct
  if (retCode != Z_OK) {
    // In C++11, we can't throw exceptions from a destructor.
    SG_LOG(SG_IO, SG_ALERT, "ZlibDecompressorIStreambuf: " <<
           ::zlibErrorMessage(_zstream, retCode));
  }
}

ZlibAbstractIStreambuf::OperationType
ZlibDecompressorIStreambuf::operationType() const
{
  return OperationType::DECOMPRESSION;
}

void ZlibDecompressorIStreambuf::zStreamInit(ZLibCompressionFormat format)
{
  int windowBits;

  switch (format) {
  case ZLibCompressionFormat::ZLIB:
    windowBits = 15;
    break;
  case ZLibCompressionFormat::GZIP:
    windowBits = 31;
    break;
  case ZLibCompressionFormat::AUTODETECT:
    windowBits = 47;            // 47 = 32 + 15
    break;
  case ZLibCompressionFormat::ZLIB_RAW:
    windowBits = -15;
    break;
  default:
    throw std::logic_error("Unexpected compression format: " +
                           std::to_string(enumValue(format)));
  }

  _zstream.zalloc = Z_NULL;     // No custom memory allocation routines
  _zstream.zfree = Z_NULL;      // Ditto. Therefore, the 'opaque' field won't
  _zstream.opaque = Z_NULL;     // be used, actually.

  int retCode = inflateInit2(&_zstream, windowBits);
  if (retCode != Z_OK) {
    throw sg_io_exception(::zlibErrorMessage(_zstream, retCode),
                          sg_location(_path));
  }
}

int ZlibDecompressorIStreambuf::zlibProcessData()
{
  // Decompress as much data as possible given _zstream.avail_in and
  // _zstream.avail_out. The input data starts at _zstream.next_in, the output
  // at _zstream.next_out, and these four fields are all updated by this call
  // to reflect exactly the amount of data zlib has consumed and produced.
  return inflate(&_zstream, _zstream.avail_in ? Z_NO_FLUSH : Z_FINISH);
}

// ***************************************************************************
// *                       ZlibCompressorIStream class                       *
// ***************************************************************************

ZlibCompressorIStream::ZlibCompressorIStream(std::istream& iStream,
                                             const SGPath& path,
                                             int compressionLevel,
                                             ZLibCompressionFormat format,
                                             ZLibMemoryStrategy memStrategy,
                                             char* inBuf,
                                             std::size_t inBufSize,
                                             char *outBuf,
                                             std::size_t outBufSize,
                                             std::size_t putbackSize)
  : std::istream(nullptr),
    _streamBuf(iStream, path, compressionLevel, format, memStrategy, inBuf,
               inBufSize, outBuf, outBufSize, putbackSize)
{
  // Associate _streamBuf to 'this' and clear the error state flags
  rdbuf(&_streamBuf);
}

ZlibCompressorIStream::ZlibCompressorIStream(
  std::unique_ptr<std::istream> iStream_p,
  const SGPath& path,
  int compressionLevel,
  ZLibCompressionFormat format,
  ZLibMemoryStrategy memStrategy,
  char* inBuf,
  std::size_t inBufSize,
  char *outBuf,
  std::size_t outBufSize,
  std::size_t putbackSize)
  : std::istream(nullptr),
    _streamBuf(std::move(iStream_p), path, compressionLevel, format,
               memStrategy, inBuf, inBufSize, outBuf, outBufSize, putbackSize)
{
  // Associate _streamBuf to 'this' and clear the error state flags
  rdbuf(&_streamBuf);
}

ZlibCompressorIStream::~ZlibCompressorIStream()
{ }

// ***************************************************************************
// *                      ZlibDecompressorIStream class                      *
// ***************************************************************************

ZlibDecompressorIStream::ZlibDecompressorIStream(std::istream& iStream,
                                                 const SGPath& path,
                                                 ZLibCompressionFormat format,
                                                 char* inBuf,
                                                 std::size_t inBufSize,
                                                 char *outBuf,
                                                 std::size_t outBufSize,
                                                 std::size_t putbackSize)
  : std::istream(nullptr),
    _streamBuf(iStream, path, format, inBuf, inBufSize, outBuf, outBufSize,
               putbackSize)
{
  // Associate _streamBuf to 'this' and clear the error state flags
  rdbuf(&_streamBuf);
}

ZlibDecompressorIStream::ZlibDecompressorIStream(
  std::unique_ptr<std::istream> iStream_p,
  const SGPath& path,
  ZLibCompressionFormat format,
  char* inBuf,
  std::size_t inBufSize,
  char *outBuf,
  std::size_t outBufSize,
  std::size_t putbackSize)
  : std::istream(nullptr),
    _streamBuf(std::move(iStream_p), path, format, inBuf, inBufSize,
               outBuf, outBufSize, putbackSize)
{
  // Associate _streamBuf to 'this' and clear the error state flags
  rdbuf(&_streamBuf);
}

ZlibDecompressorIStream::~ZlibDecompressorIStream()
{ }

} // of namespace simgear
