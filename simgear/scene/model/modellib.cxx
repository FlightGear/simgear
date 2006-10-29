// modellib.cxx - implement an SSG model library.

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include <simgear/compiler.h>
#include <simgear/props/props.hxx>
#include <simgear/scene/util/SGNodeMasks.hxx>

#include "model.hxx"
#include "animation.hxx"
#include "personality.hxx"

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
    // This routine is disabled because I believe I see multiple
    // problems with it.
    //
    // 1. It blindly deletes all managed models that aren't used
    //    elsewhere.  Is this what we really want????  In the one
    //    FlightGear case that calls this method, this clearly is not the
    //    intention.  I believe it makes more sense to simply leave items
    //    in the lbrary, even if they are not currently used, they will be
    //    there already when/if we want to use them later.
    //
    // 2. This routine only does a deRef() on the model.  This doesn't actually
    //    delete the ssg tree so there is a memory leak.

    SG_LOG( SG_GENERAL, SG_ALERT,
            "WARNGING: a disabled/broken routine has been called.  This should be fixed!" );

    return;

    map<string, osg::ref_ptr<osg::Node> >::iterator it = _table.begin();
    while (it != _table.end()) {
                                // If there is only one reference, it's
                                // ours; no one else is using the item.
        if (it->second->referenceCount() <= 1) {
            string key = it->first;
            _table.erase(it);
            it = _table.upper_bound(key);
        } else
            it++;
    }
}

osg::Node*
SGModelLib::load_model( const string &fg_root,
                           const string &path,
                           SGPropertyNode *prop_root,
                           double sim_time_sec,
                           bool cache_object,
                           SGModelData *data )
{
    osg::Group *personality_branch = new SGPersonalityBranch;
    personality_branch->setName("Model Personality Group");

                                // FIXME: normalize path to
                                // avoid duplicates.
    map<string, osg::ref_ptr<osg::Node> >::iterator it = _table.find(path);
    if (!cache_object || it == _table.end()) {
        osg::ref_ptr<osg::Node> model = sgLoad3DModel(fg_root, path, prop_root,
                                                      sim_time_sec, 0, data );
        model->setName("Loaded model node");
        if (cache_object)
            _table[path] = model;      // add one reference to keep it around

        personality_branch->addChild( model.get() );
    } else {
        personality_branch->addChild( it->second.get() );
    }

    return personality_branch;
}


// end of modellib.cxx
