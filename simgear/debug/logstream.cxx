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
    StderrLogCallback(sgDebugClass c, sgDebugPriority p) :
        m_class(c),
            m_priority(p)
    {
#ifdef _WIN32
        AllocConsole(); // but only if we want a console
        freopen("conin$", "r", stdin);
        freopen("conout$", "w", stdout);
        freopen("conout$", "w", stderr);
#endif
    }
    
    void setLogLevels( sgDebugClass c, sgDebugPriority p )
    {
        m_priority = p;
        m_class = c;
    }
    
    virtual void operator()(sgDebugClass c, sgDebugPriority p, 
        const char* file, int line, const std::string& aMessage)
    {
        if ((c & m_class) == 0 || p < m_priority) return;
        
        // if running under MSVC, we could use OutputDebugString here
        
        fprintf(stderr, "%s\n", aMessage.c_str());
        //fprintf(stderr, "%s:%d:%s:%d:%s\n", debugClassToString(c), p,
        //    file, line, aMessage.c_str());
        fflush(stderr);
    }
private:
    sgDebugClass m_class;
    sgDebugPriority m_priority;
};

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
    
    class PauseThread
    {
    public:
        PauseThread(LogStreamPrivate* parent) : m_parent(parent)
        {
            m_wasRunning = m_parent->stop();
        }
        
        ~PauseThread()
        {
            if (m_wasRunning) {
                m_parent->startLog();
            }
        }
    private:
        LogStreamPrivate* m_parent;
        bool m_wasRunning;
    };
public:
    LogStreamPrivate() :
        m_logClass(SG_ALL), 
        m_logPriority(SG_ALERT),
        m_isRunning(false)
    { 
        m_stderrCallback = new StderrLogCallback(m_logClass, m_logPriority);
        addCallback(m_stderrCallback);
    }
                    
    SGMutex m_lock;
    SGBlockingQueue<LogEntry> m_entries;
    
    typedef std::vector<simgear::LogCallback*> CallbackVec;
    CallbackVec m_callbacks;    
    
    sgDebugClass m_logClass;
    sgDebugPriority m_logPriority;
    bool m_isRunning;
    StderrLogCallback* m_stderrCallback;
    
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
            // special marker entry detected, terminate the thread since we are
            // making a configuration change or quitting the app
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
        PauseThread pause(this);
        m_callbacks.push_back(cb);
    }
    
    void removeCallback(simgear::LogCallback* cb)
    {
        PauseThread pause(this);
        CallbackVec::iterator it = std::find(m_callbacks.begin(), m_callbacks.end(), cb);
        if (it != m_callbacks.end()) {
            m_callbacks.erase(it);
        }
    }
    
    void setLogLevels( sgDebugClass c, sgDebugPriority p )
    {
        PauseThread pause(this);
        m_logPriority = p;
        m_logClass = c;
        m_stderrCallback->setLogLevels(c, p);
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
    global_privateLogstream->startLog();
}

void
logstream::setLogLevels( sgDebugClass c, sgDebugPriority p )
{
    global_privateLogstream->setLogLevels(c, p);
}

void
logstream::addCallback(simgear::LogCallback* cb)
{   
    global_privateLogstream->addCallback(cb);
}

void
logstream::removeCallback(simgear::LogCallback* cb)
{   
    global_privateLogstream->removeCallback(cb);
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

