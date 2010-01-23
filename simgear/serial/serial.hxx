/**
 * \file serial.hxx
 * Low level serial I/O support (for unix/cygwin and windows)
 */

// Written by Curtis Olson, started November 1998.
//
// Copyright (C) 1998  Curtis L. Olson - http://www.flightgear.org/~curt
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


#ifndef _SERIAL_HXX
#define _SERIAL_HXX


#ifndef __cplusplus
# error This library requires C++
#endif

#ifdef _WIN32
#  include <windows.h>
#endif

#include <simgear/compiler.h>
#include <string>
using std::string;

// if someone know how to do this all with C++ streams let me know
// #include <stdio.h>


/**
 * A class to encapsulate low level serial port IO.
 */
class SGSerialPort
{
#ifdef  _WIN32
    typedef HANDLE fd_type;
#else
    typedef int fd_type;
#endif

private:

    fd_type fd;
    bool dev_open;

public:

    /** Default constructor */
    SGSerialPort();

    /**
     * Constructor
     * @param device device name
     * @param baud baud rate
     */
    SGSerialPort(const string& device, int baud);

    /** Destructor */
    ~SGSerialPort();

    /** Open a the serial port
     * @param device name of device
     * @return success/failure
     */
    bool open_port(const string& device);

    /** Close the serial port
     * @return success/failure 
     */
    bool close_port();

    /** Set baud rate
     * @param baud baud rate
     * @return success/failure
     */
    bool set_baud(int baud);

    /** Read from the serial port
     * @return line of data
     */
    string read_port();

    /** Read from the serial port
     * @param buf input buffer
     * @param len length of buffer (i.e. max number of bytes to read
     * @return number of bytes read
     */
    int read_port(char *buf, int len);

    /** Write to the serial port
     * @param value output string
     * @return number of bytes written
     */
    int write_port(const string& value);

    /** Write to the serial port
     * @param buf pointer to character buffer containing output data
     * @param len number of bytes to write from the buffer
     * @return number of bytes written
     */
    int write_port(const char *buf, int len);

    /** @return true if device open */
    inline bool is_enabled() { return dev_open; }
};


#endif // _SERIAL_HXX


