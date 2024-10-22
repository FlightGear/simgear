// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2023 James Hogan

#ifndef _SG_SOURCE_LOCATION_HXX
#define _SG_SOURCE_LOCATION_HXX 1

#include <map>
#include <memory>
#include <string>
#include <mutex>

class SGPath;
class sg_location;

/**
 * Information encapsulating a single location in an external resource.
 *
 * A position in the resource my optionally be provided, either by
 * line number, or line number and column number.
 *
 * This is based on sg_location from simgear/structure/exception.hxx, but is
 * more space efficient, with the file path string deduplicated, and is
 * permitted to raise exceptions. This makes it more suitable for fairly compact
 * storage of debug information for later debug output.
 */
class SGSourceLocation
{
public:
    SGSourceLocation();
    SGSourceLocation(const sg_location& location);
    SGSourceLocation(const std::string& path, int line = -1, int column = -1);
    SGSourceLocation(const SGPath& path, int line = -1, int column = -1);
    explicit SGSourceLocation(const char* path, int line = -1, int column = -1);

    bool isValid() const
    {
        return (bool)_path;
    }

    const char* getPath() const
    {
        return _path ? _path->c_str() : "";
    }

    int getLine() const
    {
        return _line;
    }

    int getColumn() const
    {
        return _column;
    }

    friend std::ostream& operator<<(std::ostream& out,
                                    const SGSourceLocation& loc);

private:
    void setPath(const std::string& str);

    inline static std::map<std::string, std::shared_ptr<std::string>> _paths;
    inline static std::mutex _pathsMutex;

    std::shared_ptr<std::string> _path;
    int _line;
    int _column;
};

#endif // _SG_SOURCE_LOCATION_HXX
