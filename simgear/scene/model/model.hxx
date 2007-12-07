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

SG_USING_STD(vector);
SG_USING_STD(set);

#include <osg/Node>
#include <osg/Texture2D>
#include <osgDB/ReaderWriter>

#include <simgear/misc/sg_path.hxx>
#include <simgear/props/props.hxx>


// Has anyone done anything *really* stupid, like making min and max macros?
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif


/**
 * Abstract class for adding data to the scene graph.  modelLoaded() is
 * called by sgLoad3DModel() after the model was loaded, and the destructor
 * when the branch is removed from the graph.
 */
class SGModelData : public osg::Referenced {
public:
    virtual ~SGModelData() {}
    virtual void modelLoaded( const string& path, SGPropertyNode *prop,
                              osg::Node*branch) = 0;
};


/**
 * Load a 3D model with or without XML wrapper.  Note, this version
 * Does not know about or load the panel/cockpit information.  Use the
 * "model_panel.hxx" version if you want to load an aircraft
 * (i.e. ownship) with a panel.
 *
 * If the path ends in ".xml", then it will be used as a property-
 * list wrapper to add animations to the model.
 *
 * Subsystems should not normally invoke this function directly;
 * instead, they should use the FGModelLoader declared in loader.hxx.
 */
osg::Node*
sgLoad3DModel( const string& fg_root, const string &path,
               SGPropertyNode *prop_root, double sim_time_sec,
               osg::Node *(*load_panel)(SGPropertyNode *) = 0,
               SGModelData *data = 0,
               const SGPath& texturePath = SGPath() );


/**
 * Make the animation
 */
void
sgMakeAnimation( osg::Node* model,
                 const char * name,
                 vector<SGPropertyNode_ptr> &name_nodes,
                 SGPropertyNode *prop_root,
                 SGPropertyNode_ptr node,
                 double sim_time_sec,
                 SGPath &texture_path,
                 set<osg::Node*> &ignore_branches );

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
