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

#include <osg/Node>
#include <osg/Texture2D>
#include <osgDB/ReaderWriter>

#include <simgear/misc/sg_path.hxx>
#include <simgear/scene/util/NodeAndDrawableVisitor.hxx>

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

namespace simgear
{
osg::Node* copyModel(osg::Node* model);

// Change the StateSets of a model to hold different textures based on
// a livery path.

class TextureUpdateVisitor : public NodeAndDrawableVisitor
{
public:
    TextureUpdateVisitor(const osgDB::FilePathList& pathList);
    virtual void apply(osg::Node& node);
    virtual void apply(osg::Drawable& drawable);
    // Copied from Mathias' earlier SGTextureUpdateVisitor
protected:
    osg::Texture2D* textureReplace(int unit, const osg::StateAttribute* attr);
    osg::StateSet* cloneStateSet(const osg::StateSet* stateSet);
private:
    osgDB::FilePathList _pathList;
};

// Create new userdata structs in a copied model.
// The BVH trees are shared with the original model, but the velocity fields
// should usually be distinct fields for distinct models.
class UserDataCopyVisitor : public osg::NodeVisitor
{
public:
    UserDataCopyVisitor();
    virtual void apply(osg::Node& node);
};

}
#endif // __MODEL_HXX
