
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
