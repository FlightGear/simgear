
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


#ifndef _SG_DIR_HXX
#define _SG_DIR_HXX

#include <sys/types.h>

#include <simgear/compiler.h>
#include <string>

#include <simgear/math/sg_types.hxx>
#include <simgear/misc/sg_path.hxx>

#ifdef _MSC_VER
  typedef int mode_t;
#endif

namespace simgear
{
  typedef std::vector<SGPath> PathList;

  class Dir
  {
  public:
    Dir();
    ~Dir();
    
    /**
     * when this directory object is destroyed, remove the corresponding
     * diretory (and its contents) from the disk. Often used with temporary
     * directories to ensure they are cleaned up.
     */
    void setRemoveOnDestroy();
    
    static Dir current();
    
    /**
     * create a temporary directory, using the supplied name
     */
    static Dir tempDir(const std::string& templ);
      
    Dir(const SGPath& path);
    Dir(const Dir& rel, const SGPath& relPath);
    
    enum FileTypes
    {
      TYPE_FILE = 1,
      TYPE_DIR = 2,
      NO_DOT_OR_DOTDOT = 1 << 12,
      INCLUDE_HIDDEN = 1 << 13
    };
    
    PathList children(int types = 0, const std::string& nameGlob = "") const;
    
    /**
     * test if the directory contains no children (except '.' and '..')
     */
    bool isEmpty() const;
    
    SGPath file(const std::string& name) const;
    
    SGPath path() const
        { return _path; }
    
    /**
     * create the directory (and any parents as required) with the
     * request mode, or return failure
     */
    bool create(mode_t mode);
    
    /**
     * remove the directory. 
     * If recursive is true, contained files and directories are
     * recursively removed
     */
    bool remove(bool recursive = false);
    
    /**
     * remove our children but not us
     */
    bool removeChildren() const;
    
    
    /**
     * Check that the directory at the path exists (and is a directory!)
     */
    bool exists() const;
    
    /**
     * parent directory, if one exists
     */
    Dir parent() const;
  private:
    mutable SGPath _path;
    bool _removeOnDestroy;
  };
} // of namespace simgear

#endif // _SG_DIR_HXX


