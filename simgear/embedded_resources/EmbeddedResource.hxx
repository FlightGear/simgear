// -*- coding: utf-8 -*-
//
// EmbeddedResource.hxx --- Class for pointing to/accessing an embedded resource
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

#ifndef FG_EMBEDDEDRESOURCE_HXX
#define FG_EMBEDDEDRESOURCE_HXX

#include <iosfwd>
#include <string>
#include <ostream>
#include <memory>               // std::unique_ptr
#include <cstddef>              // std::size_t, std::ptrdiff_t

#include <simgear/io/iostreams/zlibstream.hxx>


namespace simgear
{

// Abstract base class for embedded resources
class AbstractEmbeddedResource
{
public:
  enum class CompressionType {
    NONE = 0,
    ZLIB
  };

  // Constructor.
  //
  // 'data' and 'size' indicate the resource contents. There is no requirement
  // of null-termination, including for text data (given how
  // EmbeddedResourceManager::getString() works, including a null terminator
  // for text contents is actually counter-productive). The data may be of
  // arbitrary type and size: binary, text, whatever. The constructed object
  // (for derived classes since this one is abstract) does *not* hold a copy
  // of the data, it just keeps a pointer to it and provides methods to access
  // it. The data must therefore remain available as long as the object is in
  // use---this class was designed for use with data stored in static
  // variables.
  explicit AbstractEmbeddedResource(const char *data, std::size_t size);
  AbstractEmbeddedResource(const AbstractEmbeddedResource&) = default;
  AbstractEmbeddedResource(AbstractEmbeddedResource&&) = default;
  AbstractEmbeddedResource& operator=(const AbstractEmbeddedResource&) = default;
  AbstractEmbeddedResource& operator=(AbstractEmbeddedResource&&) = default;
  virtual ~AbstractEmbeddedResource() = default;

  // Return the pointer to beginning-of-resource contents---the same that was
  // passed to the constructor.
  const char *rawPtr() const;
  // Return the resource size, as passed to the constructor. For a compressed
  // resource, this is the compressed size; such resources provide an
  // additional uncompressedSize() method.
  std::size_t rawSize() const;

  // Return an std::string object containing a copy of the resource contents.
  // For a compressed resource, this is the data obtained after decompression.
  virtual std::string str() const;
  // Return an std::streambuf instance providing read-only access to the
  // resource contents (in uncompressed form for compressed resources). This
  // allows memory-friendly access to large resources by enabling incremental
  // processing with transparent decompression for compressed resources.
  virtual std::unique_ptr<std::streambuf> streambuf() const = 0;
  // Return an std::istream instance providing read-only access to the
  // resource contents (in uncompressed form for compressed resources).
  //
  // The same remark as for streambuf() applies. std::istream is simply a
  // higher-level interface than std::streambuf, otherwise both allow the same
  // kind of processing.
  virtual std::unique_ptr<std::istream> istream() const = 0;

  // Return the resource compression type.
  virtual CompressionType compressionType() const = 0;
  // Return a string description of the resource compression type. Examples:
  // "none", "zlib".
  virtual std::string compressionDescr() const = 0;

private:
  // Pointer to the start of resource contents
  const char *_data;
  // Size of resource contents, in bytes
  std::size_t _size;
};

// Class to describe an uncompressed resource. See AbstractEmbeddedResource.
class RawEmbeddedResource : public AbstractEmbeddedResource
{
public:
  explicit RawEmbeddedResource(const char *data, std::size_t size);

  AbstractEmbeddedResource::CompressionType compressionType() const override;
  std::string compressionDescr() const override;

  // The str() method is inherited from AbstractEmbeddedResource
  std::unique_ptr<std::streambuf> streambuf() const override;
  std::unique_ptr<std::istream> istream() const override;
};

// Class to describe a zlib-compressed resource.
//
// Instances of this class point to resource contents stored in the stream
// format documented in RFC 1950.
class ZlibEmbeddedResource : public AbstractEmbeddedResource
{
public:
  explicit ZlibEmbeddedResource(const char *data, std::size_t compressedSize,
                                std::size_t uncompressedSize);

  AbstractEmbeddedResource::CompressionType compressionType() const override;
  std::string compressionDescr() const override;
  // Return the resource uncompressed size, in bytes.
  std::size_t uncompressedSize() const;

  std::string str() const override;
  std::unique_ptr<std::streambuf> streambuf() const override;
  std::unique_ptr<std::istream> istream() const override;

  // Getters and setters for parameters used in streambuf() and istream().
  // Calling any of the setters affects the subsequent streambuf() and
  // istream() calls.
  char* getInputBufferStart();
  void setInputBufferStart(char* inBuf);
  std::size_t getInputBufferSize();
  void setInputBufferSize(std::size_t size);
  char* getOutputBufferStart();
  void setOutputBufferStart(char* outBuf);
  std::size_t getOutputBufferSize();
  void setOutputBufferSize(std::size_t size);
  std::size_t getPutbackSize();
  void setPutbackSize(std::size_t size);

private:
  std::size_t _uncompressedSize;
  char* _inBuf;
  std::size_t _inBufSize;
  char* _outBuf;
  std::size_t _outBufSize;
  std::size_t _putbackSize;
};

// These functions are essentially intended for troubleshooting purposes.
std::ostream& operator<<(std::ostream&, const RawEmbeddedResource&);
std::ostream& operator<<(std::ostream&, const ZlibEmbeddedResource&);

} // of namespace simgear

#endif  // of FG_EMBEDDEDRESOURCE_HXX
