// Written by James Turner
//
// Copyright (C) 2013  James Turner  <zakalawe@mac.com>
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
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//

#include "HTTPContentDecode.hxx"

#include <cassert>
#include <cstdlib> // rand()
#include <cstring> // for memset, memcpy

#include <simgear/debug/logstream.hxx>
#include <simgear/structure/exception.hxx>
#include <simgear/io/lowlevel.hxx> // for sgEndian stuff

namespace simgear
{

namespace HTTP
{

    const int ZLIB_DECOMPRESS_BUFFER_SIZE = 32 * 1024;
    const int ZLIB_INFLATE_WINDOW_BITS = -MAX_WBITS;
  
    // see http://www.ietf.org/rfc/rfc1952.txt for these values and
    // detailed description of the logic 
    const int GZIP_HEADER_ID1 = 31;
    const int GZIP_HEADER_ID2 = 139;
    const int GZIP_HEADER_METHOD_DEFLATE = 8;
    const unsigned int GZIP_HEADER_SIZE = 10;
    const int GZIP_HEADER_FEXTRA = 1 << 2;
    const int GZIP_HEADER_FNAME = 1 << 3;
    const int GZIP_HEADER_COMMENT = 1 << 4;
    const int GZIP_HEADER_CRC = 1 << 1;
    
ContentDecoder::ContentDecoder() :
    _output(NULL),
    _zlib(NULL),
    _input(NULL),
    _inputAllocated(0),
    _inputSize(0)
{
}

ContentDecoder::~ContentDecoder()
{
    free(_output);
    free(_input);
    free(_zlib);
}

void ContentDecoder::setEncoding(const std::string& encoding)
{
    if (encoding == "gzip") {
      _contentDeflate = true;
      _needGZipHeader = true;
    } else if (encoding == "deflate") {
      _contentDeflate = true;
      _needGZipHeader = false;
    } else if (encoding != "identity") {
      SG_LOG(SG_IO, SG_WARN, "unsupported content encoding:" << encoding);
    }
}

void ContentDecoder::reset()
{
    _request = NULL;
    _contentDeflate = false;
    _needGZipHeader = false;
    _inputSize = 0;
}

void ContentDecoder::initWithRequest(Request_ptr req)
{
    _request = req;
    if (!_contentDeflate) {
        return;
    }

    if (!_zlib) {
        _zlib = (z_stream*) malloc(sizeof(z_stream));
    }
    
    memset(_zlib, 0, sizeof(z_stream));
    if (!_output) {
         _output = (unsigned char*) malloc(ZLIB_DECOMPRESS_BUFFER_SIZE);
    }
    
    _inputSize = 0;
    // NULLs means we'll get default alloc+free methods
    // which is absolutely fine
    _zlib->avail_out = ZLIB_DECOMPRESS_BUFFER_SIZE;
    _zlib->next_out = _output;
    if (inflateInit2(_zlib, ZLIB_INFLATE_WINDOW_BITS) != Z_OK) {
      SG_LOG(SG_IO, SG_WARN, "inflateInit2 failed");
    }
}

void ContentDecoder::finish()
{    
    if (_contentDeflate) {
        runDecoder();
      inflateEnd(_zlib);
    }
}

void ContentDecoder::receivedBytes(const char* n, size_t s)
{
    if (!_contentDeflate) {
        _request->processBodyBytes(n, s);
        return;
    }    
    
// allocate more space if needed (this will only happen rarely once the
// buffer has hit something proportionate to the server's compression
// window size)
    size_t requiredSize = _inputSize + s;
    if (requiredSize > _inputAllocated) {
        reallocateInputBuffer(requiredSize);
    }
    
// copy newly recieved bytes into the buffer
    memcpy(_input + _inputSize, n, s);
    _inputSize += s;
    
    if (_needGZipHeader && !consumeGZipHeader()) {
        // still waiting on the full GZIP header, so done
        return;
    }
    
    runDecoder();
}

void ContentDecoder::consumeBytes(size_t consumed)
{    
    assert(_inputSize >= consumed);
// move existing (consumed) bytes down
    if (consumed > 0) {
        size_t newSize = _inputSize - consumed;
        memmove(_input, _input + consumed, newSize);
        _inputSize = newSize;
    }
}

void ContentDecoder::reallocateInputBuffer(size_t newSize)
{    
    _input = (unsigned char*) realloc(_input, newSize);
    _inputAllocated = newSize;
}

void ContentDecoder::runDecoder()
{
    _zlib->next_in = (unsigned char*) _input;
    _zlib->avail_in = _inputSize;
    int writtenSize;
    
    // loop, running zlib() inflate and sending output bytes to
    // our request body handler. Keep calling inflate until no bytes are
    // written, and ZLIB has consumed all available input
    do {
        _zlib->next_out = _output;
        _zlib->avail_out = ZLIB_DECOMPRESS_BUFFER_SIZE;
      int result = inflate(_zlib, Z_NO_FLUSH);
      if (result == Z_OK || result == Z_STREAM_END) {
          // nothing to do
      } else if (result == Z_BUF_ERROR) {
          // transient error, fall through
      } else {
        //  _error = result;          
        return;
      }
          
      writtenSize = ZLIB_DECOMPRESS_BUFFER_SIZE - _zlib->avail_out;      
      if (writtenSize > 0) {
          _request->processBodyBytes((char*) _output, writtenSize);
      }
      
      if (result == Z_STREAM_END) {
          break;
      }
    } while ((_zlib->avail_in > 0) || (writtenSize > 0));
    
    // update input buffers based on what we consumed
    consumeBytes(_inputSize - _zlib->avail_in);
}

bool ContentDecoder::consumeGZipHeader()
{    
    size_t avail = _inputSize;
    if (avail < GZIP_HEADER_SIZE) {
      return false; // need more header bytes
    }
    
    if ((_input[0] != GZIP_HEADER_ID1) ||
        (_input[1] != GZIP_HEADER_ID2) ||
        (_input[2] != GZIP_HEADER_METHOD_DEFLATE))
    {
      return false; // invalid GZip header
    }
    
    char flags = _input[3];
    unsigned int gzipHeaderSize =  GZIP_HEADER_SIZE;
    if (flags & GZIP_HEADER_FEXTRA) {
      gzipHeaderSize += 2;
      if (avail < gzipHeaderSize) {
        return false; // need more header bytes
      }
      
      unsigned short extraHeaderBytes = *(reinterpret_cast<unsigned short*>(_input + GZIP_HEADER_FEXTRA));
      if ( sgIsBigEndian() ) {
          sgEndianSwap( &extraHeaderBytes );
      }
      
      gzipHeaderSize += extraHeaderBytes;
      if (avail < gzipHeaderSize) {
        return false; // need more header bytes
      }
    }

#if 0
    if (flags & GZIP_HEADER_FNAME) {
      gzipHeaderSize++;
      while (gzipHeaderSize <= avail) {
        if (_input[gzipHeaderSize-1] == 0) {
          break; // found terminating NULL character
        }
      }
    }
    
    if (flags & GZIP_HEADER_COMMENT) {
      gzipHeaderSize++;
      while (gzipHeaderSize <= avail) {
        if (_input[gzipHeaderSize-1] == 0) {
          break; // found terminating NULL character
        }
      }
    }
#endif
        
    if (flags & GZIP_HEADER_CRC) {
      gzipHeaderSize += 2;
    }
    
    if (avail < gzipHeaderSize) {
      return false; // need more header bytes
    }
    
    consumeBytes(gzipHeaderSize);
    _needGZipHeader = false;
    return true;
}

} // of namespace HTTP

} // of namespace simgear
