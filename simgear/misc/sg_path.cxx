// sg_path.cxx -- routines to abstract out path separator differences
//               between MacOS and the rest of the world
//
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


#include <simgear/compiler.h>

#include <simgear_config.h>
#include <simgear/debug/logstream.hxx>
#include <stdio.h>
#include <sys/stat.h>
#ifdef _WIN32
#  include <direct.h>
#endif
#include "sg_path.hxx"

using std::string;


/**
 * define directory path separators
 */

static const char sgDirPathSep = '/';
static const char sgDirPathSepBad = '\\';

#ifdef _WIN32
static const char sgSearchPathSep = ';';
#else
static const char sgSearchPathSep = ':';
#endif


// If Unix, replace all ":" with "/".  In windoze, allow the
// second character to be a ":" for things like c:\foo\bar

void
SGPath::fix()
{
    for ( string::size_type i = 0; i < path.size(); ++i ) {
#if defined( WIN32 )
	// for windoze, don't replace the ":" for the second character
	if ( i == 1 ) {
	    continue;
	}
#endif
	if ( path[i] == sgDirPathSepBad ) {
	    path[i] = sgDirPathSep;
	}
    }
}


// default constructor
SGPath::SGPath()
    : path(""),
    _cached(false)
{
}


// create a path based on "path"
SGPath::SGPath( const std::string& p )
    : path(p),
    _cached(false)
{
    fix();
}

// create a path based on "path" and a "subpath"
SGPath::SGPath( const SGPath& p, const std::string& r )
    : path(p.path),
    _cached(false)
{
    append(r);
    fix();
}

SGPath::SGPath(const SGPath& p) :
  path(p.path),
  _cached(p._cached),
  _exists(p._exists),
  _isDir(p._isDir),
  _isFile(p._isFile)
{
}
    
SGPath& SGPath::operator=(const SGPath& p)
{
  path = p.path;
  _cached = p._cached;
  _exists = p._exists;
  _isDir = p._isDir;
  _isFile = p._isFile;
  return *this;
}

// destructor
SGPath::~SGPath() {
}


// set path
void SGPath::set( const string& p ) {
    path = p;
    fix();
    _cached = false;
}


// append another piece to the existing path
void SGPath::append( const string& p ) {
    if ( path.size() == 0 ) {
	path = p;
    } else {
	if ( p[0] != sgDirPathSep ) {
	    path += sgDirPathSep;
	}
	path += p;
    }
    fix();
    _cached = false;
}

//add a new path component to the existing path string
void SGPath::add( const string& p ) {
    append( sgSearchPathSep+p );
}


// concatenate a string to the end of the path without inserting a
// path separator
void SGPath::concat( const string& p ) {
    if ( path.size() == 0 ) {
	path = p;
    } else {
	path += p;
    }
    fix();
    _cached = false;
}


// Get the file part of the path (everything after the last path sep)
string SGPath::file() const {
    int index = path.rfind(sgDirPathSep);
    if (index >= 0) {
	return path.substr(index + 1);
    } else {
	return "";
    }
}
  

// get the directory part of the path.
string SGPath::dir() const {
    int index = path.rfind(sgDirPathSep);
    if (index >= 0) {
	return path.substr(0, index);
    } else {
	return "";
    }
}

// get the base part of the path (everything but the extension.)
string SGPath::base() const {
    int index = path.rfind(".");
    if ((index >= 0) && (path.find("/", index) == string::npos)) {
	return path.substr(0, index);
    } else {
	return "";
    }
}

// get the extension (everything after the final ".")
// but make sure no "/" follows the "." character (otherwise it
// is has to be a directory name containing a ".").
string SGPath::extension() const {
    int index = path.rfind(".");
    if ((index >= 0)  && (path.find("/", index) == string::npos)) {
	return path.substr(index + 1);
    } else {
	return "";
    }
}

void SGPath::validate() const
{
  if (_cached) {
    return;
  }
  
#ifdef _WIN32
  struct _stat buf ;

  if (_stat (path.c_str(), &buf ) < 0) {
    _exists = false;
  } else {
    _exists = true;
    _isFile = ((S_IFREG & buf.st_mode ) !=0);
    _isDir = ((S_IFDIR & buf.st_mode ) !=0);
  }

#else
  struct stat buf ;

  if (stat(path.c_str(), &buf ) < 0) {
    _exists = false;
  } else {
    _exists = true;
    _isFile = ((S_ISREG(buf.st_mode )) != 0);
    _isDir = ((S_ISDIR(buf.st_mode )) != 0);
  }
  
#endif
  _cached = true;
}

bool SGPath::exists() const
{
  validate();
  return _exists;
}

bool SGPath::isDir() const
{
  validate();
  return _exists && _isDir;
}

bool SGPath::isFile() const
{
  validate();
  return _exists && _isFile;
}

#ifdef _WIN32
#  define sgMkDir(d,m)       _mkdir(d)
#else
#  define sgMkDir(d,m)       mkdir(d,m)
#endif


int SGPath::create_dir( mode_t mode ) {
    string_list dirlist = sgPathSplit(dir());
    if ( dirlist.empty() )
        return -1;
    string path = dirlist[0];
    string_list path_elements = sgPathBranchSplit(path);
    bool absolute = !path.empty() && path[0] == sgDirPathSep;

    unsigned int i = 1;
    SGPath dir = absolute ? string( 1, sgDirPathSep ) : "";
    dir.concat( path_elements[0] );
#ifdef _WIN32
    if ( dir.str().find(':') != string::npos && path_elements.size() >= 2 ) {
        dir.append( path_elements[1] );
        i = 2;
    }
#endif
    struct stat info;
    int r;
    for(; ( r = stat( dir.c_str(), &info ) ) == 0 && i < path_elements.size(); i++) {
        dir.append(path_elements[i]);
    }
    if ( r == 0 ) {
        return 0; // Directory already exists
    }
    if ( sgMkDir( dir.c_str(), mode) ) {
        SG_LOG( SG_IO, SG_ALERT, "Error creating directory: " + dir.str() );
        return -2;
    }
    for(; i < path_elements.size(); i++) {
        dir.append(path_elements[i]);
        if ( sgMkDir( dir.c_str(), mode) ) {
            SG_LOG( SG_IO, SG_ALERT, "Error creating directory: " + dir.str() );
            return -2;
        }
    }

    return 0;
}

string_list sgPathBranchSplit( const string &dirpath ) {
    string_list path_elements;
    string element, path = dirpath;
    while ( path.size() ) {
        size_t p = path.find( sgDirPathSep );
        if ( p != string::npos ) {
            element = path.substr( 0, p );
            path.erase( 0, p + 1 );
        } else {
            element = path;
            path = "";
        }
        if ( element.size() )
            path_elements.push_back( element );
    }
    return path_elements;
}


string_list sgPathSplit( const string &search_path ) {
    string tmp = search_path;
    string_list result;
    result.clear();

    bool done = false;

    while ( !done ) {
        int index = tmp.find(sgSearchPathSep);
        if (index >= 0) {
            result.push_back( tmp.substr(0, index) );
            tmp = tmp.substr( index + 1 );
        } else {
            if ( !tmp.empty() )
                result.push_back( tmp );
            done = true;
        }
    }

    return result;
}

bool SGPath::isAbsolute() const
{
  if (path.empty()) {
    return false;
  }
  
#ifdef _WIN32
  // detect '[A-Za-z]:/'
  if (path.size() > 2) {
    if (isalpha(path[0]) && (path[1] == ':') && (path[2] == sgDirPathSep)) {
      return true;
    }
  }
#endif
  
  return (path[0] == sgDirPathSep);
}
