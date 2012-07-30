#ifndef SIMGEAR_EXTENDEDPROPERTYADAPTER_HXX
#define SIMGEAR_EXTENDEDPROPERTYADAPTER_HXX 1

#include <algorithm>

#include <boost/bind.hpp>

#include <simgear/math/SGMath.hxx>
#include <simgear/structure/exception.hxx>

#include "props.hxx"

namespace simgear
{

namespace props
{
// This should be in simgear/math/SGVec.hxx and friends

template<typename T> struct NumComponents;

template<> struct NumComponents<SGVec3d>
{
    enum { num_components = 3 };
};

template<> struct NumComponents<SGVec4d>
{
    enum { num_components = 4 };
};

}

template<typename T, typename NodeContainer>
class ExtendedPropertyAdapter
{
public:
    enum { num_components = props::NumComponents<T>::num_components };
    ExtendedPropertyAdapter(const NodeContainer& elements)
        : _elements(elements)
    {
    }
    T operator()() const
    {
        T result;
        if (_elements.size() < num_components)
            throw sg_exception();
        for (int i = 0; i < num_components; ++i)
            result[i] = _elements[i]->template getValue<double>();
        return result;
    }
    void set(const T& val)
    {
        if (_elements.size() < num_components)
            throw sg_exception();
        for (int i = 0; i < num_components; ++i)
            _elements[i]->setValue(val[i]);
    }
private:
    const NodeContainer& _elements;
};

template<typename InIterator, typename OutIterator>
inline void makeChildList(SGPropertyNode* prop, InIterator inBegin,
                          InIterator inEnd, OutIterator outBegin)
{
    std::transform(inBegin, inEnd, outBegin,
                   boost::bind(static_cast<SGPropertyNode* (SGPropertyNode::*)(const char*, int, bool)>(&SGPropertyNode::getChild), prop, _1, 0, true));
}

}
#endif
