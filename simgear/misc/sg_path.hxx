/**
 * \file sg_path.hxx
 * Routines to abstract out path separator differences between MacOS
 * and the rest of the world.
 */

// Written by Curtis L. Olson, started April 1999.
//
// Copyright (C) 1999  Curtis L. Olson - http://www.flightgear.org/~curt
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


#ifndef _SG_PATH_HXX
#define _SG_PATH_HXX

#include <sys/types.h>

#include <simgear/compiler.h>
#include <string>
#include <ctime>

#include <simgear/math/sg_types.hxx>

#ifdef _MSC_VER
  typedef int mode_t;
#endif

/**
 * A class to hide path separator difference across platforms and assist
 * in managing file system path names.
 *
 * Paths can be input in any platform format and will be converted
 * automatically to the proper format.
 */

class SGPath {

public:

    /** Default constructor */
    SGPath();

    /** Copy contructor */
    SGPath(const SGPath& p);
    
    SGPath& operator=(const SGPath& p);

    /**
     * Construct a path based on the starting path provided.
     * @param p initial path
     */
    SGPath( const std::string& p );

    /**
     * Construct a path based on the starting path provided and a relative subpath
     * @param p initial path
     * @param r relative subpath
     */
    SGPath( const SGPath& p, const std::string& r );

    /** Destructor */
    ~SGPath();

    /**
     * Set path to a new value
     * @param p new path
     */
    void set( const std::string& p );
    SGPath& operator= ( const char* p ) { this->set(p); return *this; }

    bool operator==(const SGPath& other) const;
    bool operator!=(const SGPath& other) const;
    
    /**
     * Set if file information (exists, type, mod-time) is cached or
     * retrieved each time it is queried. Caching is enabled by default
     */
    void set_cached(bool cached);
    
    /**
     * Append another piece to the existing path.  Inserts a path
     * separator between the existing component and the new component.
     * @param p additional path component */
    void append( const std::string& p );

    /**
     * Append a new piece to the existing path.  Inserts a search path
     * separator to the existing path and the new patch component.
     * @param p additional path component */
    void add( const std::string& p );

    /**
     * Concatenate a string to the end of the path without inserting a
     * path separator.
     * @param p additional path suffix
     */
    void concat( const std::string& p );

    /**
     * Get the file part of the path (everything after the last path sep)
     * @return file string
     */
    std::string file() const;
  
    /**
     * Get the directory part of the path.
     * @return directory string
     */
    std::string dir() const;
  
    /**
     * Get the base part of the path (everything but the extension.)
     * @return the base string
     */
    std::string base() const;

    /**
     * Get the base part of the filename (everything before the first '.')
     * @return the base filename
     */
    std::string file_base() const;

    /**
     * Get the extension part of the path (everything after the final ".")
     * @return the extension string
     */
    std::string extension() const;
    
    /**
     * Get the extension part of the path (everything after the final ".")
     * converted to lowercase
     * @return the extension string
     */
    std::string lower_extension() const;
    
    /**
     * Get the complete extension part of the path (everything after the first ".")
     * this might look like 'tar.gz' or 'txt.Z', or might be identical to 'extension' above
     * the extension is converted to lowercase.
     * @return the extension string
     */
    std::string complete_lower_extension() const;
    
    /**
     * Get the path string
     * @return path string
     */
    std::string str() const { return path; }

    /**
     * Get the path string
     * @return path in "C" string (ptr to char array) form.
     */
    const char* c_str() const { return path.c_str(); }

    /**
     * Get the path string in OS native form
     */
    std::string str_native() const;

    /**
     * Determine if file exists by attempting to fopen it.
     * @return true if file exists, otherwise returns false.
     */
    bool exists() const;

    /**
     * Create the designated directory.
     * @return 0 on success, or <0 on failure.
     */
    int create_dir(mode_t mode);

    bool isFile() const;
    bool isDir() const;
    
    /**
     * Opposite sense to isAbsolute
     */
    bool isRelative() const { return !isAbsolute(); }
    
    /**
     * Is this an absolute path?
     * I.e starts with a directory seperator, or a single character + colon
     */
    bool isAbsolute() const;
    
    /**
     * check for default constructed path
     */
    bool isNull() const;
    
    /**
     * delete the file, if possible
     */
    bool remove();
    
    /**
     * modification time of the file
     */
    time_t modTime() const;
    
    /**
     * rename the file / directory we point at, to a new name
     * this may fail if the new location is on a different volume / share,
     * or if the destination already exists, or is not writeable
     */
    bool rename(const SGPath& newName);
private:

    void fix();

    void validate() const;

    std::string path;
    
    mutable bool _cached : 1;
    bool _cacheEnabled : 1; ///< cacheing can be disbled if required
    mutable bool _exists : 1;
    mutable bool _isDir : 1;
    mutable bool _isFile : 1;
    mutable time_t _modTime;
};


/**
 * Split a directory string into a list of it's parent directories.
 */
string_list sgPathBranchSplit( const std::string &path );

/**
 * Split a directory search path into a vector of individual paths
 */
string_list sgPathSplit( const std::string &search_path );


#endif // _SG_PATH_HXX


