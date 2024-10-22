// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 1998 Bernie Bright <bbright@c031.aone.net.au>

/**
 * @file
 * @brief Stream based logging mechanism.
 */

#pragma once

#include <simgear/compiler.h>
#include <simgear/debug/debug_types.h>

#include <sstream>
#include <vector>
#include <memory>

// forward decls
class SGPath;
      
namespace simgear
{

class LogCallback;
/**
 * Helper force a console on platforms where it might optional, when
 * we need to show a console. This basically means Windows at the
 * moment - on other plaforms it's a no-op
 */
void requestConsole(bool ignoreErrors);

void shutdownLogging();

} // of namespace simgear

/**
 * Class to manage the debug logging stream.
 */
class logstream final
{
public:
    ~logstream();   // non-virtual is intentional
    
    static void initGlobalLogstream();

    /**
     * Helper force a console on platforms where it might optional, when
     * we need to show a console. This basically means Windows at the
     * moment - on other plaforms it's a no-op
     */
    void requestConsole(bool ignoreErrors);

    /**
     * Set the global log class and priority level.
     * @param c debug class
     * @param p priority
     */
    void setLogLevels( sgDebugClass c, sgDebugPriority p );

    /**
     * @brief parse a string of the style given to --log-class= argument of
     * FlightGear, i.e with potentially several classes seperated by vertical
     * bars eg 'input|io|aircraft'
     * 
     * Parsing is case- and white-space insensitive.
     * 
     * @param logClassesSpecification 
     */
    void parseLogClasses(const std::string& logClassesSpecification);

    bool would_log(  sgDebugClass c, sgDebugPriority p,
            const char* file, int line, const char* function,
            bool freeFilename=false ) const;

    void logToFile( const SGPath& aPath, sgDebugClass c, sgDebugPriority p );

    void set_log_priority( sgDebugPriority p);
    
    void set_log_classes( sgDebugClass c);

    void addLogClass(const std::string& c);

    sgDebugClass get_log_classes() const;

    /**
     * @brief Get the Log Classes as a string, in the format which could be passed
     * tp parseLogClasses
     * 
     */
    std::string getLogClassesAsString() const;

    sgDebugPriority get_log_priority() const;

    /**
        @brief convert a string value to a log priority.
        throws std::invalid_argument if the string is not valid
     */
    static sgDebugPriority priorityFromString(const std::string& s);
    /**
     * set developer mode on/off. In developer mode, SG_DEV_WARN messages
     * are treated as warnings. In normal (non-developer) mode they are
     * treated as SG_DEBUG.
     */
    void setDeveloperMode(bool devMode);

    /**
     * @brief check if developer mode is set or not
     */
    bool inDeveloperMode() const;

    /**
     * set output of file:line mode on/off. If on, all log messages are
     * prefixed by the file:line of the caller of SG_LOG().
     */
    void setFileLine(bool fileLine);

    /**
     * the core logging method
     */
    void log( sgDebugClass c, sgDebugPriority p,
            const char* fileName, int line, const char* function,
            const std::string& msg);

    // overload of above, which can transfer ownership of the file-name.
    // this is unecesary overhead when logging from C++, since __FILE__ points
    // to constant data, but it's needed when the filename is Nasal data (for
    // example) since during shutdown the filename is freed by Nasal GC
    // asynchronously with the logging thread.
    void logCopyingFilename( sgDebugClass c, sgDebugPriority p,
             const char* fileName, int line, const char* function,
             const std::string& msg);
    
    /**
    * output formatted hex dump of memory block
    */
    void hexdump(sgDebugClass c, sgDebugPriority p,
            const char* fileName, int line, const char* function,
            const void *mem, unsigned int len, unsigned int columns = 16);


    /**
     * support for the SG_POPUP logging class
     * set the content of the popup message
     */
    void popup( const std::string& msg);

    /**
     * retrieve the contents of the popup message and clear it's internal
     * content. The return value may be an empty string.
     */
    std::string get_popup();

    /**
     * return true if a new popup message is available. false otherwise.
     */
    bool has_popup();

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

    void removeCallbacks();

    /**
     * optionally record all entries and submit them to new log callbacks that
     * are added. This allows simplified logging configuration, but still including
     * early startup information in all logs.
     */
    void setStartupLoggingEnabled(bool enabled);

    /**
     * Set up the logstream for running in test mode.  For example the callbacks
     * will be unregistered and the behaviour of the would_log() function
     * sanitized.
     */
    void setTestingMode(bool testMode);

private:
    // constructor
    logstream();

    class LogStreamPrivate;

    std::unique_ptr<LogStreamPrivate> d;
};

logstream& sglog();



/** \def SG_LOG(C,P,M)
 * Log a message.
 * @param C debug class
 * @param P priority
 * @param M message
 */
# define SG_LOGX(C,P,M) \
    do { if(sglog().would_log(C,P, __FILE__, __LINE__, __FUNCTION__)) { \
        std::ostringstream os; os << M;                  \
        sglog().log(C, P, __FILE__, __LINE__, __FUNCTION__, os.str()); \
        if ((P) == SG_POPUP) sglog().popup(os.str());    \
    } } while(0)
#ifdef FG_NDEBUG
# define SG_LOG(C,P,M)	do { if((P) == SG_POPUP) SG_LOGX(C,P,M) } while(0)
# define SG_LOG_NAN(C,P,M) SG_LOG(C,P,M)
# define SG_HEXDUMP(C,P,MEM,LEN)
#else
# define SG_LOG(C,P,M)	SG_LOGX(C,P,M)
# define SG_LOG_NAN(C,P,M) do { SG_LOGX(C,P,M); throw std::overflow_error(M); } while(0)
# define SG_LOG_HEXDUMP(C,P,MEM,LEN) if(sglog().would_log(C,P, __FILE__, __LINE__, __FUNCTION__)) \
        sglog().hexdump(C, P, __FILE__, __LINE__, __FUNCTION__, MEM, LEN)
#endif

#define SG_ORIGIN __FILE__ ":" SG_STRINGIZE(__LINE__)
