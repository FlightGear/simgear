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

#ifndef GZ_CONTAINER_FILE_HXX
#define GZ_CONTAINER_FILE_HXX

#include <string>
#include <simgear/misc/sgstream.hxx>

class SGPropertyNode;

namespace simgear
{

typedef int ContainerType;

/** A class to write container files. */
class gzContainerReader : public sg_gzifstream
{
public:
    gzContainerReader( const std::string& name,
                       const std::string& fileMagic);

    bool readContainerHeader(ContainerType* pType, size_t* pSize);
    bool readContainer(ContainerType* pType, char** ppData, size_t* pSize);
private:
    std::string filename;
};

/** A class to read container files. */
class gzContainerWriter : public sg_gzofstream
{
public:
    gzContainerWriter( const std::string& name,
                       const std::string& fileMagic);

    bool writeContainerHeader(ContainerType Type, size_t Size);
    bool writeContainer(ContainerType Type, const char* pData, size_t Size);
    bool writeContainer(ContainerType Type, const char* stringBuffer);
    bool writeContainer(ContainerType Type, SGPropertyNode* root);
private:
    std::string filename;
};

}

#endif // GZ_CONTAINER_FILE_HXX
