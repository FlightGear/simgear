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

#pragma once

#include <string>

#include "LogEntry.hxx"
#include "debug_types.h"

namespace simgear {

class LogCallback
{
public:
    virtual ~LogCallback() = default;

    // newer API: return true if you handled the message, otherwise
    // the old API will be called
    virtual bool doProcessEntry(const LogEntry& e);

    // old API, kept for compatability
    virtual void operator()(sgDebugClass c, sgDebugPriority p,
                            const char* file, int line, const std::string& aMessage);

    void setLogLevels(sgDebugClass c, sgDebugPriority p);

    void processEntry(const LogEntry& e);

protected:
    LogCallback(sgDebugClass c, sgDebugPriority p);

    bool shouldLog(sgDebugClass c, sgDebugPriority p) const;

    static const char* debugClassToString(sgDebugClass c);
    static const char* debugPriorityToString(sgDebugPriority p);

private:
    sgDebugClass m_class;
    sgDebugPriority m_priority;
};


} // namespace simgear
