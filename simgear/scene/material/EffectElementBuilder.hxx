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

#ifndef SIMGEAR_EFFECT_BUILDER_HXX
#define SIMGEAR_EFFECT_BUILDER_HXX 1

#include <simgear/xml/easyxml.hxx>
#include "EffectElement.hxx"

namespace simgear
{
class ElementBuilder;

class EffectElementBuilder
{
public:
    EffectElementBuilder(ElementBuilder* builder);
    EffectElementBuilder(const EffectElementBuilder& rhs);
    virtual ~EffectElementBuilder();
    virtual void initialize(const XMLAttributes& attributes);
    virtual void processSubElement(EffectElement* subElement);
    virtual void processData(const char* data, int length);
    virtual EffectElement* finalize();

};
}

#endif
