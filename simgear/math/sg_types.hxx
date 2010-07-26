/**
 * \file sg_types.hxx
 * Commonly used types I don't want to have to keep redefining.
 */

// Written by Curtis Olson, started March 1999.
//
// Copyright (C) 1999  Curtis L. Olson  - http://www.flightgear.org/~curt
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


#ifndef _SG_TYPES_HXX
#define _SG_TYPES_HXX


#ifndef __cplusplus
# error This library requires C++
#endif


#include <simgear/compiler.h>

#include <string>
#include <vector>

/** STL vector list of ints */
typedef std::vector < int > int_list;
typedef int_list::iterator int_list_iterator;
typedef int_list::const_iterator const_int_list_iterator;

/** STL vector list of doubles */
typedef std::vector < double > double_list;
typedef double_list::iterator double_list_iterator;
typedef double_list::const_iterator const_double_list_iterator;

/** STL vector list of strings */
typedef std::vector < std::string > string_list;
typedef string_list::iterator string_list_iterator;
typedef string_list::const_iterator const_string_list_iterator;

#endif // _SG_TYPES_HXX

