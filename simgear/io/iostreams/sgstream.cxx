// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 1998 Bernie Bright <bbright@c031.aone.net.au>

/**
 * @file
 * @brief zlib input file stream wrapper.
 */

#include <ios>
#include <simgear_config.h>
#include <simgear/compiler.h>
#include <string>
#include <ctype.h> // isspace()
#include <cerrno>

#include <zlib.h>

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
sg_gzifstream::sg_gzifstream( const SGPath& name, ios_openmode io_mode,
  bool use_exact_name )
    : istream(&gzbuf)
{
    this->open( name, io_mode, use_exact_name );
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
// If 'use_exact_name' is true, just try to open the indicated file, nothing
// else. Otherwise:
//   - if the initial open fails and the filename has a ".gz" extension, then
//     remove it and try again;
//   - if the initial open fails and the filename doesn't have a ".gz"
//     extension, then append ".gz" and try again.
//
void
sg_gzifstream::open( const SGPath& name, ios_openmode io_mode,
                     bool use_exact_name )
{
    std::string s = name.utf8Str();
    gzbuf.open( s.c_str(), io_mode );
    if ( ! (gzbuf.is_open() || use_exact_name) )
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

z_off_t
sg_gzifstream::approxOffset() {
  return gzbuf.approxOffset();
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
	in >> std::skipws;

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
    std::string ps = path.utf8Str();
#endif
	std::ifstream::open(ps.c_str(), io_mode);
}

void sg_ifstream::open( const SGPath& name, ios_openmode io_mode )
{
#if defined(SG_WINDOWS)
	std::wstring ps = name.wstr();
#else
	std::string ps = name.utf8Str();
#endif
    std::ifstream::open(ps.c_str(), io_mode);
}

std::string sg_ifstream::read_all()
{
	this->seekg(0, std::ios::end); // seek to end
	const auto pos = this->tellg();

	std::string result;
	result.resize(pos);

	this->seekg(0, std::ios::beg);
	this->read(const_cast<char*>(result.data()), pos);
    return result;
}


sg_ofstream::sg_ofstream(const SGPath& path, ios_openmode io_mode)
{
#if defined(SG_WINDOWS)
	std::wstring ps = path.wstr();
#else
	std::string ps = path.utf8Str();
#endif
    std::ofstream::open(ps.c_str(), io_mode);
}

void sg_ofstream::open( const SGPath& name, ios_openmode io_mode )
{
#if defined(SG_WINDOWS)
	std::wstring ps = name.wstr();
#else
	std::string ps = name.utf8Str();
#endif
    std::ofstream::open(ps.c_str(), io_mode);
}
