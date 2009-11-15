// Copyright (C) 2009  Tim Moore timoore@redhat.com
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#ifndef SIMGEAR_EFFECTBUILDER_HXX
#define SIMGEAR_EFFECTBUILDER_HXX 1

#include <algorithm>
#include <map>
#include <string>
#include <cstring>

#include <osgDB/Registry>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>

#include <simgear/props/props.hxx>
#include <simgear/structure/exception.hxx>
#include <simgear/structure/SGSharedPtr.hxx>
#include <simgear/structure/Singleton.hxx>

/**
 * Support classes for parsing effects.
 */

namespace simgear
{
class Effect;
class Pass;

/**
 * Builder that returns an object, probably an OSG object.
 */
template<typename T>
class EffectBuilder : public SGReferenced
{
public:
    virtual ~EffectBuilder() {}
    virtual T* build(Effect* effect, const SGPropertyNode*,
                     const osgDB::ReaderWriter::Options* options) = 0;
    static T* buildFromType(Effect* effect, const std::string& type,
                            const SGPropertyNode*props,
                            const osgDB::ReaderWriter::Options* options)
    {
        BuilderMap& builderMap = getMap();
        typename BuilderMap::iterator iter = builderMap.find(type);
        if (iter != builderMap.end())
            return iter->second->build(effect, props, options);
        else
            return 0;
    }
    struct Registrar;
    friend struct Registrar;
    struct Registrar
    {
        Registrar(const std::string& type, EffectBuilder* builder)
        {
            getMap().insert(std::make_pair(type, builder));
        }
    };
protected:
    typedef std::map<std::string, SGSharedPtr<EffectBuilder> > BuilderMap;
    struct BuilderMapSingleton : public simgear::Singleton<BuilderMapSingleton>
    {
        BuilderMap _map;
    };
    static BuilderMap& getMap()
    {
        return BuilderMapSingleton::instance()->_map;
    }
};

// Tables of strings and constants. We want to reconstruct the effect
// property tree from OSG state sets, so the tables should be bi-directional.

// two-way map for building StateSets from property descriptions, and
// vice versa. Mostly copied from the boost documentation.

namespace effect
{
using boost::multi_index_container;
using namespace boost::multi_index;

// tags for accessing both sides of a bidirectional map

struct from{};
struct to{};

template <typename T>
struct EffectNameValue
{
    // Don't use std::pair because we want to use aggregate intialization.
    const char* first;
    T second;
};

// The class template bidirectional_map wraps the specification
// of a bidirectional map based on multi_index_container.

template<typename FromType,typename ToType>
struct bidirectional_map
{
    typedef std::pair<FromType,ToType> value_type;

    /* A bidirectional map can be simulated as a multi_index_container
     * of pairs of (FromType,ToType) with two unique indices, one
     * for each member of the pair.
     */

    typedef multi_index_container<
        value_type,
        indexed_by<
            ordered_unique<
                tag<from>, member<value_type, FromType, &value_type::first> >,
            ordered_unique<
                tag<to>,  member<value_type, ToType, &value_type::second> >
            >
        > type;
};

template<typename T>
struct EffectPropertyMap
{
    typedef typename bidirectional_map<std::string, T>::type BMap;
    BMap _map;
    template<int N>
    EffectPropertyMap(const EffectNameValue<T> (&attrs)[N]);
};

template<typename T>
template<int N>
EffectPropertyMap<T>::EffectPropertyMap(const EffectNameValue<T> (&attrs)[N])
{
    for (int i = 0; i < N; ++i)
        _map.insert(typename BMap::value_type(attrs[i].first, attrs[i].second));
}

}

template<typename T>
bool findAttr(const effect::EffectPropertyMap<T>& pMap,
              const SGPropertyNode* prop,
              T& result)
{
    using namespace effect;
    if (!prop)
        return false;
    const char* name = prop->getStringValue();
    if (!name)
        return false;
    typename EffectPropertyMap<T>::BMap::iterator itr
        = pMap._map.get<from>().find(name);
    if (itr == pMap._map.end()) {
        return false;
    } else {
        result = itr->second;
        return true;
    }
}

template<typename T>
std::string findName(const effect::EffectPropertyMap<T>& pMap, T value)
{
    using namespace effect;
    std::string result;
    typename EffectPropertyMap<T>::BMap::template index_iterator<to>::type itr
        = pMap._map.get<to>().find(value);
    if (itr != pMap._map.get<to>().end())
        result = itr->first;
    return result;
}

template<typename T>
std::string findName(const effect::EffectPropertyMap<T>& pMap, GLenum value)
{
    return findName(pMap, static_cast<T>(value));
}

/**
 * Given a property node from a pass, get its value either from it or
 * from the effect parameters.
 */

const SGPropertyNode* getEffectPropertyNode(Effect* effect,
                                            const SGPropertyNode* prop);
/**
 * Get a named child property from pass parameters or effect
 * parameters.
 */
const SGPropertyNode* getEffectPropertyChild(Effect* effect,
                                             const SGPropertyNode* prop,
                                             const char* name);

class BuilderException : public sg_exception
{
public:
    BuilderException();
    BuilderException(const char* message, const char* origin = 0);
    BuilderException(const std::string& message, const std::string& = "");
    virtual ~BuilderException() throw();
};

class PassAttributeBuilder : public SGReferenced
{
protected:
    typedef std::map<const std::string, SGSharedPtr<PassAttributeBuilder> >
    PassAttrMap;

    struct PassAttrMapSingleton : public simgear::Singleton<PassAttrMapSingleton>
    {
        PassAttrMap passAttrMap;
    };
public:
    virtual void buildAttribute(Effect* effect, Pass* pass,
                                const SGPropertyNode* prop,
                                const osgDB::ReaderWriter::Options* options)
    = 0;
    static PassAttributeBuilder* find(const std::string& str)
    {
        PassAttrMap::iterator itr
            = PassAttrMapSingleton::instance()->passAttrMap.find(str);
        if (itr == PassAttrMapSingleton::instance()->passAttrMap.end())
            return 0;
        else
            return itr->second.ptr();
    }
    template<typename T> friend class InstallAttributeBuilder;
};

template<typename T>
struct InstallAttributeBuilder
{
    InstallAttributeBuilder(const string& name)
    {
        PassAttributeBuilder::PassAttrMapSingleton::instance()
            ->passAttrMap.insert(make_pair(name, new T));
    }
};

// The description of an attribute may exist in a pass' XML, but a
// derived effect might want to disable the attribute altogether. So,
// some attributes have an "active" property; if it exists and is
// false, the OSG attribute is not built at all. This is different
// from any OSG mode settings that might be around.
bool isAttributeActive(Effect* effect, const SGPropertyNode* prop);
}
#endif
