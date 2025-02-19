// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2021 James Turner <james@flightgear.org>

#pragma once

#include <cassert>

#include "untar.hxx"

#include <simgear/misc/sg_hash.hxx>

namespace simgear {

class ArchiveExtractorPrivate
{
public:
    ArchiveExtractorPrivate(ArchiveExtractor* o) : outer(o)
    {
        assert(outer);
    }

    virtual ~ArchiveExtractorPrivate() = default;

    typedef enum {
      INVALID = 0,
      READING_HEADER,
      READING_FILE,
      READING_PADDING,
      READING_PAX_GLOBAL_ATTRIBUTES,
      READING_PAX_FILE_ATTRIBUTES,
      READING_GNU_LONGNAME,
      PRE_END_OF_ARCHVE,
      END_OF_ARCHIVE,
      ERROR_STATE, ///< states above this are error conditions
      BAD_ARCHIVE,
      BAD_DATA,
      FILTER_STOPPED
    } State;

    State state = INVALID;
    ArchiveExtractor* outer = nullptr;
    sha1nfo hashState;
    SGPath mostRecentPath;

    virtual void extractBytes(const uint8_t* bytes, size_t count) = 0;

    virtual void flush() = 0;

    SGPath extractRootPath()
    {
        return outer->_rootPath;
    }

    ArchiveExtractor::PathResult filterPath(std::string& pathToExtract)
    {
        return outer->filterPath(pathToExtract);
    }

    bool doRemoveTopmostDir() const
    {
        return outer->_removeTopmostDir;
    }

    bool doCreateDirHashes() const
    {
        return outer->_doCreateDirHashes;
    }

    bool isSafePath(const std::string& p) const
    {
        if (p.empty()) {
            return false;
        }

        // reject absolute paths
        if (p.at(0) == '/') {
            return false;
        }

        // reject paths containing '..'
        size_t doubleDot = p.find("..");
        if (doubleDot != std::string::npos) {
            return false;
        }

        // on POSIX could use realpath to sanity check
        return true;
    }
};

} // namespace simgear