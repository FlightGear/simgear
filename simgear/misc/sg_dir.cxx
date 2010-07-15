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


#include <simgear/misc/sg_dir.hxx>

#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#else
#  include <sys/types.h>
#  include <dirent.h>
#endif

#include <simgear/debug/logstream.hxx>

#include <iostream>

namespace simgear
{

Dir::Dir(const SGPath& path) :
  _path(path)
{
}

Dir::Dir(const Dir& rel, const SGPath& relPath) :
  _path(rel.file(relPath.str()))
{
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
    SG_LOG(SG_GENERAL, SG_WARN, "Dir::children: FindFirstFile failed:" << _path.str());
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
  
  while (true) {
    struct dirent* entry = readdir(dp);
    if (!entry) {
      break; // done iteration
    }
    
    // skip hidden files (names beginning with '.') unless requested
    if (!(types & INCLUDE_HIDDEN) && (entry->d_name[0] == '.')) {
      continue;
    }
        
    if (entry->d_type == DT_DIR) {
      if (!(types & TYPE_DIR)) {
        continue;
      }
      
      if (types & NO_DOT_OR_DOTDOT) {
        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..")) {
          continue;
        }
      }
    } else if (entry->d_type == DT_REG) {
      if (!(types & TYPE_FILE)) {
        continue;
      }
    } else {
      continue; // ignore char/block devices, fifos, etc
    }
    
    if (!nameFilter.empty()) {
      if (strstr(entry->d_name, nameFilter.c_str()) == NULL) {
        continue;
      }
    }
    
  // passed all criteria, add to our result vector
    result.push_back(file(entry->d_name));
  }
  
  closedir(dp);
#endif
  return result;
}

SGPath Dir::file(const std::string& name) const
{
  SGPath childPath = _path;
  childPath.append(name);
  return childPath;  
}

} // of namespace simgear
