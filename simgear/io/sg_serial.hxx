/**
 * \file sg_serial.hxx
 * Serial I/O routines
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


#ifndef _SG_SERIAL_HXX
#define _SG_SERIAL_HXX


#ifndef __cplusplus
# error This library requires C++
#endif

#include <simgear/compiler.h>

#include <string>

#include <simgear/serial/serial.hxx>

#include "iochannel.hxx"

/**
 * A serial I/O class based on SGIOChannel.
 */
class SGSerial : public SGIOChannel {

    std::string device;
    std::string baud;
    SGSerialPort port;

    char save_buf[ 2 * SG_IO_MAX_MSG_SIZE ];
    int save_len;

public:

    /**
     * Create an instance of SGSerial.
     * This creates an instance of the SGSerial class. You need to
     * provide the serial device name and desired baud rate.  For Unix
     * style systems, device names will be similar to
     * ``/dev/ttyS0''. For DOS style systems you may want to use
     * something similar to ``COM1:''. As with the SGFile class,
     * device is not opened immediately, but instead will be opened
     * when the open() method is called.
     * @param device_name name of serial device
     * @param baud_rate speed of communication
     */
    SGSerial( const std::string& device_name, const std::string& baud_rate );

    /** Destructor */
    ~SGSerial();

    // open the serial port based on specified direction
    bool open( const SGProtocolDir d );

    // read a block of data of specified size
    int read( char *buf, int length );

    // read a line of data, length is max size of input buffer
    int readline( char *buf, int length );

    // write data to port
    int write( const char *buf, const int length );

    // write null terminated string to port
    int writestring( const char *str );

    // close port
    bool close();

    /** @return the serial port device name */
    inline string get_device() const { return device; }

    /** @return the baud rate */
    inline string get_baud() const { return baud; }
};


#endif // _SG_SERIAL_HXX


