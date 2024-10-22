// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 1999 Curtis L. Olson - http://www.flightgear.org/~curt

/**
 * @file
 * @brief File I/O routines
 */

#ifndef _SG_FILE_HXX
#define _SG_FILE_HXX

#include <simgear/compiler.h>

#include <simgear/misc/sg_path.hxx>

#include "iochannel.hxx"

#include <string>

/**
 * A file I/O class based on SGIOChannel.
 */
class SGFile : public SGIOChannel {

    SGPath file_name;
    int fp;
    bool eof_flag;
    // Number of repetitions to play. -1 means loop infinitely.
    const int repeat;
    int iteration;              // number of current repetition,
                                // starting at 0
    int extraoflags;

public:

    /**
     * Create an instance of SGFile.
     * When calling the constructor you need to provide a file
     * name. This file is not opened immediately, but instead will be
     * opened when the open() method is called.
     * @param file name of file to open
     * @param repeat On eof restart at the beginning of the file
     */
    SGFile( const SGPath& file, int repeat_ = 1, int extraoflags = 0);

    /**
     * Create an SGFile from an existing, open file-descriptor
     */
    SGFile( int existingFd );

    /** Destructor */
    virtual ~SGFile();

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
    std::string get_file_name() const { return file_name.utf8Str(); }

    const SGPath& get_path() const { return file_name; }

    /** @return true of eof conditions exists */
    virtual bool eof() const { return eof_flag; };

    std::string computeHash();

};

class SGBinaryFile : public SGFile {
public:
    SGBinaryFile( const SGPath& file, int repeat_ = 1 );
};

#endif // _SG_FILE_HXX
