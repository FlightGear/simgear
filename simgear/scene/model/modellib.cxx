// modellib.cxx - implement an SSG model library.

#include <simgear/compiler.h>
#include <simgear/props/props.hxx>

#include "model.hxx"

#include "modellib.hxx"



////////////////////////////////////////////////////////////////////////
// Implementation of SGModelLib.
////////////////////////////////////////////////////////////////////////

SGModelLib::SGModelLib ()
{
}

SGModelLib::~SGModelLib ()
{
    map<string, ssgBase *>::iterator it = _table.begin();
    while (it != _table.end()) {
        it->second->deRef();
        _table.erase(it);
    }
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

    map<string, ssgBase *>::iterator it = _table.begin();
    while (it != _table.end()) {
        ssgBase *item = it->second;
                                // If there is only one reference, it's
                                // ours; no one else is using the item.
        if (item->getRef() == 1) {
            item->deRef();
            _table.erase(it);
        }
        it++;
    }
}


ssgEntity *
SGModelLib::load_model( const string &fg_root,
                           const string &path,
                           SGPropertyNode *prop_root,
                           double sim_time_sec )
{
                                // FIXME: normalize path to
                                // avoid duplicates.
    map<string, ssgBase *>::iterator it = _table.find(path);
    if (it == _table.end()) {
        ssgEntity *model = sgLoad3DModel( fg_root, path, prop_root,
                                          sim_time_sec );
        model->ref();
        _table[path] = model;      // add one reference to keep it around
        return model;
    } else {
        return (ssgEntity *)it->second;
    }
}


// end of modellib.cxx
