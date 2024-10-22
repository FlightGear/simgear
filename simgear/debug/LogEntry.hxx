// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <string>

#include "debug_types.h"

namespace simgear {
/**
 * storage of a single log entry. This is used to pass log entries from
 * the various threads to the logging thread, and also to store the startup
 * entries
 */
class LogEntry final
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

    ~LogEntry();    // non-virtual is intentional

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
