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

#ifndef SIMGEAR_EFFECT_DATA_HXX
#define SIMGEAR_EFFECT_DATA_HXX 1

#include <osg/Vec4f>

#include "EffectElement.hxx"
#include "EffectElementBuilder.hxx"

namespace simgear
{
class ParamaterContext;

template<typename T>
class EffectData : public EffectElement
{
public:
    EffectData() {}
    EffectData(const EffectData& rhs) _value(rhs._value) {}
    virtual ~EffectData() {}
    T getValue(const ParameterContext*) const {return _value};
    void setValue(const T& value) { _value = value; }
private:
    T _value;
};

typedef EffectData<float> EffectFloat;
typedef EffectData<osg::Vec4f> EffectVec4f;


}
#endif
