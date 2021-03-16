// sg_mmap.cxx -- File I/O routines
//
// Written by Curtis Olson, started November 1999.
//
// Copyright (C) 1999  Curtis L. Olson - http://www.flightgear.org/~curt
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// $Id$

#include <simgear_config.h>
#include <simgear/compiler.h>

#include <string>

#ifdef _WIN32
# include <io.h>
#else
# include <sys/mman.h>
#endif

#include <cstring>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#if !defined(_MSC_VER)
# include <unistd.h>
#endif

#include <simgear/misc/stdint.hxx>
#include <simgear/debug/logstream.hxx>

#include "sg_mmap.hxx"

SGMMapFile::SGMMapFile( )
{
    set_type( sgFileType );
}

SGMMapFile::SGMMapFile(const SGPath &file, int repeat_, int extraoflags_ )
    : file_name(file), repeat(repeat_), extraoflags(extraoflags_)
{
    set_type( sgFileType );
}

SGMMapFile::SGMMapFile( int existingFd ) :
    fp(existingFd)
{
    set_type( sgFileType );
}

SGMMapFile::~SGMMapFile() {
  close();
}

  // open the file based on specified direction
bool SGMMapFile::open( const SGPath& file, const SGProtocolDir d ) {
    file_name = file;
    return open(d);
}

// open the file based on specified direction
bool SGMMapFile::open( const SGProtocolDir d ) {
    set_dir( d );

#if defined(SG_WINDOWS)
    std::wstring n = file_name.wstr();
#else
    std::string n = file_name.utf8Str();
#endif


    if ( get_dir() == SG_IO_OUT ) {
#if defined(SG_WINDOWS)
        int mode = _S_IREAD | _S_IWRITE;
        fp = ::_wopen(n.c_str(), O_WRONLY | O_CREAT | O_TRUNC | extraoflags, mode);
#else
        mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
        fp = ::open( n.c_str(), O_WRONLY | O_CREAT | O_TRUNC | extraoflags, mode );
#endif
    } else if ( get_dir() == SG_IO_IN ) {
#if defined(SG_WINDOWS)
        fp = ::_wopen( n.c_str(), O_RDONLY | extraoflags );
#else
        fp = ::open( n.c_str(), O_RDONLY | extraoflags );
#endif
    } else {
        SG_LOG( SG_IO, SG_ALERT,
            "Error:  bidirection mode not available for files." );
        return false;
    }

    if ( fp == -1 ) {
        SG_LOG( SG_IO, SG_ALERT, "Error opening file: "	<< file_name );
        return false;
    }

    // mmap
    struct stat statbuf;
    fstat(fp, &statbuf);

    size = (size_t)statbuf.st_size;
    buffer = (char*)simple_mmap(fp, size, &un);
    if (buffer == (char*)-1)
    {
        SG_LOG( SG_IO, SG_ALERT, "Error mmapping  file: " << file_name );
        return false;
    }

    eof_flag = false;

    return true;
}

off_t SGMMapFile::forward(off_t amount) {
    if ((size_t)amount > size - offset) {
        amount = size - offset;
    }
    offset += amount;
    return amount;
}

const char* SGMMapFile::advance(off_t amount) {
   const char *ptr = buffer + offset;

   off_t advanced = forward(amount);
   if (advanced != amount)
      return nullptr;

   return ptr;
}

ssize_t SGMMapFile::_read(void *buf, size_t count) {
    const char *ptr = buffer + offset;
    size_t result = forward(count);

    memcpy(buf, ptr, result);

    return result;
}

ssize_t SGMMapFile::_write(const void *buf, size_t count) {
    char *ptr = buffer + offset;
    size_t result = forward(count);

    memcpy(ptr, buf, result);

    return result;
}


// read a block of data of specified size
int SGMMapFile::read( char *buf, int length ) {
    // read a chunk
    ssize_t result = _read(buf, length);
    if ( length > 0 && result == 0 ) {
        if (repeat < 0 || iteration < repeat - 1) {
            iteration++;
            // loop reading the file, unless it is empty
            off_t fileLen = offset;		// lseek(0, SEEK_CUR)
            if (fileLen == 0) {
                eof_flag = true;
                return 0;
            } else {
                offset = 0;			// lseek(0, SEEK_SET)
                return _read(buf, length);
            }
        } else {
            eof_flag = true;
        }
    }
    return result;
}

int SGMMapFile::read( char *buf, int length, int num ) {
    return _read(buf, num*length)/length;
}

// read a line of data, length is max size of input buffer
int SGMMapFile::readline( char *buf, int length ) {
    int pos = offset;				// pos = lseek(0, SEEK_CUR)

    // read a chunk
    ssize_t result = _read(buf, length);
    if ( length > 0 && result == 0 ) {
        if ((repeat < 0 || iteration < repeat - 1) && pos != 0) {
            iteration++;

            pos = offset = 0;			// pos = lseek(0, SEEK_SET)
            result = _read(buf, length);
        } else {
            eof_flag = true;
        }
    }

    // find the end of line and reset position
    int i;
    for ( i = 0; i < result && buf[i] != '\n'; ++i );
    if ( buf[i] == '\n' ) {
	result = i + 1;
    } else {
	result = i;
    }
    offset = pos + result;			// lseek(pos+result, SEEK_SET)

    // just in case ...
    buf[ result ] = '\0';

    return result;
}

std::string SGMMapFile::read_all() {
    return std::string(buffer, size);
}

// write data to a file
int SGMMapFile::write( const char *buf, const int length ) {
    int result = _write(buf, length);

    if ( result != length ) {
        SG_LOG( SG_IO, SG_ALERT, "Error writing data: " << file_name );
    }

    return result;
}


// write null terminated string to a file
int SGMMapFile::writestring( const char *str ) {
    int length = std::strlen( str );
    return write( str, length );
}


// close the port
bool SGMMapFile::close() {
    if (fp != -1 ) {
        simple_unmmap(buffer, size, &un);
        if ( ::close( fp ) == -1 ) {
            return false;
        }
        eof_flag = true;
        return true;
    }

    return false;
}

#ifdef WIN32
/* Source:
 * https://mollyrocket.com/forums/viewtopic.php?p=2529
 */

void *
SGMMapFile::simple_mmap(int fd, size_t length, SIMPLE_UNMMAP *un)
{
    HANDLE f;
    HANDLE m;
    void *p;

    f = (HANDLE)_get_osfhandle(fd);
    if (!f) return (void *)-1;

    m = CreateFileMapping(f, NULL, PAGE_READONLY, 0, 0, NULL);
    if (!m) return (void *)-1;

    p = MapViewOfFile(m, FILE_MAP_READ, 0, 0, 0);
    if (!p)
    {
        CloseHandle(m);
        return (void *)-1;
    }

    if (un)
    {
        un->m = m;
        un->p = p;
    }

    return p;
}

void
SGMMapFile::simple_unmmap(void *addr, size_t len, SIMPLE_UNMMAP *un)
{
    UnmapViewOfFile(un->p);
    CloseHandle(un->m);
}
#endif

