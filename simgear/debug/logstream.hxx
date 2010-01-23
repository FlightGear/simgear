/** \file logstream.hxx
 * Stream based logging mechanism.
 */

// Written by Bernie Bright, 1998
//
// Copyright (C) 1998  Bernie Bright - bbright@c031.aone.net.au
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

#ifndef _LOGSTREAM_H
#define _LOGSTREAM_H

#include <simgear/compiler.h>

#ifdef _WIN32
#  include <windows.h>
#endif

#include <streambuf>
#include <ostream>
#include <cstdio>

#include <simgear/debug/debug_types.h>

using std::streambuf;
using std::ostream;

//
// TODO:
//
// 1. Change output destination. Done.
// 2. Make logbuf thread safe.
// 3. Read environment for default debugClass and debugPriority.
//

/**
 * logbuf is an output-only streambuf with the ability to disable sets of
 * messages at runtime. Only messages with priority >= logbuf::logPriority
 * and debugClass == logbuf::logClass are output.
 */
#ifdef SG_NEED_STREAMBUF_HACK
class logbuf : public __streambuf
#else
class logbuf : public std::streambuf
#endif
{
public:
    // logbuf( streambuf* sb ) : sbuf(sb) {}
    /** Constructor */
    logbuf();

    /** Destructor */
    ~logbuf();

    /**
     * Is logging enabled?
     * @return true or false*/
    bool enabled() { return logging_enabled; }

    /**
     * Set the logging level of subsequent messages.
     * @param c debug class
     * @param p priority
     */
    void set_log_state( sgDebugClass c, sgDebugPriority p );

    bool would_log( sgDebugClass c, sgDebugPriority p ) const;

    /**
     * Set the global logging level.
     * @param c debug class
     * @param p priority
     */
    static void set_log_level( sgDebugClass c, sgDebugPriority p );


    /**
     * Set the allowed logging classes.
     * @param c All enabled logging classes anded together.
     */
    static void set_log_classes (sgDebugClass c);


    /**
     * Get the logging classes currently enabled.
     * @return All enabled debug logging anded together.
     */
    static sgDebugClass get_log_classes ();


    /**
     * Set the logging priority.
     * @param c The priority cutoff for logging messages.
     */
    static void set_log_priority (sgDebugPriority p);


    /**
     * Get the current logging priority.
     * @return The priority cutoff for logging messages.
     */
    static sgDebugPriority get_log_priority ();


    /**
     * Set the stream buffer
     * @param sb stream buffer
     */
    void set_sb( std::streambuf* sb );

#ifdef _WIN32
    static void has_no_console() { has_console = false; }
#endif

protected:

    /** sync/flush */
    inline virtual int sync();

    /** overflow */
    int_type overflow( int ch );
    // int xsputn( const char* s, istreamsize n );

private:

    // The streambuf used for actual output. Defaults to cerr.rdbuf().
    static std::streambuf* sbuf;

    static bool logging_enabled;
#ifdef _WIN32
    static bool has_console;
#endif
    static sgDebugClass logClass;
    static sgDebugPriority logPriority;

private:

    // Not defined.
    logbuf( const logbuf& );
    void operator= ( const logbuf& );
};

inline int
logbuf::sync()
{
	return sbuf->pubsync();
}

inline void
logbuf::set_log_state( sgDebugClass c, sgDebugPriority p )
{
    logging_enabled = ((c & logClass) != 0 && p >= logPriority);
}

inline bool
logbuf::would_log( sgDebugClass c, sgDebugPriority p ) const
{
    return ((c & logClass) != 0 && p >= logPriority);
}

inline logbuf::int_type
logbuf::overflow( int c )
{
#ifdef _WIN32
    if ( logging_enabled ) {
        if ( !has_console ) {
            AllocConsole();
            freopen("conin$", "r", stdin);
            freopen("conout$", "w", stdout);
            freopen("conout$", "w", stderr);
            has_console = true;
        }
        return sbuf->sputc(c);
    }
    else
        return EOF == 0 ? 1: 0;
#else
    return logging_enabled ? sbuf->sputc(c) : (EOF == 0 ? 1: 0);
#endif
}

/**
 * logstream manipulator for setting the log level of a message.
 */
struct loglevel
{
    loglevel( sgDebugClass c, sgDebugPriority p )
	: logClass(c), logPriority(p) {}

    sgDebugClass logClass;
    sgDebugPriority logPriority;
};

/**
 * A helper class that ensures a streambuf and ostream are constructed and
 * destroyed in the correct order.  The streambuf must be created before the
 * ostream but bases are constructed before members.  Thus, making this class
 * a private base of logstream, declared to the left of ostream, we ensure the
 * correct order of construction and destruction.
 */
struct logstream_base
{
    // logstream_base( streambuf* sb ) : lbuf(sb) {}
    logstream_base() {}

    logbuf lbuf;
};

/**
 * Class to manage the debug logging stream.
 */
class logstream : private logstream_base, public std::ostream
{
public:
    /**
     * The default is to send messages to cerr.
     * @param out output stream
     */
    logstream( std::ostream& out )
	// : logstream_base(out.rdbuf()),
	: logstream_base(),
	  std::ostream(&lbuf) { lbuf.set_sb(out.rdbuf());}

    /**
     * Set the output stream
     * @param out output stream
     */
    void set_output( std::ostream& out ) { lbuf.set_sb( out.rdbuf() ); }

    /**
     * Set the global log class and priority level.
     * @param c debug class
     * @param p priority
     */
    void setLogLevels( sgDebugClass c, sgDebugPriority p );

    bool would_log(  sgDebugClass c, sgDebugPriority p ) const
    {
        return lbuf.would_log( c, p );
    };

    /**
     * Output operator to capture the debug level and priority of a message.
     * @param l log level
     */
    inline std::ostream& operator<< ( const loglevel& l );
    friend logstream& sglog();
    static logstream *initGlobalLogstream();
protected:
    static logstream *global_logstream;
};

inline std::ostream&
logstream::operator<< ( const loglevel& l )
{
    lbuf.set_log_state( l.logClass, l.logPriority );
    return *this;
}

/**
 * \relates logstream
 * Return the one and only logstream instance.
 * We use a function instead of a global object so we are assured that cerr
 * has been initialised.
 * @return current logstream
 */
inline logstream&
sglog()
{
    return *logstream::initGlobalLogstream();
}


/** \def SG_LOG(C,P,M)
 * Log a message.
 * @param C debug class
 * @param P priority
 * @param M message
 */
#ifdef FG_NDEBUG
# define SG_LOG(C,P,M)
#else
# define SG_LOG(C,P,M) do {                     \
	logstream& __tmplogstreamref(sglog());                      \
	if(__tmplogstreamref.would_log(C,P)) {                      \
        __tmplogstreamref << loglevel(C,P) << M << std::endl; } \
	} while(0)
#endif

#define SG_ORIGIN __FILE__ ":" SG_STRINGIZE(__LINE__)

#endif // _LOGSTREAM_H

