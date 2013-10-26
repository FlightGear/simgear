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
#include <simgear/debug/debug_types.h>

#include <sstream>
 
// forward decls
class SGPath;
      
namespace simgear
{
     
class LogCallback
{
public:
    virtual ~LogCallback() {}
    virtual void operator()(sgDebugClass c, sgDebugPriority p, 
        const char* file, int line, const std::string& aMessage) = 0;

	void setLogLevels(sgDebugClass c, sgDebugPriority p);
protected:
	LogCallback(sgDebugClass c, sgDebugPriority p);

	bool shouldLog(sgDebugClass c, sgDebugPriority p) const;
private:
	sgDebugClass m_class;
	sgDebugPriority m_priority;
};

/**
 * Helper force a console on platforms where it might optional, when
 * we need to show a console. This basically means Windows at the
 * moment - on other plaforms it's a no-op
 */
void requestConsole();
     
} // of namespace simgear

/**
 * Class to manage the debug logging stream.
 */
class logstream
{
public:
    static void initGlobalLogstream();
    /**
     * Set the global log class and priority level.
     * @param c debug class
     * @param p priority
     */
    void setLogLevels( sgDebugClass c, sgDebugPriority p );

    bool would_log(  sgDebugClass c, sgDebugPriority p ) const;

    void logToFile( const SGPath& aPath, sgDebugClass c, sgDebugPriority p );

    void set_log_priority( sgDebugPriority p);
    
    void set_log_classes( sgDebugClass c);
    
    sgDebugClass get_log_classes() const;
    
    sgDebugPriority get_log_priority() const;

    /**
     * the core logging method
     */
    void log( sgDebugClass c, sgDebugPriority p,
            const char* fileName, int line, const std::string& msg);

   /**
    * \relates logstream
    * Return the one and only logstream instance.
    * We use a function instead of a global object so we are assured that cerr
    * has been initialised.
    * @return current logstream
    */
    friend logstream& sglog();
    
    /**
     * register a logging callback. Note callbacks are run in a
     * dedicated thread, so callbacks which pass data to other threads
     * must use appropriate locking.
     */
    void addCallback(simgear::LogCallback* cb);
     
    void removeCallback(simgear::LogCallback* cb);

private:
    // constructor
    logstream();
};

logstream& sglog();



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
	if(sglog().would_log(C,P)) {                      \
        std::ostringstream os;                             \
        os << M;                                       \
        sglog().log(C, P, __FILE__, __LINE__, os.str()); \
    } \
} while(0)
#endif

#define SG_ORIGIN __FILE__ ":" SG_STRINGIZE(__LINE__)

#endif // _LOGSTREAM_H

