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
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the
// Free Software Foundation, Inc., 59 Temple Place - Suite 330,
// Boston, MA  02111-1307, USA.
//
// $Id$


#ifndef _SG_PATH_HXX
#define _SG_PATH_HXX


#include <simgear/compiler.h>
#include STL_STRING

#include <simgear/math/sg_types.hxx>

SG_USING_STD(string);


/**
 * A class to hide path separator difference across platforms and assist
 * in managing file system path names.
 *
 * Paths can be input in any platform format and will be converted
 * automatically to the proper format.
 */

class SGPath {

private:

    string path;

public:

    /** Default constructor */
    SGPath();

    /**
     * Construct a path based on the starting path provided.
     * @param p initial path
     */
    SGPath( const string& p );

    /** Destructor */
    ~SGPath();

    /**
     * Set path to a new value
     * @param p new path
     */
    void set( const string& p );
    SGPath& operator= ( const char* p ) { this->set(p); return *this; }

    /**
     * Append another piece to the existing path.  Inserts a path
     * separator between the existing component and the new component.
     * @param p additional path component */
    void append( const string& p );

    /**
     * Append a new piece to the existing path.  Inserts a search path
     * separator to the existing path and the new patch component.
     * @param p additional path component */
    void add( const string& p );

    /**
     * Concatenate a string to the end of the path without inserting a
     * path separator.
     * @param p addtional path suffix
     */
    void concat( const string& p );

    /**
     * Get the file part of the path (everything after the last path sep)
     * @return file string
     */
    string file() const;
  
    /**
     * Get the directory part of the path.
     * @return directory string
     */
    string dir() const;
  
    /**
     * Get the base part of the path (everything but the extension.)
     * @return the base string
     */
    string base() const;

    /**
     * Get the extention part of the path (everything after the final ".")
     * @return the extention string
     */
    string extension() const;

    /** Get the path string
     * @return path string
     */
    string str() const { return path; }

    /** Get the path string
     * @return path in "C" string (ptr to char array) form.
     */
    const char* c_str() { return path.c_str(); }

    /**
     * Determine if file exists by attempting to fopen it.
     * @return true if file exists, otherwise returns false.
     */
    bool exists() const;

private:

    void fix();

};


/**
 * Split a directory search path into a vector of individual paths
 */
string_list sgPathSplit( const string &search_path );


#endif // _SG_PATH_HXX


