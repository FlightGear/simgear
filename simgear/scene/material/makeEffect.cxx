
#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include "Effect.hxx"
#include "Technique.hxx"
#include "Pass.hxx"

#include <algorithm>
#include <cstring>
#include <map>
#include <sstream>

#include <boost/lexical_cast.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/tuple/tuple_comparison.hpp>

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

#include <simgear/debug/logstream.hxx>
#include <simgear/props/props_io.hxx>
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
                   const osgDB::ReaderWriter::Options* options)
{
    OpenThreads::ScopedLock<OpenThreads::ReentrantMutex> lock(effectMutex);
    EffectMap::iterator itr = effectMap.find(name);
    if (itr != effectMap.end())
        return itr->second.get();
    string effectFileName(name);
    effectFileName += ".eff";
    string absFileName
        = osgDB::findDataFile(effectFileName, options);
    if (absFileName.empty()) {
        SG_LOG(SG_INPUT, SG_WARN, "can't find \"" << effectFileName << "\"");
        return 0;
    }
    SGPropertyNode_ptr effectProps = new SGPropertyNode();
    readProperties(absFileName, effectProps.ptr(), 0, true);
    Effect* result = makeEffect(effectProps.ptr(), realizeTechniques, options);
    if (result)
        effectMap.insert(make_pair(name, result));
    return result;
}


Effect* makeEffect(SGPropertyNode* prop,
                   bool realizeTechniques,
                   const osgDB::ReaderWriter::Options* options)
{
    // Give default names to techniques and passes
    vector<SGPropertyNode_ptr> techniques = prop->getChildren("technique");
    for (int i = 0; i < (int)techniques.size(); ++i) {
        SGPropertyNode* tniqProp = techniques[i].ptr();
        if (!tniqProp->hasChild("name"))
            setValue(tniqProp->getChild("name", 0, true),
                     boost::lexical_cast<string>(i));
        vector<SGPropertyNode_ptr> passes = tniqProp->getChildren("pass");
        for (int j = 0; j < (int)passes.size(); ++j) {
            SGPropertyNode* passProp = passes[j].ptr();
            if (!passProp->hasChild("name"))
                setValue(passProp->getChild("name", 0, true),
                         boost::lexical_cast<string>(j));
            vector<SGPropertyNode_ptr> texUnits
                = passProp->getChildren("texture-unit");
            for (int k = 0; k < (int)texUnits.size(); ++k) {
                SGPropertyNode* texUnitProp = texUnits[k].ptr();
                if (!texUnitProp->hasChild("name"))
                    setValue(texUnitProp->getChild("name", 0, true),
                             boost::lexical_cast<string>(k));
            }
        }
    }
    Effect* effect = 0;
    // Merge with the parent effect, if any
    SGPropertyNode_ptr inheritProp = prop->getChild("inherits-from");
    Effect* parent = 0;
    if (inheritProp) {
        //prop->removeChild("inherits-from");
        parent = makeEffect(inheritProp->getStringValue(), false, options);
        if (parent) {
            Effect::Cache* cache = parent->getCache();
            Effect::Key key;
            if (options) {
                key = Effect::Key(prop, options->getDatabasePathList());
            } else {
                osgDB::FilePathList dummy;
                key = Effect::Key(prop, dummy);
            }
            Effect::Cache::iterator itr = cache->find(key);
            if (itr != cache->end()) {
                effect = itr->second.get();
            } else {
                effect = new Effect;
                effect->root = new SGPropertyNode;
                mergePropertyTrees(effect->root, prop, parent->root);
                cache->insert(make_pair(key, effect));
            }
        } else {
            SG_LOG(SG_INPUT, SG_WARN, "can't find base effect " <<
                   inheritProp->getStringValue());
        }
    }
    if (!effect) {
        effect = new Effect;
        effect->root = prop;
    }
    effect->parametersProp = effect->root->getChild("parameters");
    if (realizeTechniques)
        effect->realizeTechniques(options);
    return effect;
}

}
