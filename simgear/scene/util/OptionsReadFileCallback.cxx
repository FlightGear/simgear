// Copyright (C) 2012 Mathias Froehlich
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

#include "OptionsReadFileCallback.hxx"

#include <osgDB/Registry>

namespace simgear {

OptionsReadFileCallback::~OptionsReadFileCallback()
{
}

osgDB::ReaderWriter::ReadResult
OptionsReadFileCallback::openArchive(const std::string& name, osgDB::ReaderWriter::ArchiveStatus status,
                                unsigned int indexBlockSizeHint, const osgDB::Options* options)
{
    osgDB::ReadFileCallback* callback = osgDB::Registry::instance()->getReadFileCallback();
    if (callback)
        return callback->openArchive(name, status, indexBlockSizeHint, options);
    else
        return osgDB::ReadFileCallback::openArchive(name, status, indexBlockSizeHint, options);
}

osgDB::ReaderWriter::ReadResult
OptionsReadFileCallback::readObject(const std::string& name, const osgDB::Options* options)
{
    osgDB::ReadFileCallback* callback = osgDB::Registry::instance()->getReadFileCallback();
    if (callback)
        return callback->readObject(name, options);
    else
        return osgDB::ReadFileCallback::readObject(name, options);
}

osgDB::ReaderWriter::ReadResult
OptionsReadFileCallback::readImage(const std::string& name, const osgDB::Options* options)
{
    osgDB::ReadFileCallback* callback = osgDB::Registry::instance()->getReadFileCallback();
    if (callback)
        return callback->readImage(name, options);
    else
        return osgDB::ReadFileCallback::readImage(name, options);
}

osgDB::ReaderWriter::ReadResult
OptionsReadFileCallback::readHeightField(const std::string& name, const osgDB::Options* options)
{
    osgDB::ReadFileCallback* callback = osgDB::Registry::instance()->getReadFileCallback();
    if (callback)
        return callback->readHeightField(name, options);
    else
        return osgDB::ReadFileCallback::readHeightField(name, options);
}

osgDB::ReaderWriter::ReadResult
OptionsReadFileCallback::readNode(const std::string& name, const osgDB::Options* options)
{
    osgDB::ReadFileCallback* callback = osgDB::Registry::instance()->getReadFileCallback();
    if (callback)
        return callback->readNode(name, options);
    else
        return osgDB::ReadFileCallback::readNode(name, options);
}

osgDB::ReaderWriter::ReadResult
OptionsReadFileCallback::readShader(const std::string& name, const osgDB::Options* options)
{
    osgDB::ReadFileCallback* callback = osgDB::Registry::instance()->getReadFileCallback();
    if (callback)
        return callback->readShader(name, options);
    else
        return osgDB::ReadFileCallback::readShader(name, options);
}

}
