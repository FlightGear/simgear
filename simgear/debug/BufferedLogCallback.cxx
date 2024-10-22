// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2013 James Turner <zakalawe@mac.com>

/**
 * @file
 * @brief Buffer certain log messages permanently for later retrieval and display
 */

#include <simgear_config.h>
#include <simgear/debug/BufferedLogCallback.hxx>

#include <simgear/sg_inlines.h>
#include <simgear/threads/SGThread.hxx>

#include <cstdlib> // for malloc
#include <cstring>
#include <mutex>

namespace simgear
{

class BufferedLogCallback::BufferedLogCallbackPrivate
{
public:
    std::mutex m_mutex;
    vector_cstring m_buffer;
    unsigned int m_stamp;
    unsigned int m_maxLength;
};

BufferedLogCallback::BufferedLogCallback(sgDebugClass c, sgDebugPriority p) :
	simgear::LogCallback(c,p),
    d(new BufferedLogCallbackPrivate)
{
    d->m_stamp = 0;
    d->m_maxLength = 0xffff;
}

BufferedLogCallback::~BufferedLogCallback()
{
    for (auto msg : d->m_buffer) {
        free(msg);
    }
}

void BufferedLogCallback::operator()(sgDebugClass c, sgDebugPriority p,
        const char* file, int line, const std::string& aMessage)
{
    SG_UNUSED(file);
    SG_UNUSED(line);

    if (!shouldLog(c, p)) return;

    vector_cstring::value_type msg;
    if (aMessage.size() >= d->m_maxLength) {
        msg = (vector_cstring::value_type) malloc(d->m_maxLength);
        if (msg) {
            strncpy((char*) msg, aMessage.c_str(), d->m_maxLength - 1);
            msg[d->m_maxLength - 1] = 0; // add final NULL byte
        }
    } else {
        msg = (vector_cstring::value_type) strdup(aMessage.c_str());
    }

    std::lock_guard<std::mutex> g(d->m_mutex);
    d->m_buffer.push_back(msg);
    d->m_stamp++;
}

unsigned int BufferedLogCallback::stamp() const
{
    return d->m_stamp;
}

unsigned int BufferedLogCallback::threadsafeCopy(vector_cstring& aOutput)
{
    std::lock_guard<std::mutex> g(d->m_mutex);
    size_t sz = d->m_buffer.size();
    aOutput.resize(sz);
    memcpy(aOutput.data(), d->m_buffer.data(), sz * sizeof(vector_cstring::value_type));
    return d->m_stamp;
}

void BufferedLogCallback::truncateAt(unsigned int t)
{
    d->m_maxLength = t;
}

} // of namespace simgear
