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
#include <cstring>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <simgear/misc/sg_path.hxx>
#include <simgear/misc/stdint.hxx>
#include <simgear/debug/logstream.hxx>
#include <simgear/misc/strutils.hxx>

#include "sg_mmap.hxx"

#if defined(SG_WINDOWS)
# define WIN32_LEAN_AND_MEAN
# include <windows.h>
# include <io.h>
#else
# include <sys/mman.h>
# include <unistd.h>
#endif

class SGMMapFile::SGMMapFilePrivate
{
public:
    SGPath file_name;
    int fp = -1;
    bool eof_flag = true;
    // Number of repetitions to play. -1 means loop infinitely.
    int repeat = 1;
    int iteration = 0;        // number of current repetition,
                // starting at 0
    int extraoflags = 0;

    char *buffer = nullptr;
    off_t offset = 0;
    size_t size = 0;
#if defined(SG_WINDOWS)
    HANDLE _nativeHandle = NULL;
#endif

   ssize_t mmap_read(void *buf, size_t count);
   ssize_t mmap_write(const void *buf, size_t count);
    
    off_t forward(off_t amount);
};

SGMMapFile::SGMMapFile( )
    : d(new SGMMapFilePrivate)
{
    set_type( sgFileType );
}

SGMMapFile::SGMMapFile(const SGPath &file, int repeat_, int extraoflags_ )
    : d(new SGMMapFilePrivate)
{
    d->file_name = file;
    d->repeat = repeat_;
    d->extraoflags = extraoflags_;
    set_type( sgFileType );
}

SGMMapFile::SGMMapFile( int existingFd )
    : d(new SGMMapFilePrivate)
{
    d->fp = existingFd;
    set_type( sgFileType );
}

SGMMapFile::~SGMMapFile()
{
  close();
}

  // open the file based on specified direction
bool SGMMapFile::open( const SGPath& file, const SGProtocolDir dir )
{
    d->file_name = file;
    return open(dir);
}

// open the file based on specified direction
bool SGMMapFile::open( const SGProtocolDir dir )
{
    set_dir( dir );

#if defined(SG_WINDOWS)
    const std::wstring n = d->file_name.wstr();
#else
    const std::string n = d->file_name.utf8Str();
#endif


    if ( get_dir() == SG_IO_OUT ) {
#if defined(SG_WINDOWS)
        int mode = _S_IREAD | _S_IWRITE;
        d->fp = ::_wopen(n.c_str(), O_WRONLY | O_CREAT | O_TRUNC | d->extraoflags, mode);
#else
        mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
        d->fp = ::open( n.c_str(), O_WRONLY | O_CREAT | O_TRUNC | d->extraoflags, mode );
#endif
    } else if ( get_dir() == SG_IO_IN ) {
#if defined(SG_WINDOWS)
        d->fp = ::_wopen( n.c_str(), O_RDONLY | d->extraoflags );
#else
        d->fp = ::open( n.c_str(), O_RDONLY | d->extraoflags );
#endif
    } else {
        SG_LOG( SG_IO, SG_ALERT,
            "Error:  bidirection mode not available for files." );
        return false;
    }

    if ( d->fp == -1 ) {
        SG_LOG( SG_IO, SG_ALERT, "Error opening file: "	<< d->file_name
               << "\n\t" << simgear::strutils::error_string(errno));
        return false;
    }

    // mmap
    struct stat statbuf;
    fstat(d->fp, &statbuf);

    d->size = (size_t) statbuf.st_size;
#if defined(SG_WINDOWS)
    HANDLE osfHandle = (HANDLE)_get_osfhandle(d->fp);
    if (!osfHandle) {
        SG_LOG( SG_IO, SG_ALERT, "Error mmapping file: _get_osfhandle failed:" << d->file_name
               << "\n\t"  << simgear::strutils::error_string(errno));
        return false;
    }

    d->_nativeHandle = CreateFileMapping(osfHandle, NULL, PAGE_READONLY, 0, 0, NULL);
    if (!d->_nativeHandle) {
        SG_LOG( SG_IO, SG_ALERT, "Error mmapping file: CreateFileMapping failed:" << d->file_name
               << "\n\t"  << simgear::strutils::error_string(errno));
        return false;
    }

    d->buffer = reinterpret_cast<char*>(MapViewOfFile(d->_nativeHandle, FILE_MAP_READ, 0, 0, 0));
    if (!d->buffer) {
        CloseHandle(d->_nativeHandle);
        SG_LOG( SG_IO, SG_ALERT, "Error mmapping file: MapViewOfFile failed:" << d->file_name
               << "\n\t"  << simgear::strutils::error_string(errno));
        return false;
    }
#else
    d->buffer  = (char*) mmap(0, d->size, PROT_READ, MAP_PRIVATE, d->fp, 0L);
    if (d->buffer == MAP_FAILED) {
        SG_LOG( SG_IO, SG_ALERT, "Error mmapping file: " << d->file_name
               << "\n\t"  << simgear::strutils::error_string(errno));
        return false;
    }
#endif
    d-> eof_flag = false;
    return true;
}

off_t SGMMapFile::SGMMapFilePrivate::forward(off_t amount)
{
    if ((size_t) amount > size - offset) {
        amount = size - offset;
    }
    offset += amount;

    if (amount == 0)
        eof_flag = true;

    return amount;
}

const char* SGMMapFile::advance(off_t amount)
{
   const char *ptr = d->buffer + d->offset;

   off_t advanced = d->forward(amount);
   if (advanced != amount) {
      d->eof_flag = true;
      return nullptr;
   }

   return ptr;
}

ssize_t SGMMapFile::SGMMapFilePrivate::mmap_read(void *buf, size_t count)
{
    const char *ptr = buffer + offset;
    size_t result = forward(count);
    memcpy(buf, ptr, result);
    return result;
}

ssize_t SGMMapFile::SGMMapFilePrivate::mmap_write(const void *buf, size_t count)
{
    char *ptr = buffer + offset;
    size_t result = forward(count);
    memcpy(ptr, buf, result);
    return result;
}


// read a block of data of specified size
int SGMMapFile::read( char *buf, int length )
{
    // read a chunk
    ssize_t result = d->mmap_read(buf, length);
    if ( length > 0 && result == 0 ) {
        if (d->repeat < 0 || d->iteration < d->repeat - 1) {
            d->iteration++;
            // loop reading the file, unless it is empty
            off_t fileLen = d->offset;		// lseek(0, SEEK_CUR)
            if (fileLen == 0) {
                d->eof_flag = true;
                return 0;
            } else {
                d->offset = 0;			// lseek(0, SEEK_SET)
                return d->mmap_read(buf, length);
            }
        } else {
            d->eof_flag = true;
        }
    }
    return result;
}

int SGMMapFile::read( char *buf, int length, int num )
{
    return d->mmap_read(buf, num*length)/length;
}

// read a line of data, length is max size of input buffer
int SGMMapFile::readline( char *buf, int length )
{
    int pos = d->offset;				// pos = lseek(0, SEEK_CUR)

    // read a chunk
    ssize_t result = d->mmap_read(buf, length);
    if ( length > 0 && result == 0 ) {
        if ((d->repeat < 0 || d->iteration < d->repeat - 1) && pos != 0) {
            d->iteration++;

            pos = d->offset = 0;			// pos = lseek(0, SEEK_SET)
            result = d->mmap_read(buf, length);
        } else {
            d->eof_flag = true;
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
    d->offset = pos + result;			// lseek(pos+result, SEEK_SET)

    // just in case ...
    buf[ result ] = '\0';
    return result;
}

std::string SGMMapFile::read_all()
{
    return std::string(d->buffer, d->size);
}

// write data to a file
int SGMMapFile::write( const char *buf, const int length )
{
    int result = d->mmap_write(buf, length);

    if ( result != length ) {
        SG_LOG( SG_IO, SG_ALERT, "Error writing data to mmaped-: " << d->file_name
               << "\n\t"  << simgear::strutils::error_string(errno));
    }

    return result;
}


// write null terminated string to a file
int SGMMapFile::writestring( const char *str )
{
    int length = std::strlen( str );
    return write( str, length );
}

// close the port
bool SGMMapFile::close()
{
    if (d->fp != -1 ) {
#if defined(SG_WINDOWS)
        UnmapViewOfFile(d->buffer);
        CloseHandle(d->_nativeHandle);
        d->_nativeHandle = NULL;
#else
        if (munmap(d->buffer, d->size) != 0) {
            SG_LOG( SG_IO, SG_ALERT, "Error un-mapping mmaped-file: " << d->file_name
                   << "\n\t" << simgear::strutils::error_string(errno));
        }
#endif
        if ( ::close( d->fp ) == -1 ) {
            SG_LOG( SG_IO, SG_ALERT, "Error clossing mmaped-file: " << d->file_name
                   << "\n\t" << simgear::strutils::error_string(errno));
            return false;
        }
        d->eof_flag = true;
        d->fp = -1;
        return true;
    }

    return false;
}

const char* SGMMapFile::get() const
{
    return d->buffer;
}

const char* SGMMapFile::ptr() const
{
    return d->buffer + d->offset;
}

size_t SGMMapFile::get_size() const
{
    return d->size;
}

bool SGMMapFile::eof() const
{
    return d->eof_flag;
}

off_t SGMMapFile::forward(off_t amount)
{
    return d->forward(amount);
}
