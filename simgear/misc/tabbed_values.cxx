#include "tabbed_values.hxx"

SGTabbedValues::SGTabbedValues(const char *line) :
	_line(line)
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
	return strtod(data, NULL);
}

long SGTabbedValues::getLongAt(const unsigned int offset) const
{
	const char *data = fieldAt(offset);
	if (!data || (*data == '\t'))
		return 0;

	return strtol(data, NULL, 0);
}
