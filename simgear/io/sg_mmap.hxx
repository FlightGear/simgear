// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 1999 Curtis L. Olson - http://www.flightgear.org/~curt

/**
 * @file
 * @brief File I/O routines
 */

#include <simgear/compiler.h>

#include <string>
#include <cstring>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <simgear/misc/sg_path.hxx>
#include <simgear/misc/stdint.hxx>
#include <simgear/debug/logstream.hxx>
#include <simgear/misc/strutils.hxx>

#pragma once

#include <memory>
#include <string>

#include "iochannel.hxx"

// forward decls
class SGPath;

/**
 * A file I/O class based on SGIOChannel.
 */
class SGMMapFile : public SGIOChannel
{
public:

    SGMMapFile();

    /*
     * Create an instance of SGMMapFile.
     * When calling the constructor you need to provide a file
     * name. This file is not opened immediately, but instead will be
     * opened when the open() method is called.
     * @param file name of file to open
     * @param repeat On eof restart at the beginning of the file
     */
    SGMMapFile( const SGPath& file, int repeat_ = 1, int extraoflags = 0);

    /**
     * Create an SGMMapFile from an existing, open file-descriptor
     */
    SGMMapFile( int existingFd );

    /** Destructor */
    virtual ~SGMMapFile();

    // open the file based on specified direction
    bool open( const SGPath& file, const SGProtocolDir dir );
    bool open(const SGProtocolDir dir) override;

    // read a block of data of specified size
    int read(char* buf, int length) override;

    // read a block of data of specified size
    int read( char *buf, int length, int num );

    // read a line of data, length is max size of input buffer
    int readline(char* buf, int length) override;

    // reads the whole file into a buffer
    // note: this really defeats the purpose of mmapping a file
    std::string read_all();

    // get the pointer to the start of the buffer
    const char* get() const;

    // get the pointer to the current offset of the buffer
    const char* ptr() const;

    // get the pointer at the current offset and increase the offset by amount
    // returns nullptr if the offset pointer would end up beyond the mmap
    // buffer size
    const char* advance(off_t amount);

    // return the size of the mmaped area
    size_t get_size() const;

    // forward by amount, returns the amount which could be forwarded
    // without the offset getting beyond the mmap buffer size.
     // returns 0 on end of file.
    off_t forward(off_t amount);

    // rewind the offset pointer
    void rewind();

    // write data to a file
    int write(const char* buf, const int length) override;

    // write null terminated string to a file
    int writestring(const char* str) override;

    // close file
    bool close() override;

    /** @return the name of the file being manipulated. */
    std::string get_file_name() const;

    /** @return true of eof conditions exists */
    bool eof() const override;

private:
    class SGMMapFilePrivate;
    std::unique_ptr<SGMMapFilePrivate> d;
};
