// zlib input file stream wrapper.
//
// Written by Bernie Bright, 1998
//
// Copyright (C) 1998  Bernie Bright - bbright@c031.aone.net.au
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
#include <string>
#include <ctype.h> // isspace()
#include <cerrno>

#include "sgstream.hxx"

#include <simgear/misc/sg_path.hxx>

using std::istream;
using std::ostream;

sg_gzifstream::sg_gzifstream()
    : istream(&gzbuf)
{
}

//-----------------------------------------------------------------------------
//
// Open a possibly gzipped file for reading.
//
sg_gzifstream::sg_gzifstream( const SGPath& name, ios_openmode io_mode )
    : istream(&gzbuf)
{
    this->open( name, io_mode );
}

//-----------------------------------------------------------------------------
//
// Attach a stream to an already opened file descriptor.
//
sg_gzifstream::sg_gzifstream( int fd, ios_openmode io_mode )
    : istream(&gzbuf)
{
    gzbuf.attach( fd, io_mode );
}

//-----------------------------------------------------------------------------
//
// Open a possibly gzipped file for reading.
// If the initial open fails and the filename has a ".gz" extension then
// remove the extension and try again.
// If the initial open fails and the filename doesn't have a ".gz" extension
// then append ".gz" and try again.
//
void
sg_gzifstream::open( const SGPath& name, ios_openmode io_mode )
{
    std::string s = name.utf8Str();
    gzbuf.open( s.c_str(), io_mode );
    if ( ! gzbuf.is_open() )
    {
        if ( s.substr( s.length() - 3, 3 ) == ".gz" )
        {
            // remove ".gz" suffix
            s.replace( s.length() - 3, 3, "" );
    // 	    s.erase( s.length() - 3, 3 );
        }
        else
        {
            // Append ".gz" suffix
            s += ".gz";
        }

        // Try again.
        gzbuf.open( s.c_str(), io_mode );
    }
}

void
sg_gzifstream::attach( int fd, ios_openmode io_mode )
{
    gzbuf.attach( fd, io_mode );
}

//
// Manipulators
//

istream&
skipeol( istream& in )
{
    char c = '\0';

    // make sure we detect LF, CR and CR/LF
    while ( in.get(c) ) {
        if (c == '\n')
            break;
        else if (c == '\r') {
            if (in.peek() == '\n')
                in.get(c);
            break;
        }
    }
    return in;
}

istream&
skipws( istream& in ) {
    char c;
    while ( in.get(c) ) {

	if ( ! isspace( c ) ) {
	    // put pack the non-space character
	    in.putback(c);
	    break;
	}
    }
    return in;
}

istream&
skipcomment( istream& in )
{
    while ( in )
    {
	// skip whitespace
	in >> skipws;

	char c;
	if ( in.get( c ) && c != '#' )
	{
	    // not a comment
	    in.putback(c);
	    break;
	}
	in >> skipeol;
    }
    return in;
}


sg_gzofstream::sg_gzofstream()
    : ostream(&gzbuf)
{
}

//-----------------------------------------------------------------------------
//
// Open a file for gzipped writing.
//
sg_gzofstream::sg_gzofstream( const SGPath& name, ios_openmode io_mode )
    : ostream(&gzbuf)
{
    this->open( name, io_mode );
}

//-----------------------------------------------------------------------------
//
// Attach a stream to an already opened file descriptor.
//
sg_gzofstream::sg_gzofstream( int fd, ios_openmode io_mode )
    : ostream(&gzbuf)
{
    gzbuf.attach( fd, io_mode );
}

//-----------------------------------------------------------------------------
//
// Open a file for gzipped writing.
//
void
sg_gzofstream::open( const SGPath& name, ios_openmode io_mode )
{
    std::string s = name.utf8Str();
    gzbuf.open( s.c_str(), io_mode );
}

void
sg_gzofstream::attach( int fd, ios_openmode io_mode )
{
    gzbuf.attach( fd, io_mode );
}


sg_ifstream::sg_ifstream(const SGPath& path, ios_openmode io_mode)
{
#if defined(SG_WINDOWS)
	std::wstring ps = path.wstr();
#else
    std::string ps = path.local8BitStr();
#endif
	std::ifstream::open(ps.c_str(), io_mode);
}

void sg_ifstream::open( const SGPath& name, ios_openmode io_mode )
{
#if defined(SG_WINDOWS)
	std::wstring ps = name.wstr();
#else
	std::string ps = name.local8BitStr();
#endif
    std::ifstream::open(ps.c_str(), io_mode);
}

sg_ofstream::sg_ofstream(const SGPath& path, ios_openmode io_mode)
{
#if defined(SG_WINDOWS)
	std::wstring ps = path.wstr();
#else
	std::string ps = path.local8BitStr();
#endif
    std::ofstream::open(ps.c_str(), io_mode);
}

void sg_ofstream::open( const SGPath& name, ios_openmode io_mode )
{
#if defined(SG_WINDOWS)
	std::wstring ps = name.wstr();
#else
	std::string ps = name.local8BitStr();
#endif
    std::ofstream::open(ps.c_str(), io_mode);
}
