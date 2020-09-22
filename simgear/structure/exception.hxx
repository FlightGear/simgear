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

#include <exception>
#include <functional>
#include <simgear/compiler.h>
#include <string>

class SGPath;

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
  enum {max_path = 1024};
  sg_location ();
  sg_location(const std::string& path, int line = -1, int column = -1);
  sg_location(const SGPath& path, int line = -1, int column = -1);
  explicit sg_location(const char* path, int line = -1, int column = -1);

  ~sg_location() = default;

  const char *getPath() const;
  int getLine() const;
  int getColumn() const;
  int getByte() const;

  std::string asString() const;
  bool isValid() const;

private:
  void setPath(const char *p);

  char _path[max_path];
  int _line;
  int _column;
  int _byte;
};


/**
 * Abstract base class for all throwables.
 */
class sg_throwable : public std::exception
{
public:
  enum {MAX_TEXT_LEN = 1024};
  sg_throwable ();
  sg_throwable(const char *message, const char *origin = 0,
               const sg_location &loc = {});

  virtual ~sg_throwable ();
  virtual const char* getMessage () const;
  virtual const std::string getFormattedMessage () const;
  virtual void setMessage (const char* message);
  virtual const char* getOrigin () const;
  virtual void setOrigin (const char *origin);
  virtual const char* what() const noexcept;
private:
  char _message[MAX_TEXT_LEN];
  char _origin[MAX_TEXT_LEN];
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
  sg_error (const char* message, const char* origin = 0);
  sg_error(const std::string &message, const std::string &origin = {});
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
  sg_exception(const char *message, const char *origin = 0,
               const sg_location &loc = {});
  sg_exception(const std::string &message, const std::string & = {},
               const sg_location &loc = {});
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
  sg_io_exception (const char* message, const char* origin = 0);
  sg_io_exception (const char* message, const sg_location &location,
		   const char* origin = 0);
  sg_io_exception (const std::string &message, const std::string &origin = "");
  sg_io_exception (const std::string &message, const sg_location &location, 
    const std::string &origin = "");

  virtual ~sg_io_exception ();
  virtual const std::string getFormattedMessage () const;
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
  sg_format_exception (const char* message, const char* text,
		       const char* origin = 0);
  sg_format_exception (const std::string& message, const std::string& text,
		       const std::string& origin = "");
  virtual ~sg_format_exception ();
  virtual const char* getText () const;
  virtual void setText (const char* text);
private:
  char _text[MAX_TEXT_LEN];
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
  sg_range_exception (const char* message,
                      const char* origin = 0);
  sg_range_exception (const std::string& message,
                      const std::string& origin = "");
  virtual ~sg_range_exception ();
};

using ThrowCallback = std::function<void(
    const char *message, const char *origin, const sg_location &loc)>;

/**
 * @brief Specify a callback to be invoked when an exception is created.
 *
 * This is used to capture a stack-trace for our crash/error reporting system,
 * if a callback is defined
 */
void setThrowCallback(ThrowCallback cb);

#endif

// end of exception.hxx
