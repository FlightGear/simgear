// -*- coding: utf-8 -*-
//
// zlibstream.hxx --- IOStreams classes for working with RFC 1950 and RFC 1952
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

#ifndef SG_ZLIBSTREAM_HXX
#define SG_ZLIBSTREAM_HXX

#include <ios>                  // std::streamsize
#include <istream>
#include <streambuf>
#include <memory>               // std::unique_ptr
#include <zlib.h>               // struct z_stream

#include <simgear/misc/sg_path.hxx>

// This file contains:
//
//  - two stream buffer classes (ZlibCompressorIStreambuf and
//    ZlibDecompressorIStreambuf), both based on the same abstract class:
//    ZlibAbstractIStreambuf;
//
//  - two std::istream subclasses (ZlibCompressorIStream and
//    ZlibDecompressorIStream), each creating and using the corresponding
//    stream buffer class from the previous item.
//
// All these allow one to work with RFC 1950 and RFC 1952 compression
// formats, respectively known as the zlib and gzip formats.
//
// These classes are *input* streaming classes, which means they can
// efficiently handle arbitrary amounts of data without using any disk
// space nor increasing amounts of memory, and allow “client code” to pull
// exactly as much data as it wants at any given time, resuming later
// when it is ready to handle the next chunk.
//
// So, for example, assuming you've created an instance of
// ZlibCompressorIStream (bound to some input stream of your choice, let's
// call it iStream), you could read 512 bytes of data from it, and you
// would get the first 512 bytes of *compressed* data corresponding to what
// iStream provided. Then you could resume at any time and ask for the next
// 512 bytes of compressed data (or any other amount), etc.
//
// Therefore, these classes are well suited, among others, to compress or
// decompress data streams while at the same time packing the result into
// discrete chunks or packets with size constraints (you can think of the
// process as making sausages :).
//
// The input being in each case an std::istream (for compressing as well as
// for decompressing), it can be tied to an arbitrary source: a file with
// sg_ifstream or std::ifstream, a memory buffer with std::istringstream or
// std::stringstream, a TCP socket with a custom std::streambuf subclass[1]
// to interface with the sockets API, etc.
//
//   [1] Possibly wrapped in an std::istream.
//
// The stream buffer classes upon which ZlibCompressorIStream and
// ZlibDecompressorIStream are built have an xsgetn() implementation that
// avoids useless copies of data by asking zlib to write directly to the
// destination buffer. This xsgetn() method is used when calling read() on
// the std::istream subclasses, or sgetn() if you are using the stream
// buffer classes directly (i.e., ZlibCompressorIStreambuf and
// ZlibDecompressorIStreambuf). Other std::istream methods may instead rely
// only on the internal buffer and the underflow() method, and therefore be
// less efficient for large amounts of data. You may want to take a look at
// zlibstream_test.cxx to see various ways of using these classes.
//
// In case you use std::istream& operator>>(std::istream&, std::string&) or
// its overload friends, beware that it splits fields at spaces, and by
// default ignores spaces at the beginning of a field (cf. std::skipws,
// std::noskipws and friends). As far as I understand it, most of these
// operators are mainly intended in the IOStreams library to be used to
// read an int here, a double there, a space-delimited string afterwards,
// etc. (the exception could be the overload writing to a stream buffer,
// however it doesn't seem to be very efficient on my system with GNU
// libstdc++ [it is not using xsgetn()], so beware also of this one if you
// are handling large amounts of data). For moderately complex or large
// input handling, I'd suggest to use std::istream methods such as read(),
// gcount() and getline() (std::getline() can be useful too). Or directly
// use the stream buffer classes, in particular with sgetn().


namespace simgear
{

enum class ZLibCompressionFormat {
  ZLIB = 0,
  GZIP,
  AUTODETECT,
  ZLIB_RAW, // No zlib header or trailer.
};

enum class ZLibMemoryStrategy {
  FAVOR_MEMORY_OVER_SPEED = 0,
  FAVOR_SPEED_OVER_MEMORY
};

// Abstract base class for both the compressor and decompressor stream buffers.
class ZlibAbstractIStreambuf: public std::streambuf
{
public:
  /**
   *  @brief Constructor for ZlibAbstractIStreambuf.
   *  @param iStream     Input stream to read from.
   *  @param path        Optional path to the file corresponding to iStream,
   *                     if any. Only used for error messages.
   *  @param inBuf       Pointer to the input buffer (data read from iStream is
   *                     written there before being compressed or decompressed).
   *                     If nullptr, the buffer is allocated on the heap in the
   *                     constructor and deallocated in the destructor.
   *  @param inBufSize   Size of the input buffer, in chars.
   *  @param outBuf      Pointer to the output buffer. Data is read by zlib
   *                     from the input buffer, compressed or decompressed, and
   *                     the result is directly written to the output buffer.
   *                     If nullptr, the buffer is allocated on the heap in the
   *                     constructor and deallocated in the destructor.
   *  @param outBufSize  Size of the output buffer, in chars.
   *  @param putbackSize Size of the putback area inside the output buffer, in
   *                     chars.
   *
   *  It is required that putbackSize < outBufSize. It is guaranteed that,
   *  if at least putbackSize chars have been read without any putback (or
   *  unget) operation intermixed, then at least putbackSize chars can be
   *  put back in sequence. If you don't need this feature, use zero for the
   *  putbackSize value (the default) for best performance.
  */
  explicit ZlibAbstractIStreambuf(std::istream& iStream,
                                  const SGPath& path = SGPath(),
                                  char* inBuf = nullptr,
                                  std::size_t inBufSize = 262144,
                                  char* outBuf = nullptr,
                                  std::size_t outBufSize = 262144,
                                  std::size_t putbackSize = 0);

  // Alternate constructor with sink semantics for the “source” std::istream.
  // When used, the class takes ownership of the std::istream instance pointed
  // to by the first constructor argument, and keeps it alive as long as the
  // object this constructor is for is itself alive.
  explicit ZlibAbstractIStreambuf(std::unique_ptr<std::istream> iStream_p,
                                  const SGPath& path = SGPath(),
                                  char* inBuf = nullptr,
                                  std::size_t inBufSize = 262144,
                                  char* outBuf = nullptr,
                                  std::size_t outBufSize = 262144,
                                  std::size_t putbackSize = 0);

  ZlibAbstractIStreambuf(const ZlibAbstractIStreambuf&) = delete;
  ZlibAbstractIStreambuf& operator=(const ZlibAbstractIStreambuf&) = delete;
  virtual ~ZlibAbstractIStreambuf();

protected:
  enum class OperationType {
    COMPRESSION = 0,
    DECOMPRESSION
  };

  virtual OperationType operationType() const = 0;

  // Either compress or decompress a chunk of data (depending on the
  // particular subclass implementation). The semantics are the same as for
  // zlib's inflate() and deflate() functions applied to member _zstream,
  // concerning 1) the return value and 2) where, and how much to read and
  // write (which thus depends on _zstream.avail_in, _zstream.next_in,
  // _zstream.avail_out and _zstream.next_out).
  virtual int zlibProcessData() = 0;

  // The input stream, from which data is read before being processed by zlib
  std::istream& _iStream;
  // Pointer to the same, used when calling the constructor that takes an
  // std::unique_ptr<std::istream> as its first argument; empty
  // std::unique_ptr object otherwise.
  std::unique_ptr<std::istream> _iStream_p;

  // Corresponding path, if any (default-constructed SGPath instance otherwise)
  const SGPath _path;
  // Structure used to communicate with zlib
  z_stream _zstream;

private:
  // Callback whose role is to refill the output buffer when it's empty and
  // the “client” tries to read more.
  virtual int underflow() override;

  // We only support seeking forwards, with way==std::ios_base::cur and
  // off>=0, returning 0. Otherwise we return -1.
  std::streampos seekoff(std::streamoff off,
                         std::ios_base::seekdir way,
                         std::ios_base::openmode which) override;

  // Optional override when subclassing std::streambuf. This is the most
  // efficient way of reading several characters (as soon as we've emptied the
  // output buffer, data is written by zlib directly to the destination
  // buffer).
  virtual std::streamsize xsgetn(char* dest, std::streamsize n) override;
  // Utility method for xsgetn()
  std::size_t xsgetn_preparePutbackArea(char* origGptr, char* dest,
                                        char* writePtr);
  // Make sure there is data to read in the input buffer, or signal EOF.
  bool getInputData();
  // Utility method for fillOutputBuffer()
  std::size_t fOB_remainingSpace(unsigned char* nextOutPtr) const;
  // Fill the output buffer (using zlib functions) as much as possible.
  char* fillOutputBuffer();
  // Utility method
  [[ noreturn ]] void handleZ_BUF_ERROR() const;

  bool _allFinished = false;

  // The buffers
  // ~~~~~~~~~~~
  //
  // The input buffer receives data obtained from _iStream, before it is
  // processed by zlib. In underflow(), zlib reads from this buffer it and
  // writes the resulting data(*) to the output buffer. Then we point the
  // standard std::streambuf pointers (gptr() and friends) directly towards
  // the data inside that output buffer. xsgetn() is even more optimized: it
  // first empties the output buffer, then makes zlib write the remaining data
  // directly to the destination area.
  //
  //   (*) Compressed or decompressed, depending on the particular
  //       implementation of zlibProcessData() in each subclass.
  char* _inBuf;
  const std::size_t _inBufSize;
  // _inBufEndPtr points right after the last data retrieved from _iStream and
  // stored into _inBuf. When zlib has read all such data, _zstream.next_in is
  // equal to _inBufEndPtr (after proper casting). Except in this particular
  // situation, only _zstream.next_in <= _inBufEndPtr is guaranteed.
  char* _inBufEndPtr;
  // Layout of the _outBuf buffer:
  //
  // |_outBuf  <putback area>  |_outBuf + _putbackSize    |_outBuf + _outBufSize
  //
  // The first _putbackSize chars in _outBuf are reserved for the putback area
  // (right-aligned at _outBuf + _putbackSize). The actual output buffer thus
  // starts at _outBuf + _putbackSize. At any given time for callers of this
  // class, the number of characters that can be put back is gptr() - eback().
  // It may be lower than _putbackSize if we haven't read that many characters
  // yet. It may also be larger if gptr() > _outBuf + _putbackSize, i.e.,
  // when the buffer for pending data is non-empty.
  //
  // At any given time, callers should see:
  //
  //   _outBuf <= eback() <= _outBuf + _putbackSize <= gptr() <= egptr()
  //                                                <= _outBuf + _outBufSize
  //
  // (hoping this won't get out of sync with the code!)
  char *_outBuf;
  const std::size_t _outBufSize;
  // Space reserved for characters to be put back into the stream. Must be
  // strictly smaller than _outBufSize (this is checked in the constructor).
  // It is guaranteed that this number of chars can be put back, except of
  // course if we haven't read that many characters from the input stream yet.
  // If characters are buffered in _outBuf[2], then it may be that more
  // characters than _putbackSize can be put back (it is essentially a matter
  // for std::streambuf of decreasing the “next pointer for the input
  // sequence”, i.e., the one returned by gptr()).
  //
  //   [2] In the [_outBuf + _putbackSize, _outBuf + _outBufSize) area.
  const std::size_t _putbackSize;

  // Since the constructor optionally allocates memory for the input and
  // output buffers, these members allow the destructor to know which buffers
  // have to be deallocated, if any.
  bool _inBufMustBeFreed = false;
  bool _outBufMustBeFreed = false;
};


// Stream buffer class for compressing data. Input data is obtained from an
// std::istream instance; the corresponding compressed data can be read using
// the standard std::streambuf read interface (mainly: sbumpc(), sgetc(),
// snextc(), sgetn(), sputbackc(), sungetc()). Input, uncompressed data is
// “pulled” as needed for the amount of compressed data requested by the
// “client” using the methods I just listed.
class ZlibCompressorIStreambuf: public ZlibAbstractIStreambuf
{
public:
  // Same parameters as for ZlibAbstractIStreambuf, except:
  //
  //   compressionLevel: in the [0,9] range. 0 means no compression at all.
  //                     Levels 1 to 9 yield compressed data, with 1 giving
  //                     the highest compression speed but worst compression
  //                     ratio, and 9 the highest compression ratio but lowest
  //                     compression speed.
  //   format            either ZLibCompressionFormat::ZLIB or
  //                     ZLibCompressionFormat::GZIP
  //   memStrategy       either ZLibMemoryStrategy::FAVOR_MEMORY_OVER_SPEED or
  //                     ZLibMemoryStrategy::FAVOR_SPEED_OVER_MEMORY
  explicit ZlibCompressorIStreambuf(
    std::istream& iStream,
    const SGPath& path = SGPath(),
    int compressionLevel = Z_DEFAULT_COMPRESSION,
    ZLibCompressionFormat format = ZLibCompressionFormat::ZLIB,
    ZLibMemoryStrategy memStrategy = ZLibMemoryStrategy::FAVOR_SPEED_OVER_MEMORY,
    char* inBuf = nullptr,
    std::size_t inBufSize = 262144,
    char* outBuf = nullptr,
    std::size_t outBufSize = 262144,
    std::size_t putbackSize = 0);

  // Alternate constructor with sink semantics for the “source” std::istream.
  explicit ZlibCompressorIStreambuf(
    std::unique_ptr<std::istream> _iStream_p,
    const SGPath& path = SGPath(),
    int compressionLevel = Z_DEFAULT_COMPRESSION,
    ZLibCompressionFormat format = ZLibCompressionFormat::ZLIB,
    ZLibMemoryStrategy memStrategy = ZLibMemoryStrategy::FAVOR_SPEED_OVER_MEMORY,
    char* inBuf = nullptr,
    std::size_t inBufSize = 262144,
    char* outBuf = nullptr,
    std::size_t outBufSize = 262144,
    std::size_t putbackSize = 0);

  ZlibCompressorIStreambuf(const ZlibCompressorIStreambuf&) = delete;
  ZlibCompressorIStreambuf& operator=(const ZlibCompressorIStreambuf&) = delete;
  virtual ~ZlibCompressorIStreambuf();

protected:
  virtual OperationType operationType() const override;
  // Initialize the z_stream struct used by zlib
  void zStreamInit(int compressionLevel, ZLibCompressionFormat format,
                   ZLibMemoryStrategy memStrategy);
  // Call zlib's deflate() function to compress data.
  virtual int zlibProcessData() override;
};


// Stream buffer class for decompressing data. Input data is obtained from an
// std::istream instance; the corresponding decompressed data can be read
// using the standard std::streambuf read interface (mainly: sbumpc(),
// sgetc(), snextc(), sgetn(), sputbackc(), sungetc()). Input, compressed data
// is “pulled” as needed for the amount of uncompressed data requested by the
// “client” using the methods I just listed.
class ZlibDecompressorIStreambuf: public ZlibAbstractIStreambuf
{
public:
  // Same parameters as for ZlibAbstractIStreambuf, except:
  //
  //   format            ZLibCompressionFormat::ZLIB,
  //                     ZLibCompressionFormat::GZIP or
  //                     ZLibCompressionFormat::AUTODETECT
  explicit ZlibDecompressorIStreambuf(
    std::istream& iStream,
    const SGPath& path = SGPath(),
    ZLibCompressionFormat format = ZLibCompressionFormat::ZLIB,
    char* inBuf = nullptr,
    std::size_t inBufSize = 262144,
    char* outBuf = nullptr,
    std::size_t outBufSize = 262144,
    std::size_t putbackSize = 0); // default optimized for speed

  // Alternate constructor with sink semantics for the “source” std::istream.
  explicit ZlibDecompressorIStreambuf(
    std::unique_ptr<std::istream> _iStream_p,
    const SGPath& path = SGPath(),
    ZLibCompressionFormat format = ZLibCompressionFormat::ZLIB,
    char* inBuf = nullptr,
    std::size_t inBufSize = 262144,
    char* outBuf = nullptr,
    std::size_t outBufSize = 262144,
    std::size_t putbackSize = 0); // default optimized for speed

  ZlibDecompressorIStreambuf(const ZlibDecompressorIStreambuf&) = delete;
  ZlibDecompressorIStreambuf& operator=(const ZlibDecompressorIStreambuf&)
                                                                      = delete;
  virtual ~ZlibDecompressorIStreambuf();

protected:
  virtual OperationType operationType() const override;
  void zStreamInit(ZLibCompressionFormat format);
  virtual int zlibProcessData() override;
};

// std::istream subclass for compressing data. Input data is obtained from an
// std::istream instance; the corresponding compressed data can be read using
// the standard std::istream interface (read(), readsome(), gcount(), get(),
// getline(), operator>>(), peek(), putback(), ignore(), unget()... plus
// operator overloads such as istream& operator>>(istream&, string&) as
// defined in <string>, and std::getline()). Input, uncompressed data is
// “pulled” as needed for the amount of compressed data requested by the
// “client”.
//
// To get data efficiently from an instance of this class, use its read()
// method (typically in conjunction with gcount(), inside a loop).
class ZlibCompressorIStream: public std::istream
{
public:
  // Same parameters as for ZlibCompressorIStreambuf
  explicit ZlibCompressorIStream(
    std::istream& iStream,
    const SGPath& path = SGPath(),
    int compressionLevel = Z_DEFAULT_COMPRESSION,
    ZLibCompressionFormat format = ZLibCompressionFormat::ZLIB,
    ZLibMemoryStrategy memStrategy = ZLibMemoryStrategy::FAVOR_SPEED_OVER_MEMORY,
    char* inBuf = nullptr,
    std::size_t inBufSize = 262144,
    char* outBuf = nullptr,
    std::size_t outBufSize = 262144,
    std::size_t putbackSize = 0); // default optimized for speed

  // Alternate constructor with sink semantics for the “source” std::istream.
  explicit ZlibCompressorIStream(
    std::unique_ptr<std::istream> _iStream_p,
    const SGPath& path = SGPath(),
    int compressionLevel = Z_DEFAULT_COMPRESSION,
    ZLibCompressionFormat format = ZLibCompressionFormat::ZLIB,
    ZLibMemoryStrategy memStrategy = ZLibMemoryStrategy::FAVOR_SPEED_OVER_MEMORY,
    char* inBuf = nullptr,
    std::size_t inBufSize = 262144,
    char* outBuf = nullptr,
    std::size_t outBufSize = 262144,
    std::size_t putbackSize = 0); // default optimized for speed

  ZlibCompressorIStream(const ZlibCompressorIStream&) = delete;
  ZlibCompressorIStream& operator=(const ZlibCompressorIStream&) = delete;
  virtual ~ZlibCompressorIStream();

private:
  ZlibCompressorIStreambuf _streamBuf;
};

// std::istream subclass for decompressing data. Input data is obtained from
// an std::istream instance; the corresponding decompressed data can be read
// using the standard std::istream interface (read(), readsome(), gcount(),
// get(), getline(), operator>>(), peek(), putback(), ignore(), unget()...
// plus operator overloads such as istream& operator>>(istream&, string&) as
// defined in <string>, and std::getline()). Input, compressed data is
// “pulled” as needed for the amount of uncompressed data requested by the
// “client”.
//
// To get data efficiently from an instance of this class, use its read()
// method (typically in conjunction with gcount(), inside a loop).
class ZlibDecompressorIStream: public std::istream
{
public:
  // Same parameters as for ZlibDecompressorIStreambuf
  explicit ZlibDecompressorIStream(
    std::istream& iStream,
    const SGPath& path = SGPath(),
    ZLibCompressionFormat format = ZLibCompressionFormat::ZLIB,
    char* inBuf = nullptr,
    std::size_t inBufSize = 262144,
    char* outBuf = nullptr,
    std::size_t outBufSize = 262144,
    std::size_t putbackSize = 0); // default optimized for speed

  // Alternate constructor with sink semantics for the “source” std::istream.
  explicit ZlibDecompressorIStream(
    std::unique_ptr<std::istream> _iStream_p,
    const SGPath& path = SGPath(),
    ZLibCompressionFormat format = ZLibCompressionFormat::ZLIB,
    char* inBuf = nullptr,
    std::size_t inBufSize = 262144,
    char* outBuf = nullptr,
    std::size_t outBufSize = 262144,
    std::size_t putbackSize = 0); // default optimized for speed

  ZlibDecompressorIStream(const ZlibDecompressorIStream&) = delete;
  ZlibDecompressorIStream& operator=(const ZlibDecompressorIStream&) = delete;
  virtual ~ZlibDecompressorIStream();

private:
  ZlibDecompressorIStreambuf _streamBuf;
};

} // of namespace simgear

#endif  // of SG_ZLIBSTREAM_HXX
