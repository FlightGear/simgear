// exception.cxx - implementation of SimGear base exceptions.
// Started Summer 2001 by David Megginson, david@megginson.com
// This code is released into the Public Domain.
//
// $Id$


#include "exception.hxx"
#include <stdio.h>


////////////////////////////////////////////////////////////////////////
// Implementation of sg_location class.
////////////////////////////////////////////////////////////////////////

sg_location::sg_location ()
  : _path(""),
    _line(-1),
    _column(-1),
    _byte(-1)
{
}

sg_location::sg_location (const string &path, int line, int column)
  : _path(path),
    _line(line),
    _column(column),
    _byte(-1)
{
}

sg_location::~sg_location ()
{
}

const string &
sg_location::getPath () const
{
  return _path;
}

void
sg_location::setPath (const string &path)
{
  _path = path;
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

string
sg_location::asString () const
{
  char buf[128];
  string out = "";
  if (_path != "") {
    out += _path;
    if (_line != -1 || _column != -1)
      out += ",\n";
  }
  if (_line != -1) {
    sprintf(buf, "line %d", _line);
    out += buf;
    if (_column != -1)
      out += ",\n";
  }
  if (_column != -1) {
    sprintf(buf, "column %d", _column);
    out += buf;
  }
  return out;
    
}



////////////////////////////////////////////////////////////////////////
// Implementation of sg_throwable class.
////////////////////////////////////////////////////////////////////////

sg_throwable::sg_throwable ()
  : _message(""),
    _origin("")
{
}

sg_throwable::sg_throwable (const string &message, const string &origin)
  : _message(message),
    _origin(origin)
{
}

sg_throwable::~sg_throwable ()
{
}

const string &
sg_throwable::getMessage () const
{
  return _message;
}

void
sg_throwable::setMessage (const string &message)
{
  _message = message;
}

const string &
sg_throwable::getOrigin () const
{
  return _origin;
}

void
sg_throwable::setOrigin (const string &origin)
{
  _origin = origin;
}


sg_throwable *
sg_throwable::clone () const
{
  return new sg_throwable(getMessage(), getOrigin());
}




////////////////////////////////////////////////////////////////////////
// Implementation of sg_error class.
////////////////////////////////////////////////////////////////////////

sg_error::sg_error ()
  : sg_throwable ()
{
}

sg_error::sg_error (const string &message, const string &origin)
  : sg_throwable(message, origin)
{
}

sg_error::~sg_error ()
{
}

sg_error *
sg_error::clone () const
{
  return new sg_error(getMessage(), getOrigin());
}



////////////////////////////////////////////////////////////////////////
// Implementation of sg_exception class.
////////////////////////////////////////////////////////////////////////

sg_exception::sg_exception ()
  : sg_throwable ()
{
}

sg_exception::sg_exception (const string &message, const string &origin)
  : sg_throwable(message, origin)
{
}

sg_exception::~sg_exception ()
{
}

sg_exception *
sg_exception::clone () const
{
  return new sg_exception(getMessage(), getOrigin());
}



////////////////////////////////////////////////////////////////////////
// Implementation of sg_io_exception.
////////////////////////////////////////////////////////////////////////

sg_io_exception::sg_io_exception ()
  : sg_exception()
{
}

sg_io_exception::sg_io_exception (const string &message, const string &origin)
  : sg_exception(message, origin)
{
}

sg_io_exception::sg_io_exception (const string &message,
				  const sg_location &location,
				  const string &origin)
  : sg_exception(message, origin),
    _location(location)
{
}

sg_io_exception::~sg_io_exception ()
{
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

sg_io_exception *
sg_io_exception::clone () const
{
  return new sg_io_exception(getMessage(), getLocation(), getOrigin());
}




////////////////////////////////////////////////////////////////////////
// Implementation of sg_format_exception.
////////////////////////////////////////////////////////////////////////

sg_format_exception::sg_format_exception ()
  : sg_exception(),
    _text("")
{
}

sg_format_exception::sg_format_exception (const string &message,
					  const string &text,
					  const string &origin)
  : sg_exception(message, origin),
    _text(text)
{
}

sg_format_exception::~sg_format_exception ()
{
}

const string &
sg_format_exception::getText () const
{
  return _text;
}

void
sg_format_exception::setText (const string &text)
{
  _text = text;
}

sg_format_exception *
sg_format_exception::clone () const
{
  return new sg_format_exception(getMessage(), getText(), getOrigin());
}



////////////////////////////////////////////////////////////////////////
// Implementation of sg_range_exception.
////////////////////////////////////////////////////////////////////////

sg_range_exception::sg_range_exception ()
  : sg_exception()
{
}

sg_range_exception::sg_range_exception (const string &message,
					const string &origin)
  : sg_exception(message, origin)
{
}

sg_range_exception::~sg_range_exception ()
{
}

sg_range_exception *
sg_range_exception::clone () const
{
  return new sg_range_exception(getMessage(), getOrigin());
}


// end of exception.cxx
