// Stream based logging mechanism.
//
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

#include "logstream.hxx"

#include <iostream>
#include <fstream>

#include <boost/foreach.hpp>

#include <simgear/sg_inlines.h>
#include <simgear/threads/SGThread.hxx>
#include <simgear/threads/SGQueue.hxx>
#include <simgear/threads/SGGuard.hxx>

#include <simgear/misc/sg_path.hxx>

#ifdef _WIN32
// for AllocConsole
    #include "windows.h"
#endif

const char* debugClassToString(sgDebugClass c)
{
    switch (c) {
    case SG_NONE:       return "none";
    case SG_TERRAIN:    return "terrain";
    case SG_ASTRO:      return "astro";
    case SG_FLIGHT:     return "flight";
    case SG_INPUT:      return "input";
    case SG_GL:         return "opengl";
    case SG_VIEW:       return "view";
    case SG_COCKPIT:    return "cockpit";
    case SG_GENERAL:    return "general";
    case SG_MATH:       return "math";
    case SG_EVENT:      return "event";
    case SG_AIRCRAFT:   return "aircraft";
    case SG_AUTOPILOT:  return "autopilot";
    case SG_IO:         return "io";
    case SG_CLIPPER:    return "clipper";
    case SG_NETWORK:    return "network";
    case SG_ATC:        return "atc";
    case SG_NASAL:      return "nasal";
    case SG_INSTR:      return "instruments";
    case SG_SYSTEMS:    return "systems";
    case SG_AI:         return "ai";
    case SG_ENVIRONMENT:return "environment";
    case SG_SOUND:      return "sound";
    case SG_NAVAID:     return "navaid";
    default:            return "unknown";
    }
}

//////////////////////////////////////////////////////////////////////////////

class FileLogCallback : public simgear::LogCallback
{
public:
    FileLogCallback(const std::string& aPath, sgDebugClass c, sgDebugPriority p) :
        m_file(aPath.c_str(), std::ios_base::out | std::ios_base::trunc),
        m_class(c),
        m_priority(p)
    {
    }
    
    virtual void operator()(sgDebugClass c, sgDebugPriority p, 
        const char* file, int line, const std::string& message)
    {
        if ((c & m_class) == 0 || p < m_priority) return;
        m_file << debugClassToString(c) << ":" << (int) p
            << ":" << file << ":" << line << ":" << message << std::endl;
    }
private:
    std::ofstream m_file;   
    sgDebugClass m_class;
    sgDebugPriority m_priority;
};
   
class StderrLogCallback : public simgear::LogCallback
{
public:
    StderrLogCallback()
    {
#ifdef _WIN32
        AllocConsole(); // but only if we want a console
        freopen("conin$", "r", stdin);
        freopen("conout$", "w", stdout);
        freopen("conout$", "w", stderr);
#endif
    }

    virtual void operator()(sgDebugClass c, sgDebugPriority p, 
        const char* file, int line, const std::string& aMessage)
    {
        // if running under MSVC, we could use OutputDebugString here
        
        fprintf(stderr, "%s\n", aMessage.c_str());
        //fprintf(stderr, "%s:%d:%s:%d:%s\n", debugClassToString(c), p,
        //    file, line, aMessage.c_str());
        fflush(stderr);
    }
private:
    
};

namespace simgear
{
 
class BufferedLogCallback::BufferedLogCallbackPrivate
{
public:
    SGMutex m_mutex;
    sgDebugClass m_class;
    sgDebugPriority m_priority;
    string_list m_buffer;
};
   
BufferedLogCallback::BufferedLogCallback(sgDebugClass c, sgDebugPriority p) :
    d(new BufferedLogCallbackPrivate)
{
    d->m_class = c;
    d->m_priority = p;
}

BufferedLogCallback::~BufferedLogCallback()
{
}
 
void BufferedLogCallback::operator()(sgDebugClass c, sgDebugPriority p, 
        const char* file, int line, const std::string& aMessage)
{
    SG_UNUSED(file);
    SG_UNUSED(line);
    
    if ((c & d->m_class) == 0 || p < d->m_priority) return;
    
    SGGuard<SGMutex> g(d->m_mutex);
    d->m_buffer.push_back(aMessage);
}
 
void BufferedLogCallback::threadsafeCopy(string_list& aOutput)
{
    aOutput.clear();
    SGGuard<SGMutex> g(d->m_mutex);
    size_t sz = d->m_buffer.size();
    aOutput.resize(sz);
    for (unsigned int i=0; i<sz; ++i) {
        aOutput[i] = d->m_buffer[i];
    }
} 

} // of namespace simgear

class LogStreamPrivate : public SGThread
{
private:
    /**
     * storage of a single log entry. Note this is not used for a persistent
     * store, but rather for short term buffering between the submitting
     * and output threads.
     */
    class LogEntry
    {
    public:
        LogEntry(sgDebugClass c, sgDebugPriority p,
            const char* f, int l, const std::string& msg) :
            debugClass(c), debugPriority(p), file(f), line(l),
                message(msg)
        {   
        }
        
        sgDebugClass debugClass;
        sgDebugPriority debugPriority;
        const char* file;
        int line;
        std::string message;
    };
public:
    LogStreamPrivate() :
        m_logClass(SG_ALL), m_logPriority(SG_WARN),
        m_isRunning(false)
    { }
                    
    SGMutex m_lock;
    SGBlockingQueue<LogEntry> m_entries;
    std::vector<simgear::LogCallback*> m_callbacks;    
    sgDebugClass m_logClass;
    sgDebugPriority m_logPriority;
    bool m_isRunning;
    
    void startLog()
    {
        SGGuard<SGMutex> g(m_lock);
        if (m_isRunning) return;
        m_isRunning = true;
        start();
    }
    
    virtual void run()
    {
        while (1) {
            LogEntry entry(m_entries.pop());
            if ((entry.debugClass == SG_NONE) && !strcmp(entry.file, "done")) {
                return;
            }
            
            // submit to each installed callback in turn
            BOOST_FOREACH(simgear::LogCallback* cb, m_callbacks) {
                (*cb)(entry.debugClass, entry.debugPriority,
                    entry.file, entry.line, entry.message);
            }           
        } // of main thread loop
    }
    
    bool stop()
    {
        SGGuard<SGMutex> g(m_lock);
        if (!m_isRunning) {
            return false;
        }
        
        // log a special marker value, which will cause the thread to wakeup,
        // and then exit
        log(SG_NONE, SG_ALERT, "done", -1, "");
        join();
        
        m_isRunning = false;
        return true;
    }
    
    void addCallback(simgear::LogCallback* cb)
    {
        bool wasRunning = stop();
        m_callbacks.push_back(cb);
        if (wasRunning) {
            startLog();
        }
    }
    
    bool would_log( sgDebugClass c, sgDebugPriority p ) const
    {
        if (p >= SG_INFO) return true;
        return ((c & m_logClass) != 0 && p >= m_logPriority);
    }
    
    void log( sgDebugClass c, sgDebugPriority p,
            const char* fileName, int line, const std::string& msg)
    {
        LogEntry entry(c, p, fileName, line, msg);
        m_entries.push(entry);
    }
};

/////////////////////////////////////////////////////////////////////////////

static logstream* global_logstream = NULL;
static LogStreamPrivate* global_privateLogstream = NULL;

logstream::logstream()
{
    global_privateLogstream = new LogStreamPrivate;
    global_privateLogstream->addCallback(new StderrLogCallback);
    global_privateLogstream->startLog();
}

void
logstream::setLogLevels( sgDebugClass c, sgDebugPriority p )
{
    // we don't guard writing these with a mutex, since we assume setting
    // occurs very rarely, and any data races are benign. 
    global_privateLogstream->m_logClass = c;
    global_privateLogstream->m_logPriority = p;
}

void
logstream::addCallback(simgear::LogCallback* cb)
{   
    global_privateLogstream->addCallback(cb);
}

void
logstream::log( sgDebugClass c, sgDebugPriority p,
        const char* fileName, int line, const std::string& msg)
{
    global_privateLogstream->log(c, p, fileName, line, msg);
}

bool
logstream::would_log( sgDebugClass c, sgDebugPriority p ) const
{
    return global_privateLogstream->would_log(c,p);
}

sgDebugClass
logstream::get_log_classes() const
{
    return global_privateLogstream->m_logClass;
}
    
sgDebugPriority
logstream::get_log_priority() const
{
    return global_privateLogstream->m_logPriority;
}

void
logstream::set_log_priority( sgDebugPriority p)
{
    global_privateLogstream->m_logPriority = p;
}
    
void
logstream::set_log_classes( sgDebugClass c)
{
    global_privateLogstream->m_logClass = c;
}

logstream&
sglog()
{
    // Force initialization of cerr.
    static std::ios_base::Init initializer;
    if( !global_logstream )
        global_logstream = new logstream();
    return *global_logstream;
}

void
logstream::logToFile( const SGPath& aPath, sgDebugClass c, sgDebugPriority p )
{
    global_privateLogstream->addCallback(new FileLogCallback(aPath.str(), c, p));
}

