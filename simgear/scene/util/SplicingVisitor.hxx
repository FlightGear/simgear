#ifndef SIMGEAR_SPLICINGVISITOR_HXX
#define SIMGEAR_SPLICINGVISITOR_HXX 1

#include <cstddef>
#include <map>
#include <vector>
#include <osg/NodeVisitor>
#include <osg/Group>

namespace simgear
{
class SplicingVisitor : public osg::NodeVisitor
{
public:
    META_NodeVisitor(simgear,SplicingVisitor);
    SplicingVisitor();
    virtual ~SplicingVisitor() {}
    virtual void reset();
    osg::NodeList traverse(osg::Node& node);
    using osg::NodeVisitor::apply;
    virtual void apply(osg::Node& node);
    virtual void apply(osg::Group& node);
    template<typename T>
    static T* copyIfNeeded(T& node, const osg::NodeList& children);
    template<typename T>
    static T* copy(T& node, const osg::NodeList& children);
    /**
     * Push the result of processing this node.
     *
     * If the child list is not equal to the node's current children,
     * make a copy of the node. Record the (possibly new) node which
     * should be the returned result if the node is visited again.
     */
    osg::Group* pushResultNode(osg::Group* node, osg::Group* newNode,
                               const osg::NodeList& children);
    /**
     * Push the result of processing this node.
     *
     * Record the (possibly new) node which should be the returned
     * result if the node is visited again.
     */
    osg::Node* pushResultNode(osg::Node* node, osg::Node* newNode);
    /**
     * Push some node onto the list of result nodes.
     */
    osg::Node* pushNode(osg::Node* node);
    osg::Node* getResult();
    osg::Node* getNewNode(osg::Node& node)
    {
        return getNewNode(&node);
    }
    osg::Node* getNewNode(osg::Node* node);
    bool recordNewNode(osg::Node* oldNode, osg::Node* newNode);
    osg::NodeList& getResults() { return _childStack.back(); }
protected:
    std::vector<osg::NodeList> _childStack;
    typedef std::map<osg::ref_ptr<osg::Node>, osg::ref_ptr<osg::Node> > NodeMap;
    NodeMap _visited;
};

template<typename T>
T* SplicingVisitor::copyIfNeeded(T& node, const osg::NodeList& children)
{
    bool copyNeeded = false;
    if (node.getNumChildren() == children.size()) {
        for (std::size_t i = 0; i < children.size(); ++i)
            if (node.getChild(i) != children[i].get()) {
                copyNeeded = true;
                break;
            }
    } else {
        copyNeeded = true;
    }
    if (copyNeeded)
        return copy(node, children);
    else
        return &node;
}

template<typename T>
T* SplicingVisitor::copy(T& node, const osg::NodeList& children)
{
    T* result = osg::clone(&node, osg::CopyOp::SHALLOW_COPY);
    result->removeChildren(0, result->getNumChildren());
    for (osg::NodeList::const_iterator itr = children.begin(),
             end = children.end();
         itr != end;
         ++itr)
        result->addChild(itr->get());
    return result;
}
}
#endif
