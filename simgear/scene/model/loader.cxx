// loader.cxx - implement SSG model and texture loaders.

#include <simgear/compiler.h>
#include <simgear/props/props.hxx>

#include "loader.hxx"
#include "model.hxx"



////////////////////////////////////////////////////////////////////////
// Implementation of SGssgLoader.
////////////////////////////////////////////////////////////////////////

SGssgLoader::SGssgLoader ()
{
    // no op
}

SGssgLoader::~SGssgLoader ()
{
    std::map<string, ssgBase *>::iterator it = _table.begin();
    while (it != _table.end()) {
        it->second->deRef();
        _table.erase(it);
    }
}

void
SGssgLoader::flush ()
{
    std::map<string, ssgBase *>::iterator it = _table.begin();
    while (it != _table.end()) {
        ssgBase * item = it->second;
                                // If there is only one reference, it's
                                // ours; no one else is using the item.
        if (item->getRef() == 1) {
            item->deRef();
            _table.erase(it);
        }
        it++;
    }
}



////////////////////////////////////////////////////////////////////////
// Implementation of SGModelLoader.
////////////////////////////////////////////////////////////////////////

SGModelLoader::SGModelLoader ()
{
}

SGModelLoader::~SGModelLoader ()
{
}

ssgEntity *
SGModelLoader::load_model( const string &fg_root,
                           const string &path,
                           SGPropertyNode *prop_root,
                           double sim_time_sec )
{
                                // FIXME: normalize path to
                                // avoid duplicates.
    std::map<string, ssgBase *>::iterator it = _table.find(path);
    if (it == _table.end()) {
        _table[path] = sgLoad3DModel( fg_root, path, prop_root, sim_time_sec );
        it = _table.find(path);
        it->second->ref();      // add one reference to keep it around
    }
    return (ssgEntity *)it->second;
}


// end of loader.cxx
