// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2012 Thorsten Brehm <brehmt@gmail.com>

/**
 * @file
 * @brief GZ Container File Format
 */

#ifndef GZ_CONTAINER_FILE_HXX
#define GZ_CONTAINER_FILE_HXX

#include <string>
#include <simgear/io/iostreams/sgstream.hxx>

class SGPropertyNode;

namespace simgear
{

typedef int ContainerType;

/** A class to write container files. */
class gzContainerReader : public sg_gzifstream
{
public:
    gzContainerReader( const SGPath& name,
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
    gzContainerWriter( const SGPath& name,
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
