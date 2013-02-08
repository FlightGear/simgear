/** \file BufferedLogCallback.hxx
 * Buffer certain log messages permanently for later retrieval and display
 */

// Copyright (C) 2013  James Turner  zakalawe@mac.com
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
     
     
#ifndef SG_DEBUG_BUFFEREDLOGCALLBACK_HXX
#define SG_DEBUG_BUFFEREDLOGCALLBACK_HXX

#include <vector>
#include <memory> // for std::auto_ptr

#include <simgear/debug/logstream.hxx>

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
    std::auto_ptr<BufferedLogCallbackPrivate> d;
};
     

} // of namespace simgear

#endif // of SG_DEBUG_BUFFEREDLOGCALLBACK_HXX