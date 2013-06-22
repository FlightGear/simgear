// tabbed_values.hxx -- parse tab separated strings into fields
//
// Written by James Turner, started February 2003.
//
// Copyright (C) 2003 James Turner
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
