// tabbed_values.cxx -- parse tab separated strings into fields
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

#include <cstdlib>
#include <assert.h>

#include "tabbed_values.hxx"


SGTabbedValues::SGTabbedValues(const char *line)
{
	assert(line);
	_fields.push_back(const_cast<char*>(line));
}

const char* SGTabbedValues::fieldAt(const unsigned int index) const
{
	// we already computed that offset, cool
	if (_fields.size() > index)
		return _fields[index];

	while (_fields.size() <= index) {
		char* nextField = _fields.back();
		if (*nextField=='\0') return NULL; // we went off the end
			
		while (*nextField != '\t') {
			if (*nextField=='\0') return NULL; // we went off the end
			++nextField;
		}
		_fields.push_back(++nextField);
	}
	
	return _fields.back();
}

string SGTabbedValues::operator[](const unsigned int offset) const
{
	const char *data = fieldAt(offset);
	char* endPtr = const_cast<char*>(data);
	int len = 0;
	while ((*endPtr != '\0') && (*endPtr != '\t')) {
		++len;
		++endPtr;
	}
	return string(fieldAt(offset), len);
}

bool SGTabbedValues::isValueAt(const unsigned int offset) const
{
	const char *data = fieldAt(offset);
	return data && (*data != '\t'); // must be non-NULL and non-tab
}

char SGTabbedValues::getCharAt(const unsigned int offset) const
{
	const char *data = fieldAt(offset);
	if (!data || (*data == '\t'))
		return 0;
	
	return *data;
}

double SGTabbedValues::getDoubleAt(const unsigned int offset) const
{
	const char *data = fieldAt(offset);
	if (!data || (*data == '\t'))
		return 0;
		
	/* this is safe because strtod will stop parsing when it sees an unrecogznied
	character, which includes tab. */	
	return std::strtod(data, NULL);
}

long SGTabbedValues::getLongAt(const unsigned int offset) const
{
	const char *data = fieldAt(offset);
	if (!data || (*data == '\t'))
		return 0;

	return std::strtol(data, NULL, 0);
}
