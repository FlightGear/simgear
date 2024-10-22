// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2003 James Turner

/**
 * @file
 * @brief parse tab separated strings into fields
 */

// $Id$

#ifndef SG_TABBED_VALUES_HXX
#define SG_TABBED_VALUES_HXX

#include <simgear/compiler.h>

#include <vector>
#include <string>

using std::vector;
using std::string;

class SGTabbedValues
{
public:
	SGTabbedValues(const char* line);
	
	string operator[](const unsigned int) const;

	bool isValueAt(const unsigned int) const;
	
	double getDoubleAt(const unsigned int) const;
	char getCharAt(const unsigned int) const;
	long getLongAt(const unsigned int) const;
private:
	const char* fieldAt(const unsigned int offset) const;
	
	/** this is first character of each field, if the field is empty
	it will be the tab character. It is lazily built as needed, so
	if only the first field is accessed (which is a common case) we
	don't iterative over the whole line. */
	mutable vector<char*> _fields;
};

#endif
