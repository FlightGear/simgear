// Copyright (C) 2021  James Turner - <james@flightgear.org>
//
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

#include "untar.hxx"

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
        PRE_END_OF_ARCHVE,
        END_OF_ARCHIVE,
        ERROR_STATE, ///< states above this are error conditions
        BAD_ARCHIVE,
        BAD_DATA,
        FILTER_STOPPED
    } State;

    State state = INVALID;
    ArchiveExtractor* outer = nullptr;

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