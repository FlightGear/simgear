// iochannel.hxx -- High level IO channel class
//
// Written by Curtis Olson, started November 1999.
//
// Copyright (C) 1999  Curtis L. Olson - curt@flightgear.org
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
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// $Id$


#ifndef _IOCHANNEL_HXX
#define _IOCHANNEL_HXX


#include <simgear/compiler.h>

// #include "protocol.hxx"

#include STL_STRING
#include <vector>

SG_USING_STD(vector);
SG_USING_STD(string);


#define SG_IO_MAX_MSG_SIZE 16384

enum SGProtocolDir {
    SG_IO_NONE = 0,
    SG_IO_IN = 1,
    SG_IO_OUT = 2,
    SG_IO_BI = 3
};


enum SGChannelType {
    sgFileType = 0,
    sgSerialType = 1,
    sgSocketType = 2
};

class SGIOChannel {

    SGChannelType type;
    SGProtocolDir dir;
    bool valid;

public:

    SGIOChannel();
    virtual ~SGIOChannel();

    virtual bool open( const SGProtocolDir d );
    virtual int read( char *buf, int length );
    virtual int readline( char *buf, int length );
    virtual int write( const char *buf, const int length );
    virtual int writestring( const char *str );
    virtual bool close();

    inline void set_type( SGChannelType t ) { type = t; }
    inline SGChannelType get_type() const { return type; }

    inline void set_dir( const SGProtocolDir d ) { dir = d; }
    inline SGProtocolDir get_dir() const { return dir; }
    inline bool isvalid() const { return valid; }
};


#endif // _IOCHANNEL_HXX


