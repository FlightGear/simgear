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

#ifndef SG_HTTP_CONTENT_DECODER_HXX
#define SG_HTTP_CONTENT_DECODER_HXX

#include <string>

#include <zlib.h>

#include <simgear/io/HTTPRequest.hxx>


namespace simgear
{

namespace HTTP
{

class ContentDecoder
{
public:
    ContentDecoder();
    ~ContentDecoder();
    
    void reset();
    
    void initWithRequest(Request_ptr req);
    
    void finish();
    
    void setEncoding(const std::string& encoding);
    
    void receivedBytes(const char* n, size_t s);

private:
    bool consumeGZipHeader();
    void runDecoder();
    
    void consumeBytes(size_t consumed);
    void reallocateInputBuffer(size_t newSize);
    
    Request_ptr _request;
    unsigned char* _output;
    
    z_stream* _zlib;
    unsigned char* _input;
    size_t _inputAllocated, _inputSize;
    bool _contentDeflate, _needGZipHeader;
};

} // of namespace HTTP

} // of namespace simgear

#endif // of SG_HTTP_CONTENT_DECODER_HXX
