// SPDX-License-Identifier: LGPL-2.1-or-later

#include <simgear_config.h>

#include "LogEntry.hxx"

#include <cstring> // for strdup

namespace simgear {

LogEntry::~LogEntry()
{
    if (freeFilename) {
        free(const_cast<char*>(file));
        free(const_cast<char*>(function));
    }
}

LogEntry::LogEntry(const LogEntry& c) : debugClass(c.debugClass),
                                        debugPriority(c.debugPriority),
                                        originalPriority(c.originalPriority),
                                        file(c.file),
                                        line(c.line),
                                        function(c.function),
                                        message(c.message),
                                        freeFilename(c.freeFilename)
{
    if (c.freeFilename) {
        file = strdup(c.file);
        function = strdup(c.function);
    }
}

} // namespace simgear
