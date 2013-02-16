/**
 * \file zfstream.hxx
 * A C++ I/O streams interface to the zlib gz* functions.
 */

// Written by Bernie Bright, 1998
// Based on zlib/contrib/iostream/ by Kevin Ruland <kevin@rodin.wustl.edu>
//
// Copyright (C) 1998  Bernie Bright - bbright@c031.aone.net.au
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
// $Id$

#ifndef _zfstream_hxx
#define _zfstream_hxx

#include <simgear/compiler.h>

#include <zlib.h>


#include <streambuf>
#include <istream>

#define ios_openmode std::ios_base::openmode
#define ios_in       std::ios_base::in
#define ios_out      std::ios_base::out
#define ios_app      std::ios_base::app
#define ios_binary   std::ios_base::binary

#define ios_seekdir  std::ios_base::seekdir

#define ios_badbit   std::ios_base::badbit
#define ios_failbit  std::ios_base::failbit

/**
 * A C++ I/O streams interface to the zlib gz* functions.
 */
#ifdef SG_NEED_STREAMBUF_HACK
class gzfilebuf : public __streambuf {
    typedef __streambuf parent;
#else
class gzfilebuf : public std::streambuf {
    typedef std::streambuf parent;
#endif

public:
    /** Constructor */
    gzfilebuf();

    /** Destructor */
    virtual ~gzfilebuf();

    /**
     * Open a stream
     * @param name file name
     * @param io_mode mode flags
     * @return file stream
     */
    gzfilebuf* open( const char* name, ios_openmode io_mode );

    /** 
     * Attach to an existing file descriptor
     * @param file_descriptor file descriptor
     * @param io_mode mode flags
     * @return file stream
     */
    gzfilebuf* attach( int file_descriptor, ios_openmode io_mode );

    /** Close stream */
    gzfilebuf* close();

    int setcompressionlevel( int comp_level );
    int setcompressionstrategy( int comp_strategy );

    /** @return true if open, false otherwise */
    bool is_open() const { return (file != NULL); }

    /** @return stream position */
    virtual std::streampos seekoff( std::streamoff off, ios_seekdir way, ios_openmode which );

    /** sync the stream */
    virtual int sync();

protected:

    virtual int_type underflow();

    virtual int_type overflow( int_type c = parent::traits_type::eof() );
    bool    out_waiting();
    char*   base() {return obuffer;}
    int     blen() {return obuf_size;}
    char    allocate();

private:

    int_type flushbuf();
    int fillbuf();

    // Convert io_mode to "rwab" string.
    void cvt_iomode( char* mode_str, ios_openmode io_mode );

private:

    gzFile file;
    ios_openmode mode;
    bool own_file_descriptor;

    // Get (input) buffer.
    int ibuf_size;
    char* ibuffer;

    // Put (output) buffer.
    int obuf_size;
    char* obuffer;

    enum { page_size = 65536 };

private:
    // Not defined
    gzfilebuf( const gzfilebuf& );
    void operator= ( const gzfilebuf& );
};

/**
 * document me
 */
struct gzifstream_base
{
    gzifstream_base() {}

    gzfilebuf gzbuf;
};

/**
 * document me too
 */
struct gzofstream_base
{
    gzofstream_base() {}

    gzfilebuf gzbuf;
};

#endif // _zfstream_hxx
