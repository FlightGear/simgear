// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 1999 Curtis L. Olson - http://www.flightgear.org/~curt

/**
 * @file
 * @brief High level IO channel class
 */

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
