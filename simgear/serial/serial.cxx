// serial.cxx -- Unix serial I/O support
//
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


#include <simgear/compiler.h>

#include <iostream>
#include <cerrno>

#ifndef _WIN32
#  include <termios.h>
#  include <sys/types.h>
#  include <sys/stat.h>
#  include <fcntl.h>
#  include <unistd.h>
#endif

#include <simgear/debug/logstream.hxx>

#include "serial.hxx"

SGSerialPort::SGSerialPort()
    : dev_open(false)
{
    // empty
}

SGSerialPort::SGSerialPort(const string& device, int baud) {
    open_port(device);
    
    if ( dev_open ) {
	set_baud(baud);
    }
}

SGSerialPort::~SGSerialPort() {
    // closing the port here screws us up because if we would even so
    // much as make a copy of an SGSerialPort object and then delete it,
    // the file descriptor gets closed.  Doh!!!
}

bool SGSerialPort::open_port(const string& device) {

#ifdef _WIN32

    fd = CreateFile( device.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0, // dwShareMode
        NULL, // lpSecurityAttributes
        OPEN_EXISTING,
        0,
        NULL );
    if ( fd == INVALID_HANDLE_VALUE )
    {
        LPVOID lpMsgBuf;
        FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | 
            FORMAT_MESSAGE_FROM_SYSTEM | 
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            GetLastError(),
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
            (LPTSTR) &lpMsgBuf,
            0,
            NULL );

        SG_LOG( SG_IO, SG_ALERT, "Error opening serial device \"" 
            << device << "\" " << (const char*) lpMsgBuf );
        LocalFree( lpMsgBuf );
        return false;
    }

    dev_open = true;
    return true;

#else

    struct termios config;

    fd = open(device.c_str(), O_RDWR | O_NOCTTY| O_NDELAY);
    SG_LOG( SG_EVENT, SG_DEBUG, "Serial fd created = " << fd);

    if ( fd  == -1 ) {
	SG_LOG( SG_IO, SG_ALERT, "Cannot open " << device
		<< " for serial I/O" );
	return false;
    } else {
	dev_open = true;
    }

    // set required port parameters 
    if ( tcgetattr( fd, &config ) != 0 ) {
	SG_LOG( SG_IO, SG_ALERT, "Unable to poll port settings" );
	return false;
    }

    // cfmakeraw( &config );

    // cout << "config.c_iflag = " << config.c_iflag << endl;

    // disable LF expanded to CR-LF
    config.c_oflag &= ~(ONLCR);

    // disable software flow control
    config.c_iflag &= ~(IXON | IXOFF | IXANY);

    // enable the receiver and set local mode
    config.c_cflag |= (CLOCAL | CREAD);

#if !defined( sgi ) && !defined(_AIX)
    // disable hardware flow control
    config.c_cflag &= ~(CRTSCTS);
#endif

    // cout << "config.c_iflag = " << config.c_iflag << endl;
    
    // Raw (not cooked/canonical) input mode
    config.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

    if ( tcsetattr( fd, TCSANOW, &config ) != 0 ) {
	SG_LOG( SG_IO, SG_ALERT, "Unable to update port settings" );
	return false;
    }

    return true;
#endif
}


bool SGSerialPort::close_port() {
#ifdef _WIN32
    CloseHandle( fd );
#else
    close(fd);
#endif

    dev_open = false;

    return true;
}


bool SGSerialPort::set_baud(int baud) {

#ifdef _WIN32

    DCB dcb;
    if ( GetCommState( fd, &dcb ) ) {
        dcb.BaudRate = baud;
        dcb.fOutxCtsFlow = FALSE;
        dcb.fOutxDsrFlow = FALSE;
        dcb.fOutX = TRUE;
        dcb.fInX = TRUE;

        if ( !SetCommState( fd, &dcb ) ) {
            LPVOID lpMsgBuf;
            FormatMessage(
                FORMAT_MESSAGE_ALLOCATE_BUFFER | 
                FORMAT_MESSAGE_FROM_SYSTEM | 
                FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                GetLastError(),
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                (LPTSTR) &lpMsgBuf,
                0,
                NULL );

            SG_LOG( SG_IO, SG_ALERT, "Unable to update port settings: " 
                << (const char*) lpMsgBuf );
            LocalFree( lpMsgBuf );
	    return false;
        }
    } else {
        LPVOID lpMsgBuf;
        FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | 
            FORMAT_MESSAGE_FROM_SYSTEM | 
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            GetLastError(),
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
            (LPTSTR) &lpMsgBuf,
            0,
            NULL );

        SG_LOG( SG_IO, SG_ALERT, "Unable to poll port settings: " 
             << (const char*) lpMsgBuf );
        LocalFree( lpMsgBuf );
	return false;
    }

    return true;

#else

    struct termios config;
    speed_t speed = B9600;

    if ( tcgetattr( fd, &config ) != 0 ) {
	SG_LOG( SG_IO, SG_ALERT, "Unable to poll port settings" );
	return false;
    }

    if ( baud == 300 ) {
	speed = B300;
    } else if ( baud == 1200 ) {
	speed = B1200;
    } else if ( baud == 2400 ) {
	speed = B2400;
    } else if ( baud == 4800 ) {
	speed = B4800;
    } else if ( baud == 9600 ) {
	speed = B9600;
    } else if ( baud == 19200 ) {
	speed = B19200;
    } else if ( baud == 38400 ) {
	speed = B38400;
#if defined( linux ) || defined( __FreeBSD__ )
    } else if ( baud == 57600 ) {
	speed = B57600;
    } else if ( baud == 115200 ) {
	speed = B115200;
    } else if ( baud == 230400 ) {
	speed = B230400;
#endif
    } else {
	SG_LOG( SG_IO, SG_ALERT, "Unsupported baud rate " << baud );
	return false;
    }

    if ( cfsetispeed( &config, speed ) != 0 ) {
	SG_LOG( SG_IO, SG_ALERT, "Problem setting input baud rate" );
	return false;
    }

    if ( cfsetospeed( &config, speed ) != 0 ) {
	SG_LOG( SG_IO, SG_ALERT, "Problem setting output baud rate" );
	return false;
    }

    if ( tcsetattr( fd, TCSANOW, &config ) != 0 ) {
	SG_LOG( SG_IO, SG_ALERT, "Unable to update port settings" );
	return false;
    }

    return true;

#endif

}

string SGSerialPort::read_port() {

    const int max_count = 1024;
    char buffer[max_count+1];
    string result;

#ifdef _WIN32

    DWORD count;
    if ( ReadFile( fd, buffer, max_count, &count, 0 ) ) {
	buffer[count] = '\0';
        result = buffer;
    } else {
        LPVOID lpMsgBuf;
        FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | 
            FORMAT_MESSAGE_FROM_SYSTEM | 
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            GetLastError(),
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
            (LPTSTR) &lpMsgBuf,
            0,
            NULL );

        SG_LOG( SG_IO, SG_ALERT, "Serial I/O read error: " 
             << (const char*) lpMsgBuf );
        LocalFree( lpMsgBuf );
    }

    return result;

#else

    int count = read(fd, buffer, max_count);
    // cout << "read " << count << " bytes" << endl;

    if ( count < 0 ) {
	// error condition
	if ( errno != EAGAIN ) {
	    SG_LOG( SG_IO, SG_ALERT, 
		    "Serial I/O on read, error number = " << errno );
	}

	return "";
    } else {
	buffer[count] = '\0';
	result = buffer;

	return result;
    }

#endif

}

int SGSerialPort::read_port(char *buf, int len) {

#ifdef _WIN32

    DWORD count;
    if ( ReadFile( fd, buf, len, &count, 0 ) ) {
	buf[count] = '\0';

        return count;
    } else {
        LPVOID lpMsgBuf;
        FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | 
            FORMAT_MESSAGE_FROM_SYSTEM | 
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            GetLastError(),
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
            (LPTSTR) &lpMsgBuf,
            0,
            NULL );

        SG_LOG( SG_IO, SG_ALERT, "Serial I/O read error: " 
             << (const char*) lpMsgBuf );
        LocalFree( lpMsgBuf );

	buf[0] = '\0';
	return 0;
    }

#else

    string result;

    int count = read(fd, buf, len);
    // cout << "read " << count << " bytes" << endl;

    if ( count < 0 ) {
	// error condition
	if ( errno != EAGAIN ) {
	    SG_LOG( SG_IO, SG_ALERT, 
		    "Serial I/O on read, error number = " << errno );
	}

	buf[0] = '\0';
	return 0;
    } else {
	buf[count] = '\0';

	return count;
    }

#endif

}


int SGSerialPort::write_port(const string& value) {

#ifdef _WIN32

    LPCVOID lpBuffer = value.data();
    DWORD nNumberOfBytesToWrite = value.length();
    DWORD lpNumberOfBytesWritten;

    if ( WriteFile( fd,
        lpBuffer,
        nNumberOfBytesToWrite,
        &lpNumberOfBytesWritten,
        0 ) == 0 )
    {
        LPVOID lpMsgBuf;
        FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | 
            FORMAT_MESSAGE_FROM_SYSTEM | 
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            GetLastError(),
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
            (LPTSTR) &lpMsgBuf,
            0,
            NULL );

        SG_LOG( SG_IO, SG_ALERT, "Serial I/O write error: " 
             << (const char*) lpMsgBuf );
        LocalFree( lpMsgBuf );
        return int(lpNumberOfBytesWritten);
    }

    return int(lpNumberOfBytesWritten);

#else

    static bool error = false;
    int count;

    if ( error ) {
        SG_LOG( SG_IO, SG_ALERT, "attempting serial write error recovery" );
	// attempt some sort of error recovery
	count = write(fd, "\n", 1);
	if ( count == 1 ) {
	    // cout << "Serial error recover successful!\n";
	    error = false;
	} else {
	    return 0;
	}
    }

    count = write(fd, value.c_str(), value.length());
    // cout << "write '" << value << "'  " << count << " bytes" << endl;

    if ( (int)count == (int)value.length() ) {
	error = false;
    } else {
	if ( errno == EAGAIN ) {
	    // ok ... in our context we don't really care if we can't
	    // write a string, we'll just get it the next time around
	    error = false;
	} else {
	    error = true;
	    SG_LOG( SG_IO, SG_ALERT,
		    "Serial I/O on write, error number = " << errno );
	}
    }

    return count;

#endif

}


int SGSerialPort::write_port(const char* buf, int len) {
#ifdef _WIN32

    LPCVOID lpBuffer = buf;
    DWORD nNumberOfBytesToWrite = len;
    DWORD lpNumberOfBytesWritten;

    if ( WriteFile( fd,
        lpBuffer,
        nNumberOfBytesToWrite,
        &lpNumberOfBytesWritten,
        0 ) == 0 )
    {
        LPVOID lpMsgBuf;
        FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | 
            FORMAT_MESSAGE_FROM_SYSTEM | 
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            GetLastError(),
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
            (LPTSTR) &lpMsgBuf,
            0,
            NULL );

        SG_LOG( SG_IO, SG_ALERT, "Serial I/O write error: " 
             << (const char*) lpMsgBuf );
        LocalFree( lpMsgBuf );
        return int(lpNumberOfBytesWritten);
    }

    return int(lpNumberOfBytesWritten);

#else

    static bool error = false;
    int count;

    if ( error ) {
	// attempt some sort of error recovery
	count = write(fd, "\n", 1);
	if ( count == 1 ) {
	    // cout << "Serial error recover successful!\n";
	    error = false;
	} else {
	    return 0;
	}
    }

    count = write(fd, buf, len);
    // cout << "write '" << buf << "'  " << count << " bytes" << endl;

    if ( (int)count == len ) {
	error = false;
    } else {
	error = true;
	if ( errno == EAGAIN ) {
	    // ok ... in our context we don't really care if we can't
	    // write a string, we'll just get it the next time around
	} else {
	    SG_LOG( SG_IO, SG_ALERT,
		    "Serial I/O on write, error number = " << errno );
	}
    }

    return count;

#endif

}
