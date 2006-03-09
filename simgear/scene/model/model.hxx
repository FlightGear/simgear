// model.hxx - manage a 3D aircraft model.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain, and comes with no warranty.

#ifndef __MODEL_HXX
#define __MODEL_HXX 1

#ifndef __cplusplus
# error This library requires C++
#endif

#include <vector>
#include <set>

SG_USING_STD(vector);
SG_USING_STD(set);

#include <plib/sg.h>
#include <plib/ssg.h>

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
class SGModelData : public ssgBase {
public:
    virtual ~SGModelData() {}
    virtual void modelLoaded( const string& path, SGPropertyNode *prop,
                              ssgBranch *branch) {}
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
ssgBranch *
sgLoad3DModel( const string& fg_root, const string &path,
                          SGPropertyNode *prop_root, double sim_time_sec,
                          ssgEntity *(*load_panel)(SGPropertyNode *) = 0,
                          SGModelData *data = 0 );


/**
 * Make an offset matrix from rotations and position offset.
 */
void
sgMakeOffsetsMatrix( sgMat4 * result, double h_rot, double p_rot, double r_rot,
                     double x_off, double y_off, double z_off );

/**
 * Make the animation
 */
void
sgMakeAnimation( ssgBranch * model,
                 const char * name,
                 vector<SGPropertyNode_ptr> &name_nodes,
                 SGPropertyNode *prop_root,
                 SGPropertyNode_ptr node,
                 double sim_time_sec,
                 SGPath &texture_path,
                 set<ssgBranch *> &ignore_branches );

/**
 * Set the filter state on models
 */
bool
sgSetModelFilter( bool filter );

/**
 * Check if the ssg node contains an animation
 */
bool 
sgCheckAnimationBranch (ssgEntity * entity);

/**
 * Enable or disable Display list usage
 */
extern bool sgUseDisplayList;

#endif // __MODEL_HXX
