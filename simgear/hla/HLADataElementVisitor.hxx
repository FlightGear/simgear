// Copyright (C) 2009 - 2012  Mathias Froehlich - Mathias.Froehlich@web.de
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

#ifndef HLADataElementVisitor_hxx
#define HLADataElementVisitor_hxx

namespace simgear {

class HLABasicDataElement;
class HLAAbstractEnumeratedDataElement;
class HLAAbstractFixedRecordDataElement;
class HLAAbstractArrayDataElement;
class HLAAbstractVariantRecordDataElement;

class HLADataElementVisitor {
public:
    virtual ~HLADataElementVisitor() {}

    virtual void apply(HLADataElement&);

    virtual void apply(HLABasicDataElement&);
    virtual void apply(HLAAbstractEnumeratedDataElement&);
    virtual void apply(HLAAbstractArrayDataElement&);
    virtual void apply(HLAAbstractFixedRecordDataElement&);
    virtual void apply(HLAAbstractVariantRecordDataElement&);
};

class HLAConstDataElementVisitor {
public:
    virtual ~HLAConstDataElementVisitor() {}

    virtual void apply(const HLADataElement&);

    virtual void apply(const HLABasicDataElement&);
    virtual void apply(const HLAAbstractEnumeratedDataElement&);
    virtual void apply(const HLAAbstractArrayDataElement&);
    virtual void apply(const HLAAbstractFixedRecordDataElement&);
    virtual void apply(const HLAAbstractVariantRecordDataElement&);
};

}

#endif
