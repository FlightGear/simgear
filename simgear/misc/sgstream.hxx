/**
 * \file sgstream.hxx
 * zlib input file stream wrapper.
 */

// Written by Bernie Bright, 1998
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


#ifndef _SGSTREAM_HXX
#define _SGSTREAM_HXX

#ifndef __cplusplus
# error This library requires C++
#endif

#include <simgear/compiler.h>

#  include <istream>
#  include <ostream>

#include <string>

#include <simgear/misc/zfstream.hxx>

/**
 * An envelope class for gzifstream.
 */
class sg_gzifstream : private gzifstream_base, public std::istream
{
public:
    /** Default constructor */
    sg_gzifstream();

    /**
     * Constructor that attempt to open a file with and without
     * ".gz" extension.
     * @param name name of file
     * @param io_mode file open mode(s) "or'd" together
     */
    sg_gzifstream( const std::string& name,
		   ios_openmode io_mode = ios_in | ios_binary );

    /**
     * Constructor that attaches itself to an existing file descriptor.
     * @param fd file descriptor
     * @param io_mode file open mode(s) "or'd" together
     */
    sg_gzifstream( int fd, ios_openmode io_mode = ios_in|ios_binary );

    /**
     * Attempt to open a file with and without ".gz" extension.
     * @param name name of file
     * @param io_mode file open mode(s) "or'd" together
     */
    void open( const std::string& name,
	       ios_openmode io_mode = ios_in|ios_binary );

    /**
     * Attach to an existing file descriptor.
     * @param fd file descriptor
     * @param io_mode file open mode(s) "or'd" together
     */
    void attach( int fd, ios_openmode io_mode = ios_in|ios_binary );

    /**
     * Close the stream.
     */
    void close() { gzbuf.close(); }

    /** @return true if the file is successfully opened, false otherwise. */
    bool is_open() { return gzbuf.is_open(); }

private:
    // Not defined!
    sg_gzifstream( const sg_gzifstream& );    
    void operator= ( const sg_gzifstream& );    
};

/**
 * \relates sg_gzifstream
 * An istream manipulator that skips to end of line.
 * @param in input stream
 */
std::istream& skipeol( std::istream& in );

/**
 * \relates sg_gzifstream
 * An istream manipulator that skips over white space.
 * @param in input stream
 */
std::istream& skipws( std::istream& in );

/**
 * \relates sg_gzifstream
 * An istream manipulator that skips comments and white space.
 * Ignores comments that start with '#'.
 * @param in input stream
 */
std::istream& skipcomment( std::istream& in );

/**
 * An envelope class for gzofstream.
 */
class sg_gzofstream : private gzofstream_base, public std::ostream
{
public:
    /** Default constructor */
    sg_gzofstream();

    /**
     * Constructor to open a file for writing.
     * @param name name of file
     * @param io_mode file open mode(s) "or'd" together
     */
    sg_gzofstream( const std::string& name,
           ios_openmode io_mode = ios_out | ios_binary );

    /**
     * Constructor that attaches itself to an existing file descriptor.
     * @param fd file descriptor
     * @param io_mode file open mode(s) "or'd" together
     */
    sg_gzofstream( int fd, ios_openmode io_mode = ios_out|ios_binary );

    /**
     * Attempt to open a file for writing.
     * @param name name of file
     * @param io_mode file open mode(s) "or'd" together
     */
    void open( const std::string& name,
           ios_openmode io_mode = ios_out|ios_binary );

    /**
     * Attach to an existing file descriptor.
     * @param fd file descriptor
     * @param io_mode file open mode(s) "or'd" together
     */
    void attach( int fd, ios_openmode io_mode = ios_out|ios_binary );

    /**
     * Close the stream.
     */
    void close() { gzbuf.close(); }

    /** @return true if the file is successfully opened, false otherwise. */
    bool is_open() { return gzbuf.is_open(); }

private:
    // Not defined!
    sg_gzofstream( const sg_gzofstream& );
    void operator= ( const sg_gzofstream& );
};

#endif /* _SGSTREAM_HXX */

