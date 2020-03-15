// Written by Thorsten Brehm, started November 2012.
//
// Copyright (C) 2012  Thorsten Brehm, brehmt at gmail dt com
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

/**
 * GZ Container File Format
 *
 * A (gzipped) binary file.
 *
 * Byte-order is little endian (we're ignoring big endian machines - for now...,
 * - still usable, but they are writing an incompatible format).
 *
 * The binary file may contain an arbitrary number of containers.
 *
 * Each container has three elements:
 *  - container type
 *  - container payload size
 *  - container payload data (of given "size" in bytes)
 *
 * A container may consist of strings, property trees, raw binary data, and whatever else
 * is implemented.
 *
 * Use this for storing/loading user-data only - not for distribution files (scenery, aircraft etc).
 */

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include "gzcontainerfile.hxx"

#include <simgear/props/props_io.hxx>
#include <simgear/misc/stdint.hxx>
#include <simgear/misc/sg_path.hxx>

#include <string.h>

using namespace std;
using namespace simgear;

#if 1
    #define MY_SG_DEBUG SG_DEBUG
#else
    #define MY_SG_DEBUG SG_ALERT
#endif

/** The header is always the first container in each file, so it's ID doesn't
 * really matter (as long as it isn't changed) - it cannot conflict with user-specific types. */
const ContainerType HeaderType = 0;

/** Magic word to detect big/little-endian file formats */
static const uint32_t EndianMagic = 0x11223344;

/**************************************************************************
 * gzContainerWriter
 **************************************************************************/

gzContainerWriter::gzContainerWriter(const SGPath& name,
                                     const std::string& fileMagic) :
        sg_gzofstream(name, ios_out | ios_binary),
        filename(name.utf8Str())
{
    /* write byte-order marker **************************************/
    write((char*)&EndianMagic, sizeof(EndianMagic));

    /* write file header ********************************************/
    writeContainer(HeaderType, fileMagic.c_str());
}

/** Write the header of a single container. */
bool
gzContainerWriter::writeContainerHeader(ContainerType Type, size_t Size)
{
    uint32_t ui32Data;
    uint64_t ui64Data;

    SG_LOG(SG_IO, MY_SG_DEBUG, "Writing container " << Type << ", size " << Size);

    // write container type
    ui32Data = Type;
    write((char*)&ui32Data, sizeof(ui32Data));
    if (fail())
        return false;

    // write container payload size
    ui64Data = Size;
    write((char*)&ui64Data, sizeof(ui64Data));
    return !fail();
}

/** Write a complete container. */
bool
gzContainerWriter::writeContainer(ContainerType Type, const char* pData, size_t Size)
{
    // write container type and size
    if (!writeContainerHeader(Type, Size))
        return false;

    // write container payload
    write(pData, Size);
    return !fail();
}


/** Save a single string in a separate container */
bool
gzContainerWriter::writeContainer(ContainerType Type, const char* stringBuffer)
{
    return writeContainer(Type, stringBuffer, strlen(stringBuffer)+1);
}

/** Save a property tree in a separate container */
bool
gzContainerWriter::writeContainer(ContainerType Type, SGPropertyNode* root)
{
    stringstream oss;
    writeProperties(oss, root, true);
    return writeContainer(Type, oss.str().c_str());
}

/**************************************************************************
 * gzContainerReader
 **************************************************************************/

gzContainerReader::gzContainerReader(const SGPath& name,
                                     const std::string& fileMagic) :
        sg_gzifstream(SGPath(name), ios_in | ios_binary),
        filename(name.utf8Str())
{
    bool ok = (good() && !eof());

    /* check byte-order marker **************************************/
    if (ok)
    {
        uint32_t EndianCheck;
        read((char*)&EndianCheck, sizeof(EndianCheck));
        if (eof() || !good())
        {
            SG_LOG(SG_IO, SG_ALERT, "Error reading file " << name);
            ok = false;
        }
        else
        if (EndianCheck != EndianMagic)
        {
            SG_LOG(SG_IO, SG_ALERT, "Byte-order mismatch. This file was created for another architecture: " << filename);
            ok = false;
        }
    }

    /* read file header *********************************************/
    if (ok)
    {
        char* FileHeader = NULL;
        size_t Size = 0;
        ContainerType Type = -1;
        if (!readContainer(&Type, &FileHeader, &Size))
        {
            SG_LOG(SG_IO, SG_ALERT, "File format not recognized. Missing file header: " << filename);
            ok = false;
        }
        else
        if ((HeaderType != Type)||(strcmp(FileHeader, fileMagic.c_str())))
        {
            SG_LOG(SG_IO, MY_SG_DEBUG, "Invalid header. Container type " << Type << ", Header " <<
                   ((FileHeader) ? FileHeader : "(none)"));
            SG_LOG(SG_IO, SG_ALERT, "File not recognized. This is not a valid '" << fileMagic << "' file: " << filename);
            ok = false;
        }

        if (FileHeader)
        {
            free(FileHeader);
            FileHeader = NULL;
        }
    }

    if (!ok)
    {
        setstate(badbit);
    }
}

/** Read the header of a single container. */
bool
gzContainerReader::readContainerHeader(ContainerType* pType, size_t* pSize)
{
    uint32_t ui32Data;
    uint64_t ui64Data;

    *pSize = 0;
    *pType = -1;

    read((char*) &ui32Data, sizeof(ui32Data));
    if (eof())
    {
        SG_LOG(SG_IO, SG_ALERT, "File corrupt? Unexpected end of file when reading container type in " << filename);
        return false;
    }
    *pType = (ContainerType) ui32Data;

    read((char*) &ui64Data, sizeof(ui64Data));
    if (eof())
    {
        SG_LOG(SG_IO, SG_ALERT, "File corrupt? Unexpected end of file when reading container size in " << filename);
        return false;
    }
    *pSize = ui64Data;

    return !fail();
}

/** Read a single container and return its contents as a binary blob */
bool
gzContainerReader::readContainer(ContainerType* pType, char** ppData, size_t* pSize)
{
    size_t Size = 0;

    *ppData = 0;

    if (!readContainerHeader(pType, &Size))
    {
        SG_LOG(SG_IO, SG_ALERT, "Cannot load data. Invalid container header in " << filename);
        return false;
    }

    char* pData = (char*) malloc(Size);
    if (!pData)
    {
        SG_LOG(SG_IO, SG_ALERT, "Cannot load data. No more memory when reading " << filename);
        return false;
    }

    read(pData, Size);
    if (fail())
    {
        SG_LOG(SG_IO, SG_ALERT, "File corrupt? Unexpected end of file when reading " << filename);
        free(pData);
        pData = 0;
        return false;
    }

    *ppData = pData;
    *pSize = Size;
    return true;
}
