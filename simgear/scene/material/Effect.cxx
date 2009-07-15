#include "Effect.hxx"
#include "Technique.hxx"

#include <algorithm>
#include <functional>
#include <iterator>

#include <boost/bind.hpp>
#include <boost/foreach.hpp>

#include <osg/Drawable>
#include <osg/RenderInfo>
#include <osg/StateSet>

#include <osgUtil/CullVisitor>

#include <simgear/structure/OSGUtils.hxx>



namespace simgear
{
using namespace osg;
using namespace osgUtil;

Effect::Effect(const Effect& rhs, const CopyOp& copyop)
{
    using namespace std;
    using namespace boost;
    transform(rhs.techniques.begin(), rhs.techniques.end(),
              backRefInsertIterator(techniques),
              bind(simgear::clone_ref<Technique>, _1, copyop));
}
// There should always be a valid technique in an effect.

Technique* Effect::chooseTechnique(RenderInfo* info)
{
    BOOST_FOREACH(ref_ptr<Technique>& technique, techniques)
    {
        if (technique->valid(info) == Technique::VALID)
            return technique.get();
    }
    return 0;
}

void Effect::resizeGLObjectBuffers(unsigned int maxSize)
{
    BOOST_FOREACH(const ref_ptr<Technique>& technique, techniques)
    {
        technique->resizeGLObjectBuffers(maxSize);
    }
}

void Effect::releaseGLObjects(osg::State* state) const
{
    BOOST_FOREACH(const ref_ptr<Technique>& technique, techniques)
    {
        technique->releaseGLObjects(state);
    }
}
}
