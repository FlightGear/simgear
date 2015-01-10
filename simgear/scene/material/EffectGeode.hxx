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

#ifndef SIMGEAR_EFFECT_GEODE_HXX
#define SIMGEAR_EFFECT_GEODE_HXX 1

#include <osg/Geode>
#include <osg/Version>

#include <boost/iterator/iterator_adaptor.hpp>

#include "Effect.hxx"
#include "mat.hxx"

namespace simgear
{
class EffectGeode : public osg::Geode
{
  public:

#if OSG_VERSION_LESS_THAN(3,3,2)
    typedef DrawableList::iterator DrawablesIterator;
#else
    class DrawablesIterator:
      public boost::iterator_adaptor<
        DrawablesIterator,
        osg::NodeList::iterator,
        osg::ref_ptr<osg::Drawable>,
        boost::use_default,
        osg::ref_ptr<osg::Drawable> // No reference as Reference type.
                                    // The child list does not contain Drawable
                                    // ref_ptr so we can not return any
                                    // references to them.
      >
    {
      public:

        DrawablesIterator()
        {}

        explicit DrawablesIterator(osg::NodeList::iterator const& node_it):
          DrawablesIterator::iterator_adaptor_(node_it)
        {}

      private:
        friend class boost::iterator_core_access;
        osg::ref_ptr<osg::Drawable> dereference() const
        {
          return base_reference()->get()->asDrawable();
        }
    };
#endif

    EffectGeode();
    EffectGeode(const EffectGeode& rhs,
                const osg::CopyOp& copyop = osg::CopyOp::SHALLOW_COPY);
    META_Node(simgear,EffectGeode);
    Effect* getEffect() const { return _effect.get(); }
    void setEffect(Effect* effect);
    SGMaterial* getMaterial() const { return _material; }
    void setMaterial(SGMaterial* mat) { _material = mat; }
    virtual void resizeGLObjectBuffers(unsigned int maxSize);
    virtual void releaseGLObjects(osg::State* = 0) const;

#if OSG_VERSION_LESS_THAN(3,3,2)
    DrawablesIterator drawablesBegin() { return _drawables.begin(); }
    DrawablesIterator drawablesEnd() { return _drawables.end(); }
#else
    DrawablesIterator drawablesBegin() { return DrawablesIterator(_children.begin()); }
    DrawablesIterator drawablesEnd() { return DrawablesIterator(_children.end()); }
#endif

    void runGenerators(osg::Geometry *geometry);
private:
    osg::ref_ptr<Effect> _effect;
    SGMaterial* _material;
};
}
#endif
