/**
 * \file exception.hxx
 * Interface definition for SimGear base exceptions.
 * Started Spring 2001 by David Megginson, david@megginson.com
 * This code is released into the Public Domain.
 *
 * $Id$
 */

#ifndef __SIMGEAR_MISC_EXCEPTION_HXX
#define __SIMGEAR_MISC_EXCEPTION_HXX 1

#include <simgear/compiler.h>
#include <string>

using std::string;


/**
 * Information encapsulating a single location in an external resource
 *
 * A position in the resource my optionally be provided, either by
 * line number, line number and column number, or byte offset from the
 * beginning of the resource.
 */
class sg_location
{
public:
  sg_location ();
  sg_location (const string &path, int line = -1, int column = -1);
  virtual ~sg_location ();
  virtual const string &getPath () const;
  virtual void setPath (const string &path);
  virtual int getLine () const;
  virtual void setLine (int line);
  virtual int getColumn () const;
  virtual void setColumn (int column);
  virtual int getByte () const;
  virtual void setByte (int byte);
  virtual string asString () const;
private:
  string _path;
  int _line;
  int _column;
  int _byte;
};


/**
 * Abstract base class for all throwables.
 */
class sg_throwable
{
public:
  sg_throwable ();
  sg_throwable (const string &message, const string &origin = "");
  virtual ~sg_throwable ();
  virtual const string &getMessage () const;
  virtual const string getFormattedMessage () const;
  virtual void setMessage (const string &message);
  virtual const string &getOrigin () const;
  virtual void setOrigin (const string &origin);
private:
  string _message;
  string _origin;
};



/**
 * An unexpected fatal error.
 *
 * Methods and functions show throw this exception when something
 * very bad has happened (such as memory corruption or
 * a totally unexpected internal value).  Applications should catch
 * this exception only at the top level if at all, and should
 * normally terminate execution soon afterwards.
 */
class sg_error : public sg_throwable
{
public:
  sg_error ();
  sg_error (const string &message, const string &origin = "");
  virtual ~sg_error ();
};


/**
 * Base class for all SimGear exceptions.
 *
 * SimGear-based code should throw this exception only when no
 * more specific exception applies.  It may not be caught until
 * higher up in the application, where it is not possible to
 * resume normal operations if desired.
 *
 * A caller can catch sg_exception by default to ensure that
 * all exceptions are caught.  Every SimGear exception can contain
 * a human-readable error message and a human-readable string
 * indicating the part of the application causing the exception
 * (as an aid to debugging, only).
 */
class sg_exception : public sg_throwable
{
public:
  sg_exception ();
  sg_exception (const string &message, const string &origin = "");
  virtual ~sg_exception ();
};


/**
 * An I/O-related SimGear exception.
 *
 * SimGear-based code should throw this exception when it fails
 * to read from or write to an external resource, such as a file,
 * socket, URL, or database.
 *
 * In addition to the functionality of sg_exception, an
 * sg_io_exception may contain location information, such as the name
 * of a file or URL, and possible also a location in that file or URL.
 */
class sg_io_exception : public sg_exception
{
public:
  sg_io_exception ();
  sg_io_exception (const string &message, const string &origin = "");
  sg_io_exception (const string &message, const sg_location &location,
		   const string &origin = "");
  virtual ~sg_io_exception ();
  virtual const string getFormattedMessage () const;
  virtual const sg_location &getLocation () const;
  virtual void setLocation (const sg_location &location);
private:
  sg_location _location;
};


/**
 * A format-related SimGear exception.
 *
 * SimGear-based code should throw this exception when a string
 * does not appear in the expected format (for example, a date
 * string does not conform to ISO 8601).
 *
 * In addition to the functionality of sg_exception, an
 * sg_format_exception can contain a copy of the original malformated
 * text.
 */
class sg_format_exception : public sg_exception
{
public:
  sg_format_exception ();
  sg_format_exception (const string &message, const string &text,
		       const string &origin = "");
  virtual ~sg_format_exception ();
  virtual const string &getText () const;
  virtual void setText (const string &text);
private:
  string _text;
};


/**
 * A range-related SimGear exception.
 *
 * SimGear-based code should throw this exception when a value falls
 * outside the range where it can reasonably be handled; examples
 * include longitude outside the range -180:180, unrealistically high
 * forces or velocities, an illegal airport code, etc.  A range
 * exception usually means that something has gone wrong internally.
 */
class sg_range_exception : public sg_exception
{
public:
  sg_range_exception ();
  sg_range_exception (const string &message, const string &origin = "");
  virtual ~sg_range_exception ();
};

#endif

// end of exception.hxx
