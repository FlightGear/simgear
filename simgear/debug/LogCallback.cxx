// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2020 James Turner

/**
 * @file
 * @brief Base class for log callbacks
 */

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
