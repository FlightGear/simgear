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

#include "LogCallback.hxx"

using namespace simgear;

LogCallback::LogCallback(sgDebugClass c, sgDebugPriority p) : m_class(c),
                                                              m_priority(p)
{
}

void LogCallback::operator()(sgDebugClass c, sgDebugPriority p,
                             const char* file, int line, const std::string& aMessage)
{
    // override me
}

bool LogCallback::doProcessEntry(const LogEntry& e)
{
    return false;
}

void LogCallback::processEntry(const LogEntry& e)
{
    if (doProcessEntry(e))
        return; // derived class used the new API

    // call the old API
    (*this)(e.debugClass, e.debugPriority, e.file, e.line, e.message);
}


bool LogCallback::shouldLog(sgDebugClass c, sgDebugPriority p) const
{
    if ((c & m_class) != 0 && p >= m_priority)
        return true;
    if (c == SG_OSG) // always have OSG logging as it OSG logging is configured separately.
        return true;
    return false;
}

void LogCallback::setLogLevels(sgDebugClass c, sgDebugPriority p)
{
    m_priority = p;
    m_class = c;
}
const char* LogCallback::debugPriorityToString(sgDebugPriority p)
{
    switch (p) {
    case SG_DEV_ALERT:
    case SG_ALERT:
        return "ALRT";
    case SG_BULK: return "BULK";
    case SG_DEBUG: return "DBUG";
    case SG_MANDATORY_INFO:
    case SG_INFO:
        return "INFO";
    case SG_POPUP: return "POPU";
    case SG_DEV_WARN:
    case SG_WARN:
        return "WARN";

    default: return "UNKN";
    }
}

const char* LogCallback::debugClassToString(sgDebugClass c)
{
    switch (c) {
    case SG_NONE: return "none";
    case SG_TERRAIN: return "terrain";
    case SG_ASTRO: return "astro";
    case SG_FLIGHT: return "flight";
    case SG_INPUT: return "input";
    case SG_GL: return "opengl";
    case SG_VIEW: return "view";
    case SG_COCKPIT: return "cockpit";
    case SG_GENERAL: return "general";
    case SG_MATH: return "math";
    case SG_EVENT: return "event";
    case SG_AIRCRAFT: return "aircraft";
    case SG_AUTOPILOT: return "autopilot";
    case SG_IO: return "io";
    case SG_CLIPPER: return "clipper";
    case SG_NETWORK: return "network";
    case SG_ATC: return "atc";
    case SG_NASAL: return "nasal";
    case SG_INSTR: return "instruments";
    case SG_SYSTEMS: return "systems";
    case SG_AI: return "ai";
    case SG_ENVIRONMENT: return "environment";
    case SG_SOUND: return "sound";
    case SG_NAVAID: return "navaid";
    case SG_GUI: return "gui";
    case SG_TERRASYNC: return "terrasync";
    case SG_PARTICLES: return "particles";
    case SG_HEADLESS: return "headless";
    case SG_OSG: return "OSG";
    default: return "unknown";
    }
}
