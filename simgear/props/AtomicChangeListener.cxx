
#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include "AtomicChangeListener.hxx"

#include <algorithm>
#include <iterator>
#include <vector>

#include <simgear/structure/Singleton.hxx>

namespace simgear
{
using namespace std;

MultiChangeListener::MultiChangeListener()
{
}

void MultiChangeListener::valueChanged(SGPropertyNode* node)
{
    valueChangedImplementation();
}

void MultiChangeListener::valueChangedImplementation()
{
}

AtomicChangeListener::AtomicChangeListener(std::vector<SGPropertyNode*>& nodes)
    :  _dirty(false), _valid(true)
{
    listenToProperties(nodes.begin(), nodes.end());
}

void AtomicChangeListener::unregister_property(SGPropertyNode* node)
{
    _valid = false;
    // not necessary, but good hygine
    vector<SGPropertyNode*>::iterator itr
        = find(_watched.begin(), _watched.end(), node);
    if (itr != _watched.end())
        *itr = 0;
    MultiChangeListener::unregister_property(node);
}

void AtomicChangeListener::fireChangeListeners()
{
    vector<SGSharedPtr<AtomicChangeListener> >& listeners
        = ListenerListSingleton::instance()->listeners;
    for (vector<SGSharedPtr<AtomicChangeListener> >::iterator itr = listeners.begin(),
             end = listeners.end();
         itr != end;
         ++itr) {
        (*itr)->valuesChanged();
        (*itr)->_dirty = false;
    }
    listeners.clear();
}

void AtomicChangeListener::clearPendingChanges()
{
    auto& listeners = ListenerListSingleton::instance()->listeners;
    listeners.clear();
}

void AtomicChangeListener::valueChangedImplementation()
{
    if (!_dirty) {
        _dirty = true;
        if (_valid)
            ListenerListSingleton::instance()->listeners.push_back(this);
    }
}

void AtomicChangeListener::valuesChanged()
{
}
}
