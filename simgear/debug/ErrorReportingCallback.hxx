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

#include <functional>
#include <map>
#include <string>
#include <vector>

#include <simgear/structure/exception.hxx>

//forward decls
class SGPath;

namespace simgear {

void reportError(const std::string& msg, const std::string& more = {});

void reportFatalError(const std::string& msg, const std::string& more = {});

using ErrorReportCallback = std::function<void(const std::string& msg, const std::string& more, bool isFatal)>;

void setErrorReportCallback(ErrorReportCallback cb);

/** kinds of failures we can report. This is *how* (or why) something failed. Extend
  as necessary but update the correponsdings string translations if you do. More detail isn't
 necessariyl useful here: better to provide that in the 'details' string
 */
enum class LoadFailure {
    Unknown,
    NotFound,
    OutOfMemory,
    BadHeader,
    BadData,
    Misconfigured,
    IOError, // disk full, permissions error, etc
    NetworkError
};

/**
 @brief enum of the operations which can fail. This should be extended as necessary: it maps to
 translated error messages for the user. This describes what failed, the enum above gives why/how. The
 combination of what+why should be something at the user level: use details for debug-level information.
 */
enum class ErrorCode {
    LoadEffectsShaders,
    LoadingTexture,
    XMLModelLoad,
    ThreeDModelLoad, // AC3D, OBJ, etc
    BTGLoad,
    ScenarioLoad,
    GUIDialog,
    AudioFX,
    XMLLoadCommand,
    AircraftSystems, // autopilot, hydrualics, instruments
    InputDeviceConfig,
    AITrafficSchedule,
    TerraSync
};
/**
 @brief Define an error-reporting context value, for the duration of this
 object's lifetime. The context value will be active for any errors occuring on the same thread, while
 this object exists.
 */
class ErrorReportContext
{
public:
    ErrorReportContext(const std::string& key, const std::string& value);

    using ContextMap = std::map<std::string, std::string>;

    /**
    Allow establishing multiple context values in a single operation
     */
    ErrorReportContext(const ContextMap& context = {});

    void add(const std::string& key, const std::string& value);

    /**
     @brief allowed delayed add of values
     */
    void addFromMap(const ContextMap& context);

    ~ErrorReportContext();

private:
    std::vector<std::string> _keys;
};

/**
 * @brief Report failure to load a resource, so they can be collated for reporting
 * to the user.
 * 
 * @param type - the reason for the failure, if it can be determined
 * @param msg - an informational message about what caused the failure
 * @param path - path on disk to the resource. In some cases this may be a relative path;
 *  especially in the case of a resource not found, we cannot report a file path.
 */

void reportFailure(LoadFailure type, ErrorCode code, const std::string& detailedMessage = {}, sg_location loc = {});

/**
  overload taking a path as the location
 */
void reportFailure(LoadFailure type, ErrorCode code, const std::string& detailedMessage, const SGPath& p);

using FailureCallback = std::function<void(LoadFailure type, ErrorCode code, const std::string& details, const sg_location& location)>;

void setFailureCallback(FailureCallback cb);

using ContextCallback = std::function<void(const std::string& key, const std::string& value)>;

void setErrorContextCallback(ContextCallback cb);

} // namespace simgear
