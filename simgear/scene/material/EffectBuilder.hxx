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

// Simple tables of strings and constants. The table intialization
// *must* be in alphabetical order.
template <typename T>
struct EffectNameValue
{
    // Don't use std::pair because we want to use aggregate intialization.

    const char* first;
    T second;
    class Compare
    {
    private:
        static bool compare(const char* lhs, const char* rhs)
        {
            return std::strcmp(lhs, rhs) < 0;
        }
    public:
        bool operator()(const EffectNameValue& lhs,
                        const EffectNameValue& rhs) const
        {
            return compare(lhs.first, rhs.first);
        }
        bool operator()(const char* lhs, const EffectNameValue& rhs) const
        {
            return compare(lhs, rhs.first);
        }
        bool operator()(const EffectNameValue& lhs, const char* rhs) const
        {
            return compare (lhs.first, rhs);
        }
    };
};

template<typename ENV, typename T, int N>
bool findAttr(const ENV (&attrs)[N], const SGPropertyNode* prop, T& result)
{
    if (!prop)
        return false;
    const char* name = prop->getStringValue();
    if (!name)
        return false;
    std::pair<const ENV*, const ENV*> itrs
        = std::equal_range(&attrs[0], &attrs[N], name, typename ENV::Compare());
    if (itrs.first == itrs.second) {
        return false;
    } else {
        result = itrs.first->second;
        return true;
    }
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
}
#endif
