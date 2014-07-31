///@file
/// File I/O routines.
//
// Written by Curtis Olson, started November 1999.
//
// Copyright (C) 1999  Curtis L. Olson - http://www.flightgear.org/~curt
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
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA.

#ifndef _SG_FILE_HXX
#define _SG_FILE_HXX

#include <simgear/compiler.h>
#include "iochannel.hxx"

#include <string>

/**
 * A file I/O class based on SGIOChannel.
 */
class SGFile : public SGIOChannel {

    std::string file_name;
    int fp;
    bool eof_flag;
    // Number of repetitions to play. -1 means loop infinitely.
    const int repeat;
    int iteration;              // number of current repetition,
                                // starting at 0

public:

    /**
     * Create an instance of SGFile.
     * When calling the constructor you need to provide a file
     * name. This file is not opened immediately, but instead will be
     * opened when the open() method is called.
     * @param file name of file to open
     * @param repeat On eof restart at the beginning of the file
     */
    SGFile( const std::string& file, int repeat_ = 1 );

    /**
     * Create an SGFile from an existing, open file-descriptor
     */
    SGFile( int existingFd );

    /** Destructor */
    ~SGFile();

    // open the file based on specified direction
    bool open( const SGProtocolDir dir );

    // read a block of data of specified size
    int read( char *buf, int length );

    // read a line of data, length is max size of input buffer
    int readline( char *buf, int length );

    // write data to a file
    int write( const char *buf, const int length );

    // write null terminated string to a file
    int writestring( const char *str );

    // close file
    bool close();

    /** @return the name of the file being manipulated. */
    inline std::string get_file_name() const { return file_name; }

    /** @return true of eof conditions exists */
    virtual bool eof() const { return eof_flag; };
};

#endif // _SG_FILE_HXX
