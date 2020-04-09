// Written by James Turner, started July 2010.
//
// Copyright (C) 2010  Curtis L. Olson - http://www.flightgear.org/~curt
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
// $Id$

#include <simgear_config.h>
#include <simgear/compiler.h>

#include <simgear/misc/sg_dir.hxx>
#include <simgear/structure/exception.hxx>
#include <math.h>
#include <stdlib.h>
#include <cstdio>

#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#  include <direct.h>
#  include <Shlwapi.h>
#else
#  include <sys/types.h>
#  include <dirent.h>
#  include <sys/stat.h>
#  include <unistd.h>
#endif

#include <simgear/misc/strutils.hxx>
#include <simgear/debug/logstream.hxx>

#include <cstring>
#include <cstdlib>
#include <cerrno>
#include <iostream>
#include <algorithm> // for std::sort

using std::string;

namespace simgear
{

Dir::Dir() :
    _removeOnDestroy(false)
{
}

Dir::Dir(const SGPath& path) :
  _path(path),
  _removeOnDestroy(false)
{
    _path.set_cached(false); // disable caching, so create/remove work
}

Dir::Dir(const Dir& rel, const SGPath& relPath) :
  _path(rel.file(relPath.utf8Str())),
  _removeOnDestroy(false)
{
    _path.set_cached(false); // disable caching, so create/remove work
}

Dir::~Dir()
{
    if (_removeOnDestroy) {
        remove(true);
    }
}

void Dir::setRemoveOnDestroy()
{
    _removeOnDestroy = true;
}

Dir Dir::current()
{
#if defined(SG_WINDOWS)
    wchar_t* buf = _wgetcwd(NULL, 0);
#else
    char *buf = ::getcwd(NULL, 0);
#endif
    if (!buf) {
        if  (errno == 2) throw sg_exception("The current directory is invalid");
        else throw sg_exception(simgear::strutils::error_string(errno));
    }

    SGPath p(buf);
    free(buf);
    return Dir(p);
}

Dir Dir::tempDir(const std::string& templ)
{
#ifdef HAVE_MKDTEMP
    char buf[1024];
    const char* tempPath = ::getenv("TMPDIR");
    if (!tempPath) {
        tempPath = "/tmp";
    }
    SGPath p(tempPath);
    p.append(templ);
    // Mac OS-X / BSD manual says any number of 'X's, but GLibc manual
    // says exactly six, so that's what I'm going with
    p.concat("-XXXXXX");
    std::string s = p.utf8Str();
    ::snprintf(buf, 1024, "%s", s.c_str());
    if (!mkdtemp(buf)) {
        SG_LOG(SG_IO, SG_WARN,
               "mkdtemp failed: " << simgear::strutils::error_string(errno));
        return Dir();
    }

    return Dir(SGPath(buf));
#else
#if defined(SG_WINDOWS)
	std::wstring wideTemplate = simgear::strutils::convertUtf8ToWString(templ);
	wchar_t* buf = _wtempnam(0, wideTemplate.c_str());
	SGPath p(buf);
	free(buf); // unlike tempnam(), _wtempnam mallocs its result buffer
#else
    SGPath p(tempnam(0, templ.c_str()));
#endif
    Dir t(p);
    if (!t.create(0700)) {
        SG_LOG(SG_IO, SG_WARN, "failed to create temporary directory at " << p);
        return Dir();
    }

    return t;
#endif
}

static bool pathSortPredicate(const SGPath& p1, const SGPath& p2)
{
  return p1.file() < p2.file();
}

PathList Dir::children(int types, const std::string& nameFilter) const
{
  PathList result;
  if (types == 0) {
    types = TYPE_FILE | TYPE_DIR | NO_DOT_OR_DOTDOT;
  }

#if defined(SG_WINDOWS)
  std::wstring search(_path.wstr());
  if (nameFilter.empty()) {
    search += simgear::strutils::convertUtf8ToWString("\\*"); // everything
  } else {
    search += simgear::strutils::convertUtf8ToWString("\\*" + nameFilter);
  }

  WIN32_FIND_DATAW fData;
  HANDLE find = FindFirstFileW(search.c_str(), &fData);
  if (find == INVALID_HANDLE_VALUE) {
	  int err = GetLastError();
	  if (err != ERROR_FILE_NOT_FOUND) {
		SG_LOG(SG_GENERAL, SG_WARN, "Dir::children: FindFirstFile failed:" <<
			_path << " with error:" << err);
	  }
    return result;
  }

  bool done = false;
  for (bool done = false; !done; done = (FindNextFileW(find, &fData) == 0)) {
    if (fData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) {
      if (!(types & INCLUDE_HIDDEN)) {
        continue;
      }
    }

	std::string utf8File = simgear::strutils::convertWStringToUtf8(fData.cFileName);
    if (fData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
	  if (types & NO_DOT_OR_DOTDOT) {
		if ((utf8File == ".") || (utf8File == "..")) {
		  continue;
		}
	  }

      if (!(types & TYPE_DIR)) {
        continue;
      }
	} else if ((fData.dwFileAttributes & FILE_ATTRIBUTE_DEVICE) ||
				(fData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT))
	{
		continue; // always ignore device files
    } else if (!(types & TYPE_FILE)) {
       continue;
    }

    result.push_back(file(utf8File));
  }

  FindClose(find);
#else
    std::string ps = _path.utf8Str();
  DIR* dp = opendir(ps.c_str());
  if (!dp) {
    SG_LOG(SG_GENERAL, SG_WARN, "Dir::children: opendir failed:" << _path);
    return result;
  }

  int filterLen = nameFilter.size();

  while (true) {
    struct dirent* entry = readdir(dp);
    if (!entry) {
      break; // done iteration
    }

    // skip hidden files (names beginning with '.') unless requested
    if (!(types & INCLUDE_HIDDEN) && (entry->d_name[0] == '.') &&
         strcmp(entry->d_name, ".") && strcmp(entry->d_name, "..")) {
      continue;
    }

    SGPath f = file(entry->d_name);
    if (!f.exists()) {
      continue; // stat() failed
    }

    if (f.isDir()) {
      // directory handling
      if (!(types & TYPE_DIR)) {
        continue;
      }

      if (types & NO_DOT_OR_DOTDOT) {
        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..")) {
          continue;
        }
      }
    } else if (f.isFile()) {
      // regular file handling
      if (!(types & TYPE_FILE)) {
        continue;
      }
    } else {
      // block device /fifo/char file, ignore
      continue;
    }

    if (!nameFilter.empty()) {
      int nameLen = strlen(entry->d_name);
      if (nameLen < filterLen) {
        continue; // name is shorter than the filter
      }

      char* nameSuffix = entry->d_name + (nameLen - filterLen);
      if (strcmp(nameSuffix, nameFilter.c_str())) {
        continue;
      }
    }

  // passed all criteria, add to our result vector
    result.push_back(file(entry->d_name));
  }

  closedir(dp);
#endif

  // File system order is random. Make things deterministic,
  // so it's the same for every user.
  std::sort(result.begin(), result.end(), pathSortPredicate);

  return result;
}

bool Dir::isNull() const
{
  return _path.isNull();
}

bool Dir::isEmpty() const
{
#if defined(SG_WINDOWS)
  std::wstring ps = _path.wstr();
  return PathIsDirectoryEmptyW( ps.c_str() );
#else
  std::string ps = _path.utf8Str();
  DIR* dp = opendir( ps.c_str() );
  if (!dp) return true;

  int n = 0;
  dirent* d;
  while (n < 3 && (d = readdir(dp)) != nullptr) {
    n++;
  }

  closedir(dp);

  return (n == 2); // '.' and '..' always exist
#endif
}

bool Dir::exists() const
{
  return _path.isDir();
}

SGPath Dir::file(const std::string& name) const
{
  SGPath childPath = _path;
  childPath.set_cached(true);
  childPath.append(name);
  return childPath;
}

bool Dir::create(mode_t mode)
{
    if (exists()) {
        return false; // already exists
    }

// recursively create parent directories
    Dir pr(parent());
    if (!pr.path().isNull() && !pr.exists()) {
        bool ok = pr.create(mode);
        if (!ok) {
            return false;
        }
    }

// finally, create ourselves
#if defined(SG_WINDOWS)
	std::wstring ps = _path.wstr();
	int err = _wmkdir(ps.c_str());
#else
    std::string ps = _path.utf8Str();
    int err = mkdir(ps.c_str(), mode);
#endif
    if (err) {
        SG_LOG(SG_IO, SG_WARN,
               "directory creation failed for '" << _path.utf8Str() << "': " <<
               simgear::strutils::error_string(errno));
    }

    return (err == 0);
}

bool Dir::removeChildren() const
{
    if (!exists()) {
        return true;
    }

    bool ok;
    PathList cs = children(NO_DOT_OR_DOTDOT | INCLUDE_HIDDEN | TYPE_FILE | TYPE_DIR);
    for (auto path : cs) {
        if (path.isDir()) {
            Dir childDir(path);
            ok = childDir.remove(true);
        } else {
            ok = path.remove();
        }

        if (!ok) {
            SG_LOG(SG_IO, SG_WARN, "failed to remove:" << path);
            return false;
        }
    } // of child iteration

    return true;
}

bool Dir::remove(bool recursive)
{
    if (!exists()) {
        SG_LOG(SG_IO, SG_WARN, "attempt to remove non-existant dir:" << _path);
        return false;
    }

    if (recursive) {
        if (!removeChildren()) {
            SG_LOG(SG_IO, SG_WARN, "Dir at:" << _path << " failed to remove children");
            return false;
        }
    } // of recursive deletion

#if defined(SG_WINDOWS)
	std::wstring ps = _path.wstr();
    int err = _wrmdir(ps.c_str());
#else
	std::string ps = _path.utf8Str();
    int err = rmdir(ps.c_str());
#endif
    if (err) {
        SG_LOG(SG_IO, SG_WARN,
               "rmdir failed for '" << _path.utf8Str() << "': " <<
               simgear::strutils::error_string(errno));
    }
    return (err == 0);
}

Dir Dir::parent() const
{
    return Dir(_path.dir());
}

} // of namespace simgear
