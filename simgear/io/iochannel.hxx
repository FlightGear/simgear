/**
 * \file iochannel.hxx
 * High level IO channel base class.
 */

// Written by Curtis Olson, started November 1999.
//
// Copyright (C) 1999  Curtis L. Olson - http://www.flightgear.org/~curt
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// $Id$


#ifndef _IOCHANNEL_HXX
#define _IOCHANNEL_HXX


#include <simgear/compiler.h>

#define SG_IO_MAX_MSG_SIZE 16384

/**
 * Specify if this is a read (IN), write (OUT), or r/w (BI) directional
 * channel
 */
enum SGProtocolDir {
    SG_IO_NONE = 0,
    SG_IO_IN = 1,
    SG_IO_OUT = 2,
    SG_IO_BI = 3
};

/**
 * Specify the channel type
 */
enum SGChannelType {
    sgFileType = 0,
    sgSerialType = 1,
    sgSocketType = 2
};


/**
 * The SGIOChannel base class provides a consistent method for
 * applications to communication through various mediums. By providing
 * a base class with multiple derived classes, and application such as
 * FlightGear can implement a way to speak any protocol via any kind
 * of I/O channel.
 *
 * All of the SGIOChannel derived classes have exactly the same usage
 * interface once an instance has been created.
 *
 */
class SGIOChannel {

    SGChannelType type;
    SGProtocolDir dir;
    bool valid;

public:

    /** Constructor */
    SGIOChannel();

    /** Destructor */
    virtual ~SGIOChannel();

    /** Open a channel.
     * @param d channel communication "direction"
     * Direction can be one of: 
     *  - SG_IO_IN - data will be flowing into this object to the application. 
     *  - SG_IO_OUT - data will be flowing out of this object from the
     *                application. 
     *  - SG_IO_BI - data will be flowing in both directions. 
     *  - SG_IO_NONE - data will not be flowing in either direction.
     *                 This is here for the sake of completeness. 
     * @return result of open
     */
    virtual bool open( const SGProtocolDir d );

    /**
     * The read() method is modeled after the read() Unix system
     * call. You must provide a pointer to a character buffer that has
     * enough allocated space for your potential read. You can also
     * specify the maximum number of bytes allowed for this particular
     * read. The actual number of bytes read is returned.  You are
     * responsible to ensure that the size of buf is large enough to
     * accomodate your input message
     * @param buf a char pointer to your input buffer 
     * @param length max number of bytes to read
     * @return number of bytes read
     */
    virtual int read( char *buf, int length );

    /**
     * The readline() method is similar to read() except that it will
     * stop at the first end of line encountered in the input buffer.
     * @param buf a char pointer to your input buffer 
     * @param length max number of bytes to read
     * @return number of bytes read
     */
    virtual int readline( char *buf, int length );


    /**
     * The write() method is modeled after the write() Unix system
     * call and is analogous to the read() method. You provide a
     * pointer to a buffer of data, and then length of that data to be
     * written out. The number of bytes written is returned.
     * @param buf a char pointer to your output buffer 
     * @param length number of bytes to write
     * @return number of bytes written
     */
    virtual int write( const char *buf, const int length );

    /**
     * The writestring() method is a simple wrapper that will
     * calculate the length of a null terminated character array and
     * write it to the output channel.
     * @param buf a char pointer to your output buffer 
     * @return number of bytes written
     */
    virtual int writestring( const char *str );

    /**
     * The close() method is modeled after the close() Unix system
     * call and will close an open device. You should call this method
     * when you are done using your IO class, before it is destructed.
     * @return result of close
     */
    virtual bool close();

    /**
     * The eof() method returns true if end of file has been reached
     * in a context where that makes sense.  Otherwise it returns
     * false.
     * @return result of eof check
     */
    virtual bool eof() const;

    inline void set_type( SGChannelType t ) { type = t; }
    inline SGChannelType get_type() const { return type; }

    inline void set_dir( const SGProtocolDir d ) { dir = d; }
    inline SGProtocolDir get_dir() const { return dir; }
    inline bool isvalid() const { return valid; }
    inline void set_valid( const bool v ) { valid = v; }
};


#endif // _IOCHANNEL_HXX


