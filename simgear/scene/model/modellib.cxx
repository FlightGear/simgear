// modellib.cxx - implement an SSG model library.

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include <simgear/compiler.h>
#include <simgear/props/props.hxx>
#include <simgear/scene/util/SGNodeMasks.hxx>

#include "model.hxx"
#include "animation.hxx"

#include "modellib.hxx"



////////////////////////////////////////////////////////////////////////
// Implementation of SGModelLib.
////////////////////////////////////////////////////////////////////////

SGModelLib::SGModelLib ()
{
}

SGModelLib::~SGModelLib ()
{
}

void
SGModelLib::flush1()
{
}

osg::Node*
SGModelLib::load_model( const string &fg_root,
                           const string &path,
                           SGPropertyNode *prop_root,
                           double sim_time_sec,
                           bool cache_object,
                           SGModelData *data )
{
  return sgLoad3DModel(fg_root, path, prop_root, sim_time_sec, 0, data );
}


// end of modellib.cxx
