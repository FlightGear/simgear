// exception.cxx - implementation of SimGear base exceptions.
// Started Summer 2001 by David Megginson, david@megginson.com
// This code is released into the Public Domain.
//
// $Id$

#include <simgear_config.h>
#include "exception.hxx"

#include <stdio.h>
#include <cstring>
#include <sstream>

#include <simgear/misc/sg_path.hxx>

static ThrowCallback static_callback;

////////////////////////////////////////////////////////////////////////
// Implementation of sg_location class.
////////////////////////////////////////////////////////////////////////

sg_location::sg_location() noexcept
    : _line(-1),
      _column(-1),
      _byte(-1)
{
    _path[0] = '\0';
}

sg_location::sg_location(const std::string& path, int line, int column) noexcept
    : _line(line),
      _column(column),
      _byte(-1)
{
  setPath(path.c_str());
}

sg_location::sg_location(const SGPath& path, int line, int column) noexcept
    : _line(line),
      _column(column),
      _byte(-1)
{
    setPath(path.utf8Str().c_str());
}

sg_location::sg_location(const char* path, int line, int column) noexcept
    : _line(line),
      _column(column),
      _byte(-1)
{
  setPath(path);
}

void sg_location::setPath(const char* path) noexcept
{
    if (path) {
        strncpy(_path, path, max_path);
        _path[max_path - 1] = '\0';
    } else {
        _path[0] = '\0';
    }
}

const char* sg_location::getPath() const noexcept { return _path; }

int sg_location::getLine() const noexcept { return _line; }

int sg_location::getColumn() const noexcept
{
  return _column;
}


int sg_location::getByte() const noexcept
{
  return _byte;
}

std::string
sg_location::asString() const noexcept
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

bool sg_location::isValid() const noexcept { return strlen(_path) > 0; }

////////////////////////////////////////////////////////////////////////
// Implementation of sg_throwable class.
////////////////////////////////////////////////////////////////////////

sg_throwable::sg_throwable() noexcept
{
  _message[0] = '\0';
  _origin[0] = '\0';
}

sg_throwable::sg_throwable(const char* message, const char* origin,
                           const sg_location& loc, bool report) noexcept : _location(loc)
{
    setMessage(message);
    setOrigin(origin);

    if (static_callback && report) {
        static_callback(_message, _origin, loc);
    }
}

const char*
sg_throwable::getMessage() const noexcept
{
  return _message;
}

std::string
sg_throwable::getFormattedMessage() const noexcept
{
    std::string ret = getMessage();
    const std::string loc = getLocation().asString();
    if (loc.length()) {
        ret += "\n at ";
        ret += loc;
    }
    return ret;
}

void sg_throwable::setMessage(const char* message) noexcept
{
  strncpy(_message, message, MAX_TEXT_LEN);
  _message[MAX_TEXT_LEN - 1] = '\0';

}

const char*
sg_throwable::getOrigin() const noexcept
{
  return _origin;
}

void sg_throwable::setOrigin(const char* origin) noexcept
{
  if (origin) {
    strncpy(_origin, origin, MAX_TEXT_LEN);
    _origin[MAX_TEXT_LEN - 1] = '\0';
  } else {
    _origin[0] = '\0';
  }
}

const char* sg_throwable::what() const noexcept
{
  try {
    return getMessage();
  }
  catch (...) {
    return "";
  }
}

sg_location sg_throwable::getLocation() const noexcept
{
    return _location;
}

void sg_throwable::setLocation(const sg_location& location) noexcept
{
    _location = location;
}

////////////////////////////////////////////////////////////////////////
// Implementation of sg_error class.
////////////////////////////////////////////////////////////////////////

sg_error::sg_error(const char* message, const char* origin, bool report) noexcept
    : sg_throwable(message, origin, {}, report)
{
}

sg_error::sg_error(const std::string& message, const std::string& origin, bool report) noexcept
    : sg_throwable(message.c_str(), origin.c_str(), {}, report)
{
}

////////////////////////////////////////////////////////////////////////
// Implementation of sg_exception class.
////////////////////////////////////////////////////////////////////////

sg_exception::sg_exception(const char* message, const char* origin,
                           const sg_location& loc, bool report) noexcept
    : sg_throwable(message, origin, loc, report) {}

sg_exception::sg_exception(const std::string& message,
                           const std::string& origin, const sg_location& loc,
                           bool report) noexcept
    : sg_throwable(message.c_str(), origin.c_str(), loc, report) {}


////////////////////////////////////////////////////////////////////////
// Implementation of sg_io_exception.
////////////////////////////////////////////////////////////////////////

sg_io_exception::sg_io_exception(const char* message, const char* origin, bool report)
    : sg_exception(message, origin, {}, report)
{
}

sg_io_exception::sg_io_exception(const char* message,
                                 const sg_location& location,
                                 const char* origin,
                                 bool report)
    : sg_exception(message, origin, location, report) {}

sg_io_exception::sg_io_exception(const std::string& message,
                                 const std::string& origin,
                                 bool report)
    : sg_exception(message, origin, {}, report)
{
}

sg_io_exception::sg_io_exception(const std::string& message,
                                 const sg_location& location,
                                 const std::string& origin,
                                 bool report)
    : sg_exception(message, origin, location, report) {}


////////////////////////////////////////////////////////////////////////
// Implementation of sg_format_exception.
////////////////////////////////////////////////////////////////////////

sg_format_exception::sg_format_exception() noexcept
    : sg_exception()
{
  _text[0] = '\0';
}

sg_format_exception::sg_format_exception(const char* message,
                                         const char* text,
                                         const char* origin,
                                         bool report)
    : sg_exception(message, origin, {}, report)
{
  setText(text);
}

sg_format_exception::sg_format_exception(const std::string& message,
                                         const std::string& text,
                                         const std::string& origin,
                                         bool report)
    : sg_exception(message, origin, {}, report)
{
  setText(text.c_str());
}


const char*
sg_format_exception::getText() const noexcept
{
  return _text;
}

void sg_format_exception::setText(const char* text) noexcept
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

sg_range_exception::sg_range_exception(const char* message,
                                       const char* origin, bool report)
    : sg_exception(message, origin, {}, report)
{
}

sg_range_exception::sg_range_exception(const std::string& message,
                                       const std::string& origin,
                                       bool report)
    : sg_exception(message, origin, {}, report)
{
}


////////////////////////////////////////////////////////////////////////

void setThrowCallback(ThrowCallback cb) { static_callback = cb; }

// end of exception.cxx
