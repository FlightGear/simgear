// tileentry.hxx -- routines to handle an individual scenery tile
//
// Written by Curtis Olson, started May 1998.
//
// Copyright (C) 1998 - 2001  Curtis L. Olson  - http://www.flightgear.org/~curt
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


#ifndef _READERWRITERSTG_HXX
#define _READERWRITERSTG_HXX

#include <osgDB/ReaderWriter>

class SGBucket;

namespace simgear {

class ReaderWriterSTG : public osgDB::ReaderWriter {
public:
    ReaderWriterSTG();
    virtual ~ReaderWriterSTG();

    virtual const char* className() const;

    virtual ReadResult
    readNode(const std::string&, const osgDB::Options*) const;

private:
    struct _ModelBin;
};

}
#endif // _READERWRITERSTG_HXX
