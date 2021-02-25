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

#include <simgear/misc/sg_path.hxx>

using std::string;

namespace simgear {

static ErrorReportCallback static_callback;
static ContextCallback static_contextCallback;


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

static FailureCallback static_failureCallback;

void reportFailure(LoadFailure type, ErrorCode code, const std::string& details, sg_location loc)
{
    if (!static_failureCallback) {
        return;
    }

    static_failureCallback(type, code, details, loc);
}

void reportFailure(LoadFailure type, ErrorCode code, const std::string& details, const SGPath& path)
{
    if (!static_failureCallback) {
        return;
    }

    static_failureCallback(type, code, details, sg_location{path});
}

void setFailureCallback(FailureCallback cb)
{
    static_failureCallback = cb;
}

void setErrorContextCallback(ContextCallback cb)
{
    static_contextCallback = cb;
}

ErrorReportContext::ErrorReportContext(const std::string& key, const std::string& value)
{
    if (static_contextCallback) {
        _keys.push_back(key);
        static_contextCallback(key, value);
    }
}

ErrorReportContext::ErrorReportContext(const ContextMap& context)
{
    addFromMap(context);
}

void ErrorReportContext::addFromMap(const ContextMap& context)
{
    if (static_contextCallback) {
        for (const auto& p : context) {
            _keys.push_back(p.first);
            static_contextCallback(p.first, p.second);
        }
    }
}


ErrorReportContext::~ErrorReportContext()
{
    if (static_contextCallback) {
        // pop all our keys
        for (const auto& k : _keys) {
            static_contextCallback(k, "POP");
        }
    }
}

} // namespace simgear
