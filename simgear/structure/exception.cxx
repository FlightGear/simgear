// exception.cxx - implementation of SimGear base exceptions.
// Started Summer 2001 by David Megginson, david@megginson.com
// This code is released into the Public Domain.
//
// $Id$


#include "exception.hxx"

#include <stdio.h>
#include <cstring>
#include <sstream>

#include <simgear/misc/sg_path.hxx>

////////////////////////////////////////////////////////////////////////
// Implementation of sg_location class.
////////////////////////////////////////////////////////////////////////

sg_location::sg_location ()
  : _line(-1),
    _column(-1),
    _byte(-1)
{
    _path[0] = '\0';
}

sg_location::sg_location (const std::string& path, int line, int column)
  : _line(line),
    _column(column),
    _byte(-1)
{
  setPath(path.c_str());
}

sg_location::sg_location (const SGPath& path, int line, int column)
: _line(line),
_column(column),
_byte(-1)
{
    setPath(path.utf8Str().c_str());
}

sg_location::sg_location (const char* path, int line, int column)
  : _line(line),
    _column(column),
    _byte(-1)
{
  setPath(path);
}

sg_location::~sg_location () throw ()
{
}

const char*
sg_location::getPath () const
{
  return _path;
}

void
sg_location::setPath (const char* path)
{
  if (path) {
    strncpy(_path, path, max_path);
    _path[max_path -1] = '\0';
  } else {
    _path[0] = '\0';
  }
}

int
sg_location::getLine () const
{
  return _line;
}

void
sg_location::setLine (int line)
{
  _line = line;
}

int
sg_location::getColumn () const
{
  return _column;
}

void
sg_location::setColumn (int column)
{
  _column = column;
}

int
sg_location::getByte () const
{
  return _byte;
}

void
sg_location::setByte (int byte)
{
  _byte = byte;
}

std::string
sg_location::asString () const
{
  std::ostringstream out;
  if (_path[0]) {
    out << _path;
    if (_line != -1 || _column != -1)
      out << ",\n";
  }
  if (_line != -1) {
    out << "line " << _line;
    if (_column != -1)
      out << ", ";
  }
  if (_column != -1) {
    out << "column " << _column;
  }
  return out.str();
}



////////////////////////////////////////////////////////////////////////
// Implementation of sg_throwable class.
////////////////////////////////////////////////////////////////////////

sg_throwable::sg_throwable ()
{
  _message[0] = '\0';
  _origin[0] = '\0';
}

sg_throwable::sg_throwable (const char* message, const char* origin)
{
  setMessage(message);
  setOrigin(origin);
}

sg_throwable::~sg_throwable () throw ()
{
}

const char*
sg_throwable::getMessage () const
{
  return _message;
}

const std::string
sg_throwable::getFormattedMessage () const
{
  return std::string(getMessage());
}

void
sg_throwable::setMessage (const char* message)
{
  strncpy(_message, message, MAX_TEXT_LEN);
  _message[MAX_TEXT_LEN - 1] = '\0';

}

const char*
sg_throwable::getOrigin () const
{
  return _origin;
}

void
sg_throwable::setOrigin (const char* origin)
{
  if (origin) {
    strncpy(_origin, origin, MAX_TEXT_LEN);
    _origin[MAX_TEXT_LEN - 1] = '\0';
  } else {
    _origin[0] = '\0';
  }
}

const char* sg_throwable::what() const throw()
{
  try {
    return getMessage();
  }
  catch (...) {
    return "";
  }
}


////////////////////////////////////////////////////////////////////////
// Implementation of sg_error class.
////////////////////////////////////////////////////////////////////////

sg_error::sg_error ()
  : sg_throwable ()
{
}

sg_error::sg_error (const char* message, const char *origin)
  : sg_throwable(message, origin)
{
}

sg_error::sg_error(const std::string& message, const std::string& origin)
  : sg_throwable(message.c_str(), origin.c_str())
{
}

sg_error::~sg_error () throw ()
{
}

////////////////////////////////////////////////////////////////////////
// Implementation of sg_exception class.
////////////////////////////////////////////////////////////////////////

sg_exception::sg_exception ()
  : sg_throwable ()
{
}

sg_exception::sg_exception (const char* message, const char* origin)
  : sg_throwable(message, origin)
{
}

sg_exception::sg_exception( const std::string& message,
                            const std::string& origin )
  : sg_throwable(message.c_str(), origin.c_str())
{
}

sg_exception::~sg_exception () throw ()
{
}

////////////////////////////////////////////////////////////////////////
// Implementation of sg_io_exception.
////////////////////////////////////////////////////////////////////////

sg_io_exception::sg_io_exception ()
  : sg_exception()
{
}

sg_io_exception::sg_io_exception (const char* message, const char* origin)
  : sg_exception(message, origin)
{
}

sg_io_exception::sg_io_exception (const char* message,
				  const sg_location &location,
				  const char* origin)
  : sg_exception(message, origin),
    _location(location)
{
}

sg_io_exception::sg_io_exception( const std::string& message,
                                  const std::string& origin )
  : sg_exception(message, origin)
{
}

sg_io_exception::sg_io_exception( const std::string& message,
                                  const sg_location &location,
                                  const std::string& origin )
  : sg_exception(message, origin),
    _location(location)
{
}

sg_io_exception::~sg_io_exception () throw ()
{
}

const std::string
sg_io_exception::getFormattedMessage () const
{
  std::string ret = getMessage();
  std::string loc = getLocation().asString();
  if (loc.length()) {
    ret += "\n at ";
    ret += loc;
  }
  return ret;
}

const sg_location &
sg_io_exception::getLocation () const
{
  return _location;
}

void
sg_io_exception::setLocation (const sg_location &location)
{
  _location = location;
}



////////////////////////////////////////////////////////////////////////
// Implementation of sg_format_exception.
////////////////////////////////////////////////////////////////////////

sg_format_exception::sg_format_exception ()
  : sg_exception()
{
  _text[0] = '\0';
}

sg_format_exception::sg_format_exception (const char* message,
					  const char* text,
					  const char* origin)
  : sg_exception(message, origin)
{
  setText(text);
}

sg_format_exception::sg_format_exception( const std::string& message,
                                          const std::string& text,
                                          const std::string& origin )
  : sg_exception(message, origin)
{
  setText(text.c_str());
}

sg_format_exception::~sg_format_exception () throw ()
{
}

const char*
sg_format_exception::getText () const
{
  return _text;
}

void
sg_format_exception::setText (const char* text)
{
  if (text) {
    strncpy(_text, text, MAX_TEXT_LEN);
    _text[MAX_TEXT_LEN-1] = '\0';
  } else {
    _text[0] = '\0';
  }
}



////////////////////////////////////////////////////////////////////////
// Implementation of sg_range_exception.
////////////////////////////////////////////////////////////////////////

sg_range_exception::sg_range_exception ()
  : sg_exception()
{
}

sg_range_exception::sg_range_exception (const char* message,
					const char* origin)
  : sg_exception(message, origin)
{
}

sg_range_exception::sg_range_exception(const std::string& message,
                                       const std::string& origin)
  : sg_exception(message, origin)
{
}

sg_range_exception::~sg_range_exception () throw ()
{
}
// end of exception.cxx
