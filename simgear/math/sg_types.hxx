// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 1999 Curtis L. Olson - http://www.flightgear.org/~curt

/**
 * @file
 * @brief Commonly used types
 */

#ifndef _SG_TYPES_HXX
#define _SG_TYPES_HXX


#ifndef __cplusplus
# error This library requires C++
#endif


#include <simgear/compiler.h>

#include <string>
#include <vector>

/** STL vector list of strings */
typedef std::vector < std::string > string_list;
typedef string_list::iterator string_list_iterator;
typedef string_list::const_iterator const_string_list_iterator;

#endif // _SG_TYPES_HXX

