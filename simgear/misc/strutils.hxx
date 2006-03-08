/**
 * \file strutils.hxx
 * String utilities.
 */

// Written by Bernie Bright, started 1998
//
// Copyright (C) 1998  Bernie Bright - bbright@bigpond.net.au
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


#ifndef STRUTILS_H
#define STRUTILS_H

#include <simgear/compiler.h>

#include STL_STRING

#include <vector>
SG_USING_STD(vector);

#ifdef SG_HAVE_STD_INCLUDES
#  include <cstdlib>
#else
#  include <stdlib.h>
#endif

SG_USING_STD(string);

namespace simgear {
    namespace strutils {

// 	/** 
// 	 * atof() wrapper for "string" type
// 	 */
// 	inline double
// 	atof( const string& str )
// 	{
// 	    return ::atof( str.c_str() );
// 	}

// 	/**
// 	 * atoi() wrapper for "string" type
// 	 */
// 	inline int
// 	atoi( const string& str )
// 	{
// 	    return ::atoi( str.c_str() );
// 	}

	/**
	 * Strip leading and/or trailing whitespace from s.
	 * @param s String to strip.
	 * @return The stripped string.
	 */
	string lstrip( const string& s );
	string rstrip( const string& s );
	string strip( const string& s );

	/**
	 * Split a string into a words using 'sep' as the delimiter string.
	 * Produces a result similar to the perl and python functions of the
	 * same name.
	 * 
	 * @param s The string to split into words,
	 * @param sep Word delimiters.  If not specified then any whitespace is a separator,
	 * @param maxsplit If given, splits at no more than maxsplit places,
	 * resulting in at most maxsplit+1 words.
	 * @return Array of words.
	 */
	vector<string>
	split( const string& s,
	       const char* sep = 0,
	       int maxsplit = 0 );

    } // end namespace strutils
} // end namespace simgear

#endif // STRUTILS_H

