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
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the
// Free Software Foundation, Inc., 59 Temple Place - Suite 330,
// Boston, MA  02111-1307, USA.
//
// $Id$

#ifndef _LOGSTREAM_H
#define _LOGSTREAM_H

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/compiler.h>

// At least Irix needs this
#ifdef SG_HAVE_NATIVE_SGI_COMPILERS
#include <char_traits.h>
#endif

#ifdef SG_HAVE_STD_INCLUDES
# include <streambuf>
# include <iostream>
#else
# include <iostream.h>
# include <simgear/sg_traits.hxx>
#endif

#include <simgear/debug/debug_types.h>

#ifndef SG_HAVE_NATIVE_SGI_COMPILERS
SG_USING_STD(streambuf);
SG_USING_STD(ostream);
SG_USING_STD(cerr);
SG_USING_STD(endl);
#else
SG_USING_STD(char_traits);
#endif

#ifdef __MWERKS__
SG_USING_STD(iostream);
#endif

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
class logbuf : public streambuf
{
public:

#ifndef SG_HAVE_STD_INCLUDES
    typedef char_traits<char>           traits_type;
    typedef char_traits<char>::int_type int_type;
    // typedef char_traits<char>::pos_type pos_type;
    // typedef char_traits<char>::off_type off_type;
#endif
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

    /**
     * Set the global logging level.
     * @param c debug class
     * @param p priority
     */
    static void set_log_level( sgDebugClass c, sgDebugPriority p );

    /**
     * Set the stream buffer
     * @param sb stream buffer
     */
    void set_sb( streambuf* sb );

protected:

    /** sync/flush */
    inline virtual int sync();

    /** overflow */
    int_type overflow( int ch );
    // int xsputn( const char* s, istreamsize n );

private:

    // The streambuf used for actual output. Defaults to cerr.rdbuf().
    static streambuf* sbuf;

    static bool logging_enabled;
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
#ifdef SG_HAVE_STD_INCLUDES
	return sbuf->pubsync();
#else
	return sbuf->sync();
#endif
}

inline void
logbuf::set_log_state( sgDebugClass c, sgDebugPriority p )
{
    logging_enabled = ((c & logClass) != 0 && p >= logPriority);
}

inline logbuf::int_type
logbuf::overflow( int c )
{
    return logging_enabled ? sbuf->sputc(c) : (EOF == 0 ? 1: 0);
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
class logstream : private logstream_base, public ostream
{
public:
    /**
     * The default is to send messages to cerr.
     * @param out output stream
     */
    logstream( ostream& out )
	// : logstream_base(out.rdbuf()),
	: logstream_base(),
	  ostream(&lbuf) { lbuf.set_sb(out.rdbuf());}

    /**
     * Set the output stream
     * @param out output stream
     */
    void set_output( ostream& out ) { lbuf.set_sb( out.rdbuf() ); }

    /**
     * Set the global log class and priority level.
     * @param c debug class
     * @param p priority
     */
    void setLogLevels( sgDebugClass c, sgDebugPriority p );

    /**
     * Output operator to capture the debug level and priority of a message.
     * @param l log level
     */
    inline ostream& operator<< ( const loglevel& l );
};

inline ostream&
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
    static logstream logstrm( cerr );
    return logstrm;
}


/** \def SG_LOG(C,P,M)
 * Log a message.
 * @param C debug class
 * @param P priority
 * @param M message
 */
#ifdef FG_NDEBUG
# define SG_LOG(C,P,M)
#elif defined( __MWERKS__ )
# define SG_LOG(C,P,M) ::sglog() << ::loglevel(C,P) << M << std::endl
#else
# define SG_LOG(C,P,M) sglog() << loglevel(C,P) << M << endl
#endif


#endif // _LOGSTREAM_H

