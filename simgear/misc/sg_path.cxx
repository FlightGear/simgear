// sg_path.cxx -- routines to abstract out path separator differences
//               between MacOS and the rest of the world
//
// Written by Curtis L. Olson, started April 1999.
//
// Copyright (C) 1999  Curtis L. Olson - curt@flightgear.org
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


#include <simgear_config.h>
#include <stdio.h>

#include "sg_path.hxx"


// If Unix, replace all ":" with "/".  If MacOS, replace all "/" with
// ":" it should go without saying that neither of these characters
// should be used in file or directory names.  In windoze, allow the
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
	if ( path[i] == SG_BAD_PATH_SEP ) {
	    path[i] = SG_PATH_SEP;
	}
    }
}


// default constructor
SGPath::SGPath()
    : path("")
{
}


// create a path based on "path"
SGPath::SGPath( const std::string& p )
    : path(p)
{
    fix();
}


// destructor
SGPath::~SGPath() {
}


// set path
void SGPath::set( const string& p ) {
    path = p;
    fix();
}


// append another piece to the existing path
void SGPath::append( const string& p ) {
    if ( path.size() == 0 ) {
	path = p;
    } else {
	if ( p[0] != SG_PATH_SEP ) {
	    path += SG_PATH_SEP;
	}
	path += p;
    }
    fix();
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
}


// Get the file part of the path (everything after the last path sep)
string SGPath::file() const {
    int index = path.rfind(SG_PATH_SEP);
    if (index >= 0) {
	return path.substr(index + 1);
    } else {
	return "";
    }
}
  

// get the directory part of the path.
string SGPath::dir() const {
    int index = path.rfind(SG_PATH_SEP);
    if (index >= 0) {
	return path.substr(0, index);
    } else {
	return "";
    }
}

// get the base part of the path (everything but the extension.)
string SGPath::base() const {
    int index = path.rfind(".");
    if (index >= 0) {
	return path.substr(0, index);
    } else {
	return "";
    }
}

// get the extention (everything after the final ".")
string SGPath::extension() const {
    int index = path.rfind(".");
    if (index >= 0) {
	return path.substr(index + 1);
    } else {
	return "";
    }
}

bool SGPath::exists() const {
    FILE* fp = fopen( path.c_str(), "r");
    if (fp == 0) {
	return false;
    }
    fclose(fp);
    return true;
}
