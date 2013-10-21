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

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include <simgear/misc/sg_dir.hxx>
#include <math.h>
#include <stdlib.h>
#include <cstdio>

#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#  include <direct.h>
#else
#  include <sys/types.h>
#  include <dirent.h>
#  include <sys/stat.h>
#  include <unistd.h>
#  include <errno.h>
#endif

#include <simgear/debug/logstream.hxx>
#include <boost/foreach.hpp>

#include <cstring>
#include <cstdlib>
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
  _path(rel.file(relPath.str())),
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
#ifdef _WIN32
    char* buf = _getcwd(NULL, 0);
#else
    char* buf = ::getcwd(NULL, 0);
#endif
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
    ::snprintf(buf, 1024, "%s", p.c_str());
    if (!mkdtemp(buf)) {
        SG_LOG(SG_IO, SG_WARN, "mkdtemp failed:" << strerror(errno));
        return Dir();
    }
    
    return Dir(SGPath(buf));
#else
    SGPath p(tempnam(0, templ.c_str()));
    Dir t(p);
    if (!t.create(0700)) {
        SG_LOG(SG_IO, SG_WARN, "failed to create temporary directory at " << p.str());
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
  
#ifdef _WIN32
  std::string search(_path.str());
  if (nameFilter.empty()) {
    search += "\\*"; // everything
  } else {
    search += "\\*" + nameFilter;
  }
  
  WIN32_FIND_DATA fData;
  HANDLE find = FindFirstFile(search.c_str(), &fData);
  if (find == INVALID_HANDLE_VALUE) {
	  int err = GetLastError();
	  if (err != ERROR_FILE_NOT_FOUND) {
		SG_LOG(SG_GENERAL, SG_WARN, "Dir::children: FindFirstFile failed:" << 
			_path.str() << " with error:" << err);
	  }
    return result;
  }
  
  bool done = false;
  for (bool done = false; !done; done = (FindNextFile(find, &fData) == 0)) {
    if (fData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) {
      if (!(types & INCLUDE_HIDDEN)) {
        continue;
      }
    }
    
    if (fData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
	  if (types & NO_DOT_OR_DOTDOT) {
		if (!strcmp(fData.cFileName,".") || !strcmp(fData.cFileName,"..")) {
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

    result.push_back(file(fData.cFileName));
  }

  FindClose(find);
#else
  DIR* dp = opendir(_path.c_str());
  if (!dp) {
    SG_LOG(SG_GENERAL, SG_WARN, "Dir::children: opendir failed:" << _path.str());
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

bool Dir::isEmpty() const
{
  bool empty= true;
#ifdef _WIN32 
  WIN32_FIND_DATA fData;
  HANDLE find = FindFirstFile("\\*", &fData);
  if (find == INVALID_HANDLE_VALUE) {
    return true;
  }
  
// since an empty dir will still have . and .. children, we need
// watch for those - anything else means the dir is really non-empty
  bool done = false;
  for (; !done; done = (FindNextFile(find, &fData) == 0)) {
    if (fData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
		  if (!strcmp(fData.cFileName,".") || !strcmp(fData.cFileName,"..")) {
		    continue;
		  }
	  }
    
    empty = false;
    break;
  }
    
  FindClose(find);
#else
  DIR* dp = opendir(_path.c_str());
  if (!dp) {
    return true;
  }
    
  while (true) {
    struct dirent* entry = readdir(dp);
    if (!entry) {
      break; // done iteration
    }
    
    if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..")) {
      continue;
    }
      
    empty = false;
    break;
  }
  closedir(dp);
#endif
  return empty;
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

#ifdef _WIN32
#  define sgMkDir(d,m)       _mkdir(d)
#else
#  define sgMkDir(d,m)       mkdir(d,m)
#endif

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
    int err = sgMkDir(_path.c_str(), mode);
    if (err) {
        SG_LOG(SG_IO, SG_WARN,  "directory creation failed: (" << _path.str() << ") " << strerror(errno) );
    }
    
    return (err == 0);
}

bool Dir::removeChildren() const
{
    bool ok;
    PathList cs = children(NO_DOT_OR_DOTDOT | INCLUDE_HIDDEN | TYPE_FILE | TYPE_DIR);
    BOOST_FOREACH(SGPath path, cs) {
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
            return false;
        }
    } // of recursive deletion
    
#ifdef _WIN32
    int err = _rmdir(_path.c_str());
#else
    int err = rmdir(_path.c_str());
#endif
    if (err) {
        SG_LOG(SG_IO, SG_WARN, "rmdir failed:" << _path.str() << ":" << strerror(errno));
    }
    return (err == 0);
}

Dir Dir::parent() const
{
    return Dir(_path.dir());
}

} // of namespace simgear
