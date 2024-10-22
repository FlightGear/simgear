// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2023 James Hogan

#include <simgear_config.h>
#include <simgear/structure/SGSourceLocation.hxx>

#include <sstream>

#include <simgear/misc/sg_path.hxx>
#include <simgear/structure/exception.hxx>

SGSourceLocation::SGSourceLocation()
    : _line(-1),
      _column(-1)
{
}

SGSourceLocation::SGSourceLocation(const sg_location& location)
    : _line(location.getLine()),
      _column(location.getColumn())
{
    setPath(location.getPath());
}

SGSourceLocation::SGSourceLocation(const std::string& path, int line, int column)
    : _line(line),
      _column(column)
{
    setPath(path);
}

SGSourceLocation::SGSourceLocation(const SGPath& path, int line, int column)
    : _line(line),
      _column(column)
{
    setPath(path.utf8Str());
}

SGSourceLocation::SGSourceLocation(const char* path, int line, int column)
    : _line(line),
      _column(column)
{
    setPath(path);
}

void SGSourceLocation::setPath(const std::string& str)
{
    const std::lock_guard<std::mutex> lock(_pathsMutex);
    auto it = _paths.find(str);
    if (it == _paths.end()) {
        _path = std::make_shared<std::string>(str);
        _paths[str] = _path;
    } else {
        _path = (*it).second;
    }
}

std::ostream& operator<<(std::ostream& out,
                         const SGSourceLocation& loc)
{
    if (loc._path)
        out << *loc._path;
    if (loc._line >= 0)
        out << ":" << loc._line;
    if (loc._column >= 0)
        out << ":" << loc._column;
    return out;
}
