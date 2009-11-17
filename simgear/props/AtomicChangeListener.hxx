#ifndef SIMGEAR_ATOMICCHANGELISTENER_HXX
#define SIMGEAR_ATOMICCHANGELISTENER_HXX 1

#include <algorithm>
#include <iterator>
#include <vector>

#include <boost/bind.hpp>

#include <simgear/structure/Singleton.hxx>

#include "props.hxx"
#include "ExtendedPropertyAdapter.hxx"

namespace simgear
{
// Performs an action when one of several nodes changes
class MultiChangeListener : public SGPropertyChangeListener
{
public:
    MultiChangeListener();
    template<typename Pitr>
    void listenToProperties(Pitr propsBegin, Pitr propsEnd)
    {
        for (Pitr itr = propsBegin, end = propsEnd; itr != end; ++itr)
            (*itr)->addChangeListener(this);
    }
private:
    void valueChanged(SGPropertyNode* node);
    virtual void valueChangedImplementation();

};

class AtomicChangeListener : public MultiChangeListener,
                             public virtual SGReferenced
{
public:
    AtomicChangeListener(std::vector<SGPropertyNode*>& nodes);
    /**
     * Lookup / create child nodes from their relative names.
     */
    template<typename Itr>
    AtomicChangeListener(SGPropertyNode* parent, Itr childNamesBegin,
                         Itr childNamesEnd)
        : _dirty(false), _valid(true)
    {
        using namespace std;
        for (Itr itr = childNamesBegin, end = childNamesEnd;
             itr != end;
             ++itr)
            _watched.push_back(makeNode(parent, *itr));
        listenToProperties(_watched.begin(), _watched.end());
    }
    bool isDirty() { return _dirty; }
    bool isValid() { return _valid; }
    void unregister_property(SGPropertyNode* node);
    static void fireChangeListeners();
private:
    virtual void valueChangedImplementation();
    virtual void valuesChanged();
    bool _dirty;
    bool _valid;
    struct ListenerListSingleton : public Singleton<ListenerListSingleton>
    {
        std::vector<SGSharedPtr<AtomicChangeListener> > listeners;
    };
protected:
    std::vector<SGPropertyNode*> _watched;
};

template<typename T, typename Func>
class ExtendedPropListener : public AtomicChangeListener
{
public:
    ExtendedPropListener(std::vector<SGPropertyNode*>& nodes, const Func& func,
                         bool initial = false)
        : AtomicChangeListener(nodes), _func(func)
    {
        if (initial)
            valuesChanged();

    }
    template<typename Itr>
    ExtendedPropListener(SGPropertyNode* parent, Itr childNamesBegin,
                         Itr childNamesEnd, const Func& func,
                         bool initial = false)
        : AtomicChangeListener(parent, childNamesBegin, childNamesEnd),
          _func(func)
    {
        if (initial)
            valuesChanged();
    }
    virtual void valuesChanged()
    {
        ExtendedPropertyAdapter<T, std::vector<SGPropertyNode*> > adaptor(_watched);
        T val = adaptor();
        _func(val);
    }
private:
    Func _func;
};

}
#endif
