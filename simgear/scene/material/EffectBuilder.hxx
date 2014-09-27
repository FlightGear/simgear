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
#include <iterator>
#include <map>
#include <string>
#include <cstring>

#include <osg/Object>
#include <osgDB/Registry>

#include <boost/bind.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>

#include <simgear/props/AtomicChangeListener.hxx>
#include <simgear/props/props.hxx>
#include <simgear/scene/util/SGReaderWriterOptions.hxx>
#include <simgear/structure/exception.hxx>
#include <simgear/structure/SGSharedPtr.hxx>
#include <simgear/structure/Singleton.hxx>

#include "Effect.hxx"
/**
 * Support classes for parsing effects.
 */

namespace simgear
{
class Effect;
class Pass;
class SGReaderWriterOptions;

/**
 * Builder that returns an object, probably an OSG object.
 */
template<typename T>
class EffectBuilder : public SGReferenced
{
public:
    virtual ~EffectBuilder() {}
    virtual T* build(Effect* effect, Pass* pass, const SGPropertyNode*,
                     const SGReaderWriterOptions* options) = 0;
    static T* buildFromType(Effect* effect, Pass* pass, const std::string& type,
                            const SGPropertyNode*props,
                            const SGReaderWriterOptions* options)
    {
        BuilderMap& builderMap = getMap();
        typename BuilderMap::iterator iter = builderMap.find(type);
        if (iter != builderMap.end())
            return iter->second->build(effect, pass, props, options);
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
#if _MSC_VER >= 1600
    struct value_type {
        FromType first;
        ToType second;
        value_type(FromType f, ToType s) : first(f),second(s){}
    };
#else
    typedef std::pair<FromType,ToType> value_type;
#endif

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

// A one-way map that can be initialized using an array
template<typename T>
struct SimplePropertyMap
{
    typedef std::map<std::string, T> map_type;
    map_type _map;
    template<int N>
    SimplePropertyMap(const EffectNameValue<T> (&attrs)[N])
    {
        for (int i = 0; i < N; ++i)
        _map.insert(typename map_type::value_type(attrs[i].first,
                                                  attrs[i].second));
    }
};

class BuilderException : public sg_exception
{
public:
    BuilderException();
    BuilderException(const char* message, const char* origin = 0);
    BuilderException(const std::string& message, const std::string& = "");
    virtual ~BuilderException() throw();
};
}

template<typename T>
void findAttr(const effect::EffectPropertyMap<T>& pMap,
              const char* name,
              T& result)
{
    using namespace effect;
    typename EffectPropertyMap<T>::BMap::iterator itr
        = pMap._map.template get<from>().find(name);
    if (itr == pMap._map.end()) {
        throw effect::BuilderException(std::string("findAttr: could not find attribute ")
                                       + std::string(name));
    } else {
        result = itr->second;
    }
}

template<typename T>
inline void findAttr(const effect::EffectPropertyMap<T>& pMap,
                     const std::string& name,
                     T& result)
{
    findAttr(pMap, name.c_str(), result);
}

template<typename T>
void findAttr(const effect::EffectPropertyMap<T>& pMap,
              const SGPropertyNode* prop,
              T& result)
{
    if (!prop)
        throw effect::BuilderException("findAttr: empty property");
    const char* name = prop->getStringValue();
    if (!name)
        throw effect::BuilderException("findAttr: no name for lookup");
    findAttr(pMap, name, result);
}

// Versions that don't throw an error

template<typename T>
const T* findAttr(const effect::EffectPropertyMap<T>& pMap,
                  const char* name)
{
    using namespace effect;
    typename EffectPropertyMap<T>::BMap::iterator itr
        = pMap._map.template get<from>().find(name);
    if (itr == pMap._map.end())
        return 0;
    else 
        return &itr->second;
}

template<typename T>
const T* findAttr(const effect::SimplePropertyMap<T>& pMap,
                  const char* name)
{
    using namespace effect;
    typename SimplePropertyMap<T>::map_type::const_iterator itr
        = pMap._map.find(name);
    if (itr == pMap._map.end())
        return 0;
    else 
        return &itr->second;
}

template<typename T, template<class> class Map>
const T* findAttr(const Map<T>& pMap,
                     const std::string& name)
{
    return findAttr(pMap, name.c_str());
}


template<typename T>
std::string findName(const effect::EffectPropertyMap<T>& pMap, T value)
{
    using namespace effect;
    std::string result;
    typename EffectPropertyMap<T>::BMap::template index_iterator<to>::type itr
        = pMap._map.template get<to>().find(value);
    if (itr != pMap._map.template get<to>().end())
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

/**
 * Get the name of a node mentioned in a \<use\> clause from the global property
 * tree.
 * @return empty if prop doesn't contain a \<use\> clause; otherwise the
 * mentioned node name.
 */
std::string getGlobalProperty(const SGPropertyNode* prop,
                              const SGReaderWriterOptions *);

template<typename NameItr>
std::vector<std::string>
getVectorProperties(const SGPropertyNode* prop,
                    const SGReaderWriterOptions *options, size_t vecSize,
                    NameItr defaultNames)
{
    using namespace std;
    vector<string> result;
    if (!prop)
        return result;
    PropertyList useProps = prop->getChildren("use");
    if (useProps.size() == 1) {
        string parentName = useProps[0]->getStringValue();
        if (parentName.empty() || parentName[0] != '/')
            parentName = options->getPropertyNode()->getPath() + "/" + parentName;
        if (parentName[parentName.size() - 1] != '/')
            parentName.append("/");
        NameItr itr = defaultNames;
        for (size_t i = 0; i < vecSize; ++i, ++itr)
            result.push_back(parentName + *itr);
    } else if (useProps.size() == vecSize) {
        string parentName = useProps[0]->getStringValue();
        parentName += "/";
        for (PropertyList::const_iterator itr = useProps.begin(),
                 end = useProps.end();
             itr != end;
             ++itr) {
            string childName = (*itr)->getStringValue();
            if (childName.empty() || childName[0] != '/')
                result.push_back(parentName + childName);
            else
                result.push_back(childName);
        }
    }
    return result;
}

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
    virtual ~PassAttributeBuilder(); // anchor into the compilation unit.
  
    virtual void buildAttribute(Effect* effect, Pass* pass,
                                const SGPropertyNode* prop,
                                const SGReaderWriterOptions* options)
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
    template<typename T> friend struct InstallAttributeBuilder;
};

template<typename T>
struct InstallAttributeBuilder
{
    InstallAttributeBuilder(const std::string& name)
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

namespace effect
{
/**
 * Bridge between types stored in properties and what OSG or the
 * effects code want.
 */
template<typename T> struct Bridge;

/**
 * Default just passes on the same type.
 *
 */
template<typename T>
struct Bridge
{
    typedef T sg_type;
    static T get(const T& val) { return val; }
};

template<typename T>
struct Bridge<const T> : public Bridge<T>
{
};

// Save some typing...
template<typename InType, typename OutType>
struct BridgeOSGVec
{
    typedef InType sg_type;
    static OutType get(const InType& val) { return toOsg(val); }
};
template<>
struct Bridge<osg::Vec3f> : public BridgeOSGVec<SGVec3d, osg::Vec3f>
{
};

template<>
struct Bridge<osg::Vec3d> : public BridgeOSGVec<SGVec3d, osg::Vec3d>
{
};

template<>
struct Bridge<osg::Vec4f> : public BridgeOSGVec<SGVec4d, osg::Vec4f>
{
};

template<>
struct Bridge<osg::Vec4d> : public BridgeOSGVec<SGVec4d, osg::Vec4d>
{
};

/**
 * Functor for calling a function on an osg::Referenced object and a
 * value (e.g., an SGVec4d from a property) which will be converted to
 * the equivalent OSG type.
 *
 * General version, function takes obj, val
 */
template<typename OSGParam, typename Obj, typename Func>
struct OSGFunctor : public Bridge<OSGParam>
{
    OSGFunctor(Obj* obj, const Func& func)
        : _obj(obj), _func(func) {}
    void operator()(const typename Bridge<OSGParam>::sg_type& val) const
    {
        _func(_obj, this->get(val));
    }
    osg::ref_ptr<Obj>_obj;
    const Func _func;
};

/**
 * Version which uses a pointer to member function instead.
 */
template<typename OSGParam, typename Obj>
struct OSGFunctor<OSGParam, Obj, void (Obj::* const)(const OSGParam&)>
    : public Bridge<OSGParam>
{
    typedef void (Obj::*const MemFunc)(const OSGParam&);
    OSGFunctor(Obj* obj, MemFunc func)
        : _obj(obj), _func(func) {}
    void operator()(const typename Bridge<OSGParam>::sg_type& val) const
    {
        (_obj->*_func)(this->get(val));
    }
    osg::ref_ptr<Obj>_obj;
    MemFunc _func;
};

/**
 * Typical convenience constructors
 */
template<typename OSGParam, typename Obj, typename Func>
OSGFunctor<OSGParam, Obj, Func> make_OSGFunctor(Obj* obj, const Func& func)
{
    return OSGFunctor<OSGParam, Obj, Func>(obj, func);
}

template<typename OSGParam, typename Obj>
OSGFunctor<OSGParam, Obj, void (Obj::*const)(const OSGParam&)>
make_OSGFunctor(Obj* obj, void (Obj::*const func)(const OSGParam&))
{
    return OSGFunctor<OSGParam, Obj,
        void (Obj::* const)(const OSGParam&)>(obj, func);
}

template<typename OSGParamType, typename ObjType, typename F>
class ScalarChangeListener
    : public SGPropertyChangeListener, public DeferredPropertyListener
{
public:
    ScalarChangeListener(ObjType* obj, const F& setter,
                         const std::string& propName)
        : _obj(obj), _setter(setter)
    {
        _propName = new std::string(propName);
    	SG_LOG(SG_GL,SG_DEBUG,"Creating ScalarChangeListener for " << *_propName );
    }
    virtual ~ScalarChangeListener()
    {
        delete _propName;
        _propName = 0;
    }
    void valueChanged(SGPropertyNode* node)
    {
        _setter(_obj.get(), node->getValue<OSGParamType>());
    }
    void activate(SGPropertyNode* propRoot)
    {
        SG_LOG(SG_GL,SG_DEBUG,"Adding change listener to " << *_propName );
        SGPropertyNode* listenProp = makeNode(propRoot, *_propName);
        delete _propName;
        _propName = 0;
        if (listenProp)
            listenProp->addChangeListener(this, true);
    }
private:
    osg::ref_ptr<ObjType> _obj;
    F _setter;
    std::string* _propName;
};

template<typename T, typename Func>
class EffectExtendedPropListener : public DeferredPropertyListener
{
public:
    template<typename Itr>
    EffectExtendedPropListener(const Func& func,
                               const std::string* propName, Itr childNamesBegin,
                               Itr childNamesEnd)
        : _propName(0), _func(func)
    {
        if (propName)
            _propName = new std::string(*propName);
        _childNames = new std::vector<std::string>(childNamesBegin,
                                                   childNamesEnd);
    }
    virtual ~EffectExtendedPropListener()
    {
        delete _propName;
        delete _childNames;
    }
    void activate(SGPropertyNode* propRoot)
    {
        SGPropertyNode* parent = 0;
        if (_propName)
            parent = propRoot->getNode(*_propName, true);
        else
            parent = propRoot;
        _propListener
            = new ExtendedPropListener<T, Func>(parent, _childNames->begin(),
                                                _childNames->end(),
                                                _func, true);
        delete _propName;
        _propName = 0;
        delete _childNames;
        _childNames = 0;
    }
private:
    std::string* _propName;
    std::vector<std::string>* _childNames;
    SGSharedPtr<ExtendedPropListener<T, Func> > _propListener;
    Func _func;
};

template<typename T, typename Func, typename Itr>
DeferredPropertyListener*
new_EEPropListener(const Func& func, const std::string* propName,
                   const Itr& namesBegin, const Itr& namesEnd)
{
    return new EffectExtendedPropListener<T, Func>
        (func, 0, namesBegin, namesEnd);
}

/**
 * Set DYNAMIC data variance on an osg::Object.
 */

inline void setDynamicVariance(void* obj)
{
}

inline void setDynamicVariance(osg::Object* obj)
{
    obj->setDataVariance(osg::Object::DYNAMIC);
}

/**
 * Initialize the value and the possible updating of an effect
 * attribute. If the value is specified directly, set it. Otherwise,
 * use the \<use\> tag to look at the parameters. Again, if there is a
 * value there set it directly. Otherwise, the parameter contains its
 * own \<use\> tag referring to a property in the global property tree;
 * install a change listener that will set the attribute when the
 * property changes.
 *
 * For relative property names, the property root found in options is
 * used.
 */
template<typename OSGParamType, typename ObjType, typename F>
void
initFromParameters(Effect* effect, const SGPropertyNode* prop, ObjType* obj,
                   const F& setter, const SGReaderWriterOptions* options)
{
    const SGPropertyNode* valProp = getEffectPropertyNode(effect, prop);
    if (!valProp)
        return;
    if (valProp->nChildren() == 0) {
        setter(obj, valProp->getValue<OSGParamType>());
    } else {
        setDynamicVariance(obj);
        std::string propName = getGlobalProperty(valProp, options);
        ScalarChangeListener<OSGParamType, ObjType, F>* listener
            = new ScalarChangeListener<OSGParamType, ObjType, F>(obj, setter,
                                                                 propName);
        effect->addDeferredPropertyListener(listener);
    }
    return;
}

template<typename OSGParamType, typename ObjType, typename SetterReturn>
inline void
initFromParameters(Effect* effect, const SGPropertyNode* prop, ObjType* obj,
                   SetterReturn (ObjType::*setter)(const OSGParamType),
                   const SGReaderWriterOptions* options)
{
	initFromParameters<OSGParamType>(effect, prop, obj,
                                     boost::bind(setter, _1, _2), options);
}

/*
 * Initialize vector parameters from individual properties.
 * The parameter may be updated at runtime.
 *
 * If the value is specified directly, set it. Otherwise, use the
 * \<use\> tag to look at the parameters. Again, if there is a value
 * there set it directly. Otherwise, the parameter contains one or several
 * \<use\> tags. If there is one tag, it is a property that is the root
 * for the values needed to update the parameter; nameIter holds the
 * names of the properties relative to the root. If there are several
 * \<use\> tags, they each hold the name of the property holding the
 * value for the corresponding vector member.
 *
 * Install a change listener that will set the attribute when the
 * property changes.
 *
 * For relative property names, the property root found in options is
 * used.
 */
template<typename OSGParamType, typename ObjType, typename NameItrType,
         typename F>
void
initFromParameters(Effect* effect, const SGPropertyNode* prop, ObjType* obj,
                   const F& setter,
                   NameItrType nameItr, const SGReaderWriterOptions* options)
{
    typedef typename Bridge<OSGParamType>::sg_type sg_type;
    const int numComponents = props::NumComponents<sg_type>::num_components;
    const SGPropertyNode* valProp = getEffectPropertyNode(effect, prop);
    if (!valProp)
        return;
    if (valProp->nChildren() == 0) { // Has <use>?
        setter(obj, Bridge<OSGParamType>::get(valProp->getValue<sg_type>()));
    } else {
        setDynamicVariance(obj);
        std::vector<std::string> paramNames
            = getVectorProperties(valProp, options,numComponents, nameItr);
        if (paramNames.empty())
            throw BuilderException();
        std::vector<std::string>::const_iterator pitr = paramNames.begin();
        DeferredPropertyListener* listener
            =  new_EEPropListener<sg_type>(make_OSGFunctor<OSGParamType>
                                           (obj, setter),
                                           0, pitr, pitr + numComponents);
        effect->addDeferredPropertyListener(listener);
    }
    return;
}

template<typename OSGParamType, typename ObjType, typename NameItrType,
         typename SetterReturn>
inline void
initFromParameters(Effect* effect, const SGPropertyNode* prop, ObjType* obj,
                   SetterReturn (ObjType::*setter)(const OSGParamType&),
                   NameItrType nameItr, const SGReaderWriterOptions* options)
{
    initFromParameters<OSGParamType>(effect, prop, obj,
                                     boost::bind(setter, _1, _2), nameItr,
                                     options);
}
extern const char* colorFields[];
}
}
#endif
