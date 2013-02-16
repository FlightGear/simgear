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
#include <simgear/props/props.hxx>
#include <simgear/scene/util/NodeAndDrawableVisitor.hxx>

namespace simgear
{
class SGReaderWriterOptions;
}

osg::Texture2D*
SGLoadTexture2D(bool staticTexture, const std::string& path,
                const osgDB::Options* options = 0,
                bool wrapu = true, bool wrapv = true, int mipmaplevels = -1);

inline osg::Texture2D*
SGLoadTexture2D(const std::string& path,
                const osgDB::Options* options = 0,
                bool wrapu = true, bool wrapv = true, int mipmaplevels = -1)
{
    return SGLoadTexture2D(true, path, options, wrapu, wrapv, mipmaplevels);
}

inline osg::Texture2D*
SGLoadTexture2D(const SGPath& path,
                const osgDB::Options* options = 0,
                bool wrapu = true, bool wrapv = true,
                int mipmaplevels = -1)
{
    return SGLoadTexture2D(true, path.str(), options, wrapu, wrapv,
                           mipmaplevels);
}

inline osg::Texture2D*
SGLoadTexture2D(bool staticTexture, const SGPath& path,
                const osgDB::Options* options = 0,
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

/**
 * Transform an OSG subgraph by substituting Effects and EffectGeodes
 * for osg::Geodes with osg::StateSets. This is only guaranteed to
 * work for models prouced by the .ac loader.
 *
 * returns a copy if any nodes are changed
 */
osg::ref_ptr<osg::Node>
instantiateEffects(osg::Node* model,
                   PropertyList& effectProps,
                   const SGReaderWriterOptions* options);

/**
 * Transform an OSG subgraph by substituting the Effects and
 * EffectGeodes for osg::Geodes with osg::StateSets, inheriting from
 * the default model effect. This is only guaranteed to work for
 * models prouced by the .ac loader.
 *
 * returns a copy if any nodes are changed
 */

inline osg::ref_ptr<osg::Node>
instantiateEffects(osg::Node* model,
                   const SGReaderWriterOptions* options)
{
    PropertyList effectProps;
    return instantiateEffects(model, effectProps, options);
}
}
#endif // __MODEL_HXX
