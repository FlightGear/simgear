/**
 * \file strutils.hxx
 * String utilities.
 */

// Written by Bernie Bright, 1998
//
// Copyright (C) 1998  Bernie Bright - bbright@c031.aone.net.au
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
// License along with this library; if not, write to the
// Free Software Foundation, Inc., 59 Temple Place - Suite 330,
// Boston, MA  02111-1307, USA.
//
// $Id$


#ifndef STRUTILS_H
#define STRUTILS_H

#include <simgear/compiler.h>

#include STL_STRING

#ifdef SG_HAVE_STD_INCLUDES
#  include <cstdlib>
#else
#  include <stdlib.h>
#endif

SG_USING_STD(string);


/** Default characters to remove. */
extern const string whitespace;

/** Returns a string with trailing characters removed. */
string trimleft( const string& s, const string& trimmings = whitespace );

/** Returns a string with leading characters removed. */
string trimright( const string& s, const string& trimmings = whitespace );

/** Returns a string with leading and trailing characters removed. */
string trim( const string& s, const string& trimmings = whitespace );

/** atof() wrapper for "string" type */
inline double
atof( const string& str )
{
    return ::atof( str.c_str() );
}

/** atoi() wrapper for "string" type */
inline int
atoi( const string& str )
{
    return ::atoi( str.c_str() );
}

#endif // STRUTILS_H

