// tileentry.cxx -- routines to handle a scenery tile
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
//

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include <simgear/scene/model/ModelRegistry.hxx>

#include <osgDB/Registry>
#include <osgDB/FileNameUtils>

#include "TileEntry.hxx"
#include "ReaderWriterSTG.hxx"

using namespace simgear;

ReaderWriterSTG::ReaderWriterSTG()
{
    supportsExtension("stg", "SimGear stg database format");
}

ReaderWriterSTG::~ReaderWriterSTG()
{
}

const char* ReaderWriterSTG::className() const
{
    return "STG Database reader";
}

//#define SLOW_PAGER 1
#ifdef SLOW_PAGER
#include <unistd.h>
#endif

osgDB::ReaderWriter::ReadResult
ReaderWriterSTG::readNode(const std::string& fileName,
                          const osgDB::ReaderWriter::Options* options) const
{
    osg::Node* result = TileEntry::loadTileByFileName(fileName, options);
    // For debugging race conditions
#ifdef SLOW_PAGER
    sleep(5);
#endif
    if (result)
        return result;
    else
        return ReadResult::FILE_NOT_HANDLED;
}

