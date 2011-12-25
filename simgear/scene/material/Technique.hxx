// Copyright (C) 2008  Timothy Moore timoore@redhat.com
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#ifndef SIMGEAR_TECHNIQUE_HXX
#define SIMGEAR_TECHNIQUE_HXX 1

#include "EffectGeode.hxx"

#include <simgear/structure/SGAtomic.hxx>
#include <simgear/structure/SGExpression.hxx>

#include <map>
#include <vector>
#include <string>

#include <OpenThreads/Mutex>
#include <osg/buffered_value>
#include <osg/Geode>
#include <osg/Object>
#include <osg/GraphicsThread>

namespace osg
{
class CopyOp;
class Drawable;
class RenderInfo;
class StateSet;
}

namespace osgUtil
{
class CullVisitor;
}

namespace simgear
{
class Pass;

class Technique : public osg::Object
{
public:
    META_Object(simgear,Technique);
    Technique(bool alwaysValid = false);
    Technique(const Technique& rhs,
              const osg::CopyOp& copyop = osg::CopyOp::SHALLOW_COPY);
    virtual ~Technique();
    enum Status
    {
        UNKNOWN,
        QUERY_IN_PROGRESS,
        INVALID,
        VALID
    };
    /** Returns the validity of a technique in a state. If we don't
     * know, a query will be launched.
     */
    virtual Status valid(osg::RenderInfo* renderInfo);
    /** Returns the validity of the technique without launching a
     * query.
     */
    Status getValidStatus(const osg::RenderInfo* renderInfo) const;
    /** Tests and sets the validity of the Technique. Must be run in a
     *graphics context.
     */
    virtual void validateInContext(osg::GraphicsContext* gc);

    virtual EffectGeode::DrawablesIterator
    processDrawables(const EffectGeode::DrawablesIterator& begin,
                     const EffectGeode::DrawablesIterator& end,
                     osgUtil::CullVisitor* cv,
                     bool isCullingActive);
    std::vector<osg::ref_ptr<Pass> > passes;
    osg::StateSet* getShadowingStateSet() { return _shadowingStateSet.get(); }
    const osg::StateSet* getShadowingStateSet() const
    {
        return _shadowingStateSet.get();
    }
    void setShadowingStateSet(osg::StateSet* ss) { _shadowingStateSet = ss; }
    virtual void resizeGLObjectBuffers(unsigned int maxSize);
    virtual void releaseGLObjects(osg::State* state = 0) const;
    bool getAlwaysValid() const { return _alwaysValid; }
    void setAlwaysValid(bool val) { _alwaysValid = val; }
    void setValidExpression(SGExpressionb* exp,
                            const simgear::expression::BindingLayout&);
    void setGLExtensionsPred(float glVersion,
                             const std::vector<std::string>& extensions);
    void refreshValidity();
protected:
    // Validity of technique in a graphics context.
    struct ContextInfo : public osg::Referenced
    {
        ContextInfo() : valid(UNKNOWN) {}
        ContextInfo(const ContextInfo& rhs) : osg::Referenced(rhs), valid(rhs.valid()) {}
        ContextInfo& operator=(const ContextInfo& rhs)
        {
            valid = rhs.valid;
            return *this;
        }
        Swappable<Status> valid;
    };
    typedef osg::buffered_object<ContextInfo> ContextMap;
    mutable ContextMap _contextMap;
    bool _alwaysValid;
    osg::ref_ptr<osg::StateSet> _shadowingStateSet;
    SGSharedPtr<SGExpressionb> _validExpression;
    int _contextIdLocation;
};

class TechniquePredParser : public expression::ExpressionParser
{
public:
    void setTechnique(Technique* tniq) { _tniq = tniq; }
    Technique* getTechnique() { return _tniq.get(); }
//    void setEffect(Effect* effect) { _effect = effect; }
//    Effect* getEffect() { return _effect.get(); }
protected:
    osg::ref_ptr<Technique> _tniq;
    // osg::ref_ptr<Effect> _effect;
};
}
#endif
