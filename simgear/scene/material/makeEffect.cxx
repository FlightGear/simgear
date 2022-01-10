
#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include "Effect.hxx"
#include "EffectBuilder.hxx"
#include "Technique.hxx"
#include "Pass.hxx"

#include <algorithm>
#include <cstring>
#include <map>

#include <OpenThreads/ReentrantMutex>
#include <OpenThreads/ScopedLock>

#include <osg/Material>
#include <osg/Program>
#include <osg/Referenced>
#include <osg/Texture2D>
#include <osg/Vec4d>

#include <osgDB/FileUtils>
#include <osgDB/ReadFile>
#include <osgDB/Registry>

#include <simgear/debug/ErrorReportingCallback.hxx>
#include <simgear/debug/logstream.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/scene/util/SGReaderWriterOptions.hxx>
#include <simgear/scene/util/SGSceneFeatures.hxx>
#include <simgear/scene/util/SplicingVisitor.hxx>
#include <simgear/structure/SGExpression.hxx>

namespace simgear
{
using namespace std;
using namespace osg;
using namespace effect;

typedef vector<const SGPropertyNode*> RawPropVector;
typedef map<const string, ref_ptr<Effect> > EffectMap;

namespace
{
EffectMap effectMap;
OpenThreads::ReentrantMutex effectMutex;
}

/** Merge two property trees, producing a new tree.
 * If the nodes are both leaves, value comes from left leaf.
 * Otherwise, The children are examined. If a left and right child are
 * "identical," they are merged and the result placed in the children
 * of the result. Otherwise the left children are placed after the
 * right children in the result.
 *
 * Nodes are considered equal if their names and indexes are equal.
 */

struct PropPredicate
    : public unary_function<const SGPropertyNode*, bool>
{
    PropPredicate(const SGPropertyNode* node_) : node(node_) {}
    bool operator()(const SGPropertyNode* arg) const
    {
        if (strcmp(node->getName(), arg->getName()))
            return false;
        return node->getIndex() == arg->getIndex();
    }
    const SGPropertyNode* node;
};

namespace effect
{
void mergePropertyTrees(SGPropertyNode* resultNode,
                        const SGPropertyNode* left, const SGPropertyNode* right)
{
    if (left->nChildren() == 0) {
        copyProperties(left, resultNode);
        return;
    }
    resultNode->setAttributes(right->getAttributes());
    RawPropVector leftChildren;
    for (int i = 0; i < left->nChildren(); ++i)
        leftChildren.push_back(left->getChild(i));
    // Merge identical nodes
    for (int i = 0; i < right->nChildren(); ++i) {
        const SGPropertyNode* node = right->getChild(i);
        RawPropVector::iterator litr
            = find_if(leftChildren.begin(), leftChildren.end(),
                      PropPredicate(node));
        SGPropertyNode* newChild
            = resultNode->getChild(node->getName(), node->getIndex(), true);
        if (litr != leftChildren.end()) {
            mergePropertyTrees(newChild, *litr, node);
            leftChildren.erase(litr);
        } else {
            copyProperties(node, newChild);
        }
    }
    // Now copy nodes remaining in the left tree
    for (RawPropVector::iterator itr = leftChildren.begin(),
             e = leftChildren.end();
         itr != e;
         ++itr) {
        SGPropertyNode* newChild
            = resultNode->getChild((*itr)->getName(), (*itr)->getIndex(), true);
        copyProperties(*itr, newChild);
    }
}
}

Effect* makeEffect(const string& name,
                   bool realizeTechniques,
                   const SGReaderWriterOptions* options,
                   const SGPath& modelPath)
{
    {
        OpenThreads::ScopedLock<OpenThreads::ReentrantMutex> lock(effectMutex);
        EffectMap::iterator itr = effectMap.find(name);
        if ((itr != effectMap.end())&&
            itr->second.valid())
            return itr->second.get();
    }
    string effectFileName(name);
    effectFileName += ".eff";
    string absFileName = SGModelLib::findDataFile(effectFileName, options, modelPath);
    if (absFileName.empty()) {
        simgear::reportFailure(simgear::LoadFailure::NotFound, simgear::ErrorCode::LoadEffectsShaders, "Couldn't find Effect:" + effectFileName);
        return nullptr;
    }
    SGPropertyNode_ptr effectProps = new SGPropertyNode();
    try {
        readProperties(absFileName, effectProps.ptr(), 0, true);
    }
    catch (sg_io_exception& e) {
        simgear::reportFailure(simgear::LoadFailure::BadData, simgear::ErrorCode::LoadEffectsShaders, e.getFormattedMessage(),
                               e.getLocation());
        return nullptr;
    }
    ref_ptr<Effect> result = makeEffect(effectProps.ptr(), realizeTechniques,
                                        options, SGPath::fromUtf8(absFileName));
    if (result.valid()) {
        OpenThreads::ScopedLock<OpenThreads::ReentrantMutex> lock(effectMutex);
        pair<EffectMap::iterator, bool> irslt
            = effectMap.insert(make_pair(name, result));
        if (!irslt.second) {
            // Another thread beat us to it!. Discard our newly
            // constructed Effect and use the one in the cache.
            result = irslt.first->second;
        }
    }
    return result.release();
}


Effect* makeEffect(SGPropertyNode* prop,
                   bool realizeTechniques,
                   const SGReaderWriterOptions* options,
                   const SGPath& filePath)
{
    // Give default names to techniques and passes
    vector<SGPropertyNode_ptr> techniques = prop->getChildren("technique");
    for (int i = 0; i < (int)techniques.size(); ++i) {
        SGPropertyNode* tniqProp = techniques[i].ptr();
        if (!tniqProp->hasChild("name")) {
            setValue(tniqProp->getChild("name", 0, true),
                     std::to_string(i));
        }
        vector<SGPropertyNode_ptr> passes = tniqProp->getChildren("pass");
        for (int j = 0; j < (int)passes.size(); ++j) {
            SGPropertyNode* passProp = passes[j].ptr();
            if (!passProp->hasChild("name")) {
                setValue(passProp->getChild("name", 0, true),
                         std::to_string(j));
            }
            vector<SGPropertyNode_ptr> texUnits
                = passProp->getChildren("texture-unit");
            for (int k = 0; k < (int)texUnits.size(); ++k) {
                SGPropertyNode* texUnitProp = texUnits[k].ptr();
                if (!texUnitProp->hasChild("name")) {
                    setValue(texUnitProp->getChild("name", 0, true),
                             std::to_string(k));
                }
            }
        }
    }
    ref_ptr<Effect> effect;
    if(!prop->hasChild("name")){
        setValue(prop->getChild("name", 0, true), "noname");
    }
    SGPropertyNode_ptr nameProp = prop->getChild("name");
    // Merge with the parent effect, if any
    SGPropertyNode_ptr inheritProp = prop->getChild("inherits-from");
    Effect* parent = 0;
    if (inheritProp) {
        //prop->removeChild("inherits-from");
        parent = makeEffect(inheritProp->getStringValue(), false, options, filePath);
        if (parent) {
            Effect::Key key;
            key.unmerged = prop;
            if (options) {
                key.paths = options->getDatabasePathList();
            }
            Effect::Cache* cache = 0;
            Effect::Cache::iterator itr;
            {
                OpenThreads::ScopedLock<OpenThreads::ReentrantMutex>
                    lock(effectMutex);
                cache = parent->getCache();
                itr = cache->find(key);
                if ((itr != cache->end())&&
                    itr->second.lock(effect))
                {
                    effect->generator = parent->generator;  // Copy the generators
                }
            }
            if (!effect.valid()) {
                effect = new Effect;
                effect->setName(nameProp->getStringValue());
                effect->setFilePath(filePath.isNull() ? parent->filePath() : filePath);
                effect->root = new SGPropertyNode;
                mergePropertyTrees(effect->root, prop, parent->root);
                effect->parametersProp = effect->root->getChild("parameters");
                OpenThreads::ScopedLock<OpenThreads::ReentrantMutex>
                    lock(effectMutex);
                pair<Effect::Cache::iterator, bool> irslt
                    = cache->insert(make_pair(key, effect));
                if (!irslt.second) {
                    ref_ptr<Effect> old;
                    if (irslt.first->second.lock(old))
                        effect = old; // Another thread beat us in creating it! Discard our own...
                    else
                        irslt.first->second = effect; // update existing, but empty observer
                }
                effect->generator = parent->generator;  // Copy the generators
            }
        } else {
            simgear::reportFailure(simgear::LoadFailure::NotFound, simgear::ErrorCode::LoadEffectsShaders,
                                   string{"couldn't find base effect to inherit from:"} + inheritProp->getStringValue(), filePath);
            return nullptr;
        }
    } else {
        effect = new Effect;
        effect->setFilePath(filePath);
        effect->setName(nameProp->getStringValue());
        effect->root = prop;
        effect->parametersProp = effect->root->getChild("parameters");
    }
    const SGPropertyNode *generateProp = prop->getChild("generate");
    if(generateProp)
    {
        effect->generator.clear();

        // Effect needs some generated properties, like tangent vectors
        const SGPropertyNode *parameter = generateProp->getChild("normal");
        if(parameter) effect->setGenerator(Effect::NORMAL, parameter->getIntValue());

        parameter = generateProp->getChild("tangent");
        if(parameter) effect->setGenerator(Effect::TANGENT, parameter->getIntValue());

        parameter = generateProp->getChild("binormal");
        if(parameter) effect->setGenerator(Effect::BINORMAL, parameter->getIntValue());
    }
    if (realizeTechniques) {
        try {
            OpenThreads::ScopedLock<OpenThreads::ReentrantMutex>
                lock(effectMutex);
            effect->realizeTechniques(options);
        }
        catch (BuilderException& e) {
            simgear::reportFailure(simgear::LoadFailure::Misconfigured, simgear::ErrorCode::LoadEffectsShaders,
                                   "Failed to build technique:" + e.getFormattedMessage(), filePath);
            return nullptr;
        }
    }
    return effect.release();
}

void clearEffectCache()
{
    SG_LOG(SG_INPUT, SG_DEBUG, "clearEffectCache called");
    OpenThreads::ScopedLock<OpenThreads::ReentrantMutex> lock(effectMutex);
    effectMap.clear();
    UniformFactory::instance()->reset();
}

}
