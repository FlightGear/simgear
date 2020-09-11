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

#include "debug_types.h"

namespace simgear {
/**
 * storage of a single log entry. This is used to pass log entries from
 * the various threads to the logging thread, and also to store the startup
 * entries
 */
class LogEntry
{
public:
    LogEntry(sgDebugClass c, sgDebugPriority p,
             sgDebugPriority op,
             const char* file, int line, const char* function,
             const std::string& msg, bool freeFilename)
    :
    debugClass(c),
    debugPriority(p),
    originalPriority(op),
    file(file),
    line(line),
    function(function),
    message(msg),
    freeFilename(freeFilename)
    {
    }

    LogEntry(const LogEntry& c);
    LogEntry& operator=(const LogEntry& c) = delete;

    ~LogEntry();

    const sgDebugClass debugClass;
    const sgDebugPriority debugPriority;
    const sgDebugPriority originalPriority;
    const char* file;
    const int line;
    const char* function;
    const std::string message;

    bool freeFilename = false; ///< if true, we own, and therefore need to free, the memory pointed to by 'file'
};

} // namespace simgear
