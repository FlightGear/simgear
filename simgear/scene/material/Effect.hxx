// Copyright (C) 2008 - 2009  Tim Moore timoore@redhat.com
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

#ifndef SIMGEAR_EFFECT_HXX
#define SIMGEAR_EFFECT_HXX 1

#include <vector>
#include <string>
#include <boost/tr1/unordered_map.hpp>

#include <boost/functional/hash.hpp>

#include <osg/Object>
#include <osg/observer_ptr>
#include <osgDB/ReaderWriter>

#include <simgear/props/props.hxx>
#include <simgear/scene/util/UpdateOnceCallback.hxx>

namespace osg
{
class Drawable;
class StateSet;
class RenderInfo;
}

namespace osgUtil
{
class CullVisitor;
}

namespace simgear
{
class Technique;
class Effect;
class SGReaderWriterOptions;

/**
 * Object to be initialized at some point after an effect -- and its
 * containing effect geode -- are hooked into the scene graph. Some
 * things, like manipulations of the global property tree, are are
 * only safe in the update process.
 */
class DeferredPropertyListener {
public:
    virtual void activate(SGPropertyNode* propRoot) {};
    virtual ~DeferredPropertyListener() {};
};

class Effect : public osg::Object
{
public:
    META_Object(simgear,Effect)
    Effect();
    Effect(const Effect& rhs,
           const osg::CopyOp& copyop = osg::CopyOp::SHALLOW_COPY);
    osg::StateSet* getDefaultStateSet();

    // Define what needs to be generated for this effect
    enum Generator
    {
        NORMAL,
        TANGENT,
        BINORMAL
    };
    void setGenerator(Generator what, int where) { generator[what] = where; }
    int getGenerator(Generator what) const;  // Returns -1 if generator should not be used
    std::map<Generator, int> generator;  // What is generated into which attribute location

    std::vector<osg::ref_ptr<Technique> > techniques;
    SGPropertyNode_ptr root;
    // Pointer to the parameters node, if it exists
    SGPropertyNode_ptr parametersProp;
    Technique* chooseTechnique(osg::RenderInfo* renderInfo);
    virtual void resizeGLObjectBuffers(unsigned int maxSize);
    virtual void releaseGLObjects(osg::State* state = 0) const;
    /**
     * Build the techniques from the effect properties.
     */
    bool realizeTechniques(const SGReaderWriterOptions* options = 0);
    void addDeferredPropertyListener(DeferredPropertyListener* listener);
    // Callback that is added to the effect geode to initialize the
    // effect.
    friend struct InitializeCallback;
    struct InitializeCallback : public UpdateOnceCallback
    {
        void doUpdate(osg::Node* node, osg::NodeVisitor* nv);
    };

    std::string getName(){return _name;}
    void setName(std::string name){_name = name;}
protected:
    ~Effect();
    // Support for a cache of effects that inherit from this one, so
    // Effect objects with the same parameters and techniques can be
    // shared.
    struct Key
    {
        Key() {}
        Key(SGPropertyNode* unmerged_, const osgDB::FilePathList& paths_)
            : unmerged(unmerged_), paths(paths_)
        {
        }
        Key& operator=(const Key& rhs)
        {
            unmerged = rhs.unmerged;
            paths = rhs.paths;
            return *this;
        }
        SGPropertyNode_ptr unmerged;
        osgDB::FilePathList paths;
        struct EqualTo
        {
            bool operator()(const Key& lhs, const Key& rhs) const;
        };
    };
    typedef std::tr1::unordered_map<Key, osg::observer_ptr<Effect>,
                                    boost::hash<Key>, Key::EqualTo> Cache;
    Cache* getCache()
    {
        if (!_cache)
            _cache = new Cache;
        return _cache;
    }
    Cache* _cache;
    friend size_t hash_value(const Key& key);
    friend Effect* makeEffect(SGPropertyNode* prop, bool realizeTechniques,
                              const SGReaderWriterOptions* options);
    bool _isRealized;
    std::string _name;
};
// Automatic support for boost hash function
size_t hash_value(const Effect::Key&);


Effect* makeEffect(const std::string& name,
                   bool realizeTechniques,
                   const SGReaderWriterOptions* options);

Effect* makeEffect(SGPropertyNode* prop,
                   bool realizeTechniques,
                   const SGReaderWriterOptions* options);

bool makeParametersFromStateSet(SGPropertyNode* paramRoot,
                                const osg::StateSet* ss);

void clearEffectCache();

namespace effect
{
/**
 * The function that implements effect property tree inheritance.
 */
void mergePropertyTrees(SGPropertyNode* resultNode,
                        const SGPropertyNode* left,
                        const SGPropertyNode* right);
}
}
#endif
