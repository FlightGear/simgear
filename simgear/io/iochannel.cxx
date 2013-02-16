// iochannel.cxx -- High level IO channel class
//
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


#include "iochannel.hxx"


// constructor
SGIOChannel::SGIOChannel()
{
}


// destructor
SGIOChannel::~SGIOChannel()
{
}


// dummy configure routine
bool SGIOChannel::open( const SGProtocolDir d ) {
    return false;
}


// dummy process routine
int SGIOChannel::read( char *buf, int length ) {
    return 0;
}


// dummy process routine
int SGIOChannel::readline( char *buf, int length ) {
    return 0;
}


// dummy process routine
int SGIOChannel::write( const char *buf, const int length ) {
    return false;
}


// dummy process routine
int SGIOChannel::writestring( const char *str ) {
    return false;
}


// dummy close routine
bool SGIOChannel::close() {
    return false;
}


// dummy eof routine
bool SGIOChannel::eof() const {
    return false;
}
