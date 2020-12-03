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

#include "ErrorReportingCallback.hxx"

using std::string;

namespace simgear {

static ErrorReportCallback static_callback;

void setErrorReportCallback(ErrorReportCallback cb)
{
    static_callback = cb;
}


void reportError(const std::string& msg, const std::string& more)
{
    if (!static_callback)
        return;

    static_callback(msg, more, false);
}

void reportFatalError(const std::string& msg, const std::string& more)
{
    if (!static_callback)
        return;
    static_callback(msg, more, true);
}

} // namespace simgear
