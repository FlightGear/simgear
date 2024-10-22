// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2013 James Turner <zakalawe@mac.com>

/**
 * @file
 * @brief Buffer certain log messages permanently for later retrieval and display
 */
     
#ifndef SG_DEBUG_BUFFEREDLOGCALLBACK_HXX
#define SG_DEBUG_BUFFEREDLOGCALLBACK_HXX

#include <vector>
#include <memory> // for std::unique_ptr

#include <simgear/debug/LogCallback.hxx>

namespace simgear
{

class BufferedLogCallback : public LogCallback
{
public:
    BufferedLogCallback(sgDebugClass c, sgDebugPriority p);
    virtual ~BufferedLogCallback();
    
    /// truncate messages longer than a certain length. This is to work-around
    /// for broken PUI behaviour, it can be removed once PUI is gone.
    void truncateAt(unsigned int);
    
    virtual void operator()(sgDebugClass c, sgDebugPriority p, 
        const char* file, int line, const std::string& aMessage);
    
    /**
     * read the stamp value associated with the log buffer. This is
     * incremented whenever the log contents change, so can be used
     * to poll for changes.
     */
    unsigned int stamp() const;
    
    /**
    * copying a (large) vector of std::string would be very expensive.
    * once logged, this call retains storage of the underlying string data,
    * so when copying, it's sufficient to pass around the strings as raw
    * char arrays. This means we're only copying a vector of pointers,
    * which is very efficient.
    */
    typedef std::vector<unsigned char*> vector_cstring;
     
    /**
     * copy the buffered log data into the provided output list
     * (which will be cleared first). This method is safe to call from
     * any thread.
     *
     * returns the stamp value of the copied data
     */
    unsigned int threadsafeCopy(vector_cstring& aOutput);
private:
    class BufferedLogCallbackPrivate;
    std::unique_ptr<BufferedLogCallbackPrivate> d;
};
     

} // of namespace simgear

#endif // of SG_DEBUG_BUFFEREDLOGCALLBACK_HXX
