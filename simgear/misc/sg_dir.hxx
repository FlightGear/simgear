// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2010 James Turner
// SPDX-FileCopyrightText: 2010 Curtis L. Olson

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

  class Dir final
  {
  public:
    Dir();
    ~Dir();   // non-virtual intentional

    /**
     * when this directory object is destroyed, remove the corresponding
     * directory (and its contents) from the disk. Often used with temporary
     * directories to ensure they are cleaned up.
     */
    void setRemoveOnDestroy();

    static Dir current();

    /**
     * Create a temporary directory, using the supplied name.
     * The return value 'd' is such that d.isNull() in case this failed.
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
     * Check if the underlying SGPath is null.
     *
     * Note: this is the case for a default-constructed Dir instance.
     */
    bool isNull() const;

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


