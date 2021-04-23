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
#include "logstream.hxx"

#include <simgear/misc/sg_path.hxx>

using std::string;

namespace simgear {

static ErrorReportCallback static_callback;
static ContextCallback static_contextCallback;


string_list static_errorNames = {
    "shader/effect",
    "loading texture",
    "XML/animation load",
    "3D model load",
    "loading BTG",
    "sceanrio load",
    "UI dialog load",
    "loading Audio/FX",
    "Nasal/commnad load of XML",
    "aircraft-systems",
    "loading input/joystick config",
    "load AI/traffic",
    "from TerraSync"};

string_list static_errorTypeNames = {
    "unknown",
    "not found",
    "out of memory",
    "bad header",
    "bad data",
    "misconfigured",
    "disk IO/permissions",
    "network IO"};

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
        SG_LOG(SG_GENERAL, SG_WARN, "Error:" << static_errorTypeNames.at(static_cast<int>(type)) << " from " << static_errorNames.at(static_cast<int>(code)) << "::" << details << "\n\t" << loc.asString());
        return;
    }

    static_failureCallback(type, code, details, loc);
}

void reportFailure(LoadFailure type, ErrorCode code, const std::string& details, const SGPath& path)
{
    if (!static_failureCallback) {
        // if no callback is registered, log here instead
        SG_LOG(SG_GENERAL, SG_WARN, "Error:" << static_errorTypeNames.at(static_cast<int>(type)) << " from " << static_errorNames.at(static_cast<int>(code)) << "::" << details << "\n\t" << path);

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
    add(key, value);
}

void ErrorReportContext::add(const std::string& key, const std::string& value)
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
