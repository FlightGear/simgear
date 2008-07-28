// model.hxx - manage a 3D aircraft model.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain, and comes with no warranty.

#ifndef __MODEL_HXX
#define __MODEL_HXX 1

#ifndef __cplusplus
# error This library requires C++
#endif

#include <simgear/compiler.h>

#include <vector>
#include <set>

using std::vector;
using std::set;

#include <osg/Node>
#include <osg/Texture2D>
#include <osgDB/ReaderWriter>

#include <simgear/misc/sg_path.hxx>

osg::Texture2D*
SGLoadTexture2D(bool staticTexture, const std::string& path,
                const osgDB::ReaderWriter::Options* options = 0,
                bool wrapu = true, bool wrapv = true, int mipmaplevels = -1);

inline osg::Texture2D*
SGLoadTexture2D(const std::string& path,
                const osgDB::ReaderWriter::Options* options = 0,
                bool wrapu = true, bool wrapv = true, int mipmaplevels = -1)
{
    return SGLoadTexture2D(true, path, options, wrapu, wrapv, mipmaplevels);
}

inline osg::Texture2D*
SGLoadTexture2D(const SGPath& path,
                const osgDB::ReaderWriter::Options* options = 0,
                bool wrapu = true, bool wrapv = true,
                int mipmaplevels = -1)
{
    return SGLoadTexture2D(true, path.str(), options, wrapu, wrapv,
                           mipmaplevels);
}

inline osg::Texture2D*
SGLoadTexture2D(bool staticTexture, const SGPath& path,
                const osgDB::ReaderWriter::Options* options = 0,
                bool wrapu = true, bool wrapv = true,
                int mipmaplevels = -1)
{
    return SGLoadTexture2D(staticTexture, path.str(), options, wrapu, wrapv,
                           mipmaplevels);
}

#endif // __MODEL_HXX
