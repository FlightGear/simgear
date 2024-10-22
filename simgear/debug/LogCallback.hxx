// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2020 James Turner

/**
 * @file
 * @brief Base class for log callbacks
 */

#pragma once

#include <string>

#include "LogEntry.hxx"
#include "debug_types.h"

namespace simgear {

class LogCallback
{
public:
    virtual ~LogCallback() = default;

    // newer API: return true if you handled the message, otherwise
    // the old API will be called
    virtual bool doProcessEntry(const LogEntry& e);

    // old API, kept for compatability
    virtual void operator()(sgDebugClass c, sgDebugPriority p,
                            const char* file, int line, const std::string& aMessage);

    void setLogLevels(sgDebugClass c, sgDebugPriority p);

    void processEntry(const LogEntry& e);

protected:
    LogCallback(sgDebugClass c, sgDebugPriority p);

    bool shouldLog(sgDebugClass c, sgDebugPriority p) const;
private:
    sgDebugClass m_class;
    sgDebugPriority m_priority;
};


} // namespace simgear
