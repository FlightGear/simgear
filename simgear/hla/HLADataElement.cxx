// Copyright (C) 2009 - 2010  Mathias Froehlich - Mathias.Froehlich@web.de
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
//

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include <simgear/compiler.h>

#include "HLADataElement.hxx"

#include <simgear/debug/logstream.hxx>

#include "HLADataElementVisitor.hxx"

namespace simgear {

HLADataElement::~HLADataElement()
{
}

bool
HLADataElement::setDataElement(HLADataElementIndex::const_iterator begin, HLADataElementIndex::const_iterator end, HLADataElement* dataElement)
{
    return false;
}

HLADataElement*
HLADataElement::getDataElement(HLADataElementIndex::const_iterator begin, HLADataElementIndex::const_iterator end)
{
    if (begin != end)
        return 0;
    return this;
}

const HLADataElement*
HLADataElement::getDataElement(HLADataElementIndex::const_iterator begin, HLADataElementIndex::const_iterator end) const
{
    if (begin != end)
        return 0;
    return this;
}

void
HLADataElement::setTimeStamp(const SGTimeStamp& timeStamp)
{
    if (!_stamp.valid())
        return;
    _stamp->setTimeStamp(timeStamp);
}

void
HLADataElement::setTimeStampValid(bool timeStampValid)
{
    if (!_stamp.valid())
        return;
    _stamp->setTimeStampValid(timeStampValid);
}

double
HLADataElement::getTimeDifference(const SGTimeStamp& timeStamp) const
{
    if (!_stamp.valid())
        return 0;
    if (!_stamp->getTimeStampValid())
        return 0;
    return (timeStamp - _stamp->getTimeStamp()).toSecs();
}

void
HLADataElement::createStamp()
{
    _setStamp(new Stamp);
    setDirty(true);
}

void
HLADataElement::attachStamp(HLADataElement& dataElement)
{
    _setStamp(dataElement._getStamp());
}

void
HLADataElement::clearStamp()
{
    _setStamp(0);
}

void
HLADataElement::accept(HLADataElementVisitor& visitor)
{
    visitor.apply(*this);
}

void
HLADataElement::accept(HLAConstDataElementVisitor& visitor) const
{
    visitor.apply(*this);
}

void
HLADataElement::_setStamp(HLADataElement::Stamp* stamp)
{
    _stamp = stamp;
}

}
