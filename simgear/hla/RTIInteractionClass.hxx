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

#ifndef RTIInteractionClass_hxx
#define RTIInteractionClass_hxx

#include <string>
#include "simgear/structure/SGReferenced.hxx"

namespace simgear {

class HLAInteractionClass;

class RTIInteractionClass : public SGReferenced {
public:
    RTIInteractionClass(HLAInteractionClass* interactionClass);
    virtual ~RTIInteractionClass();

    virtual bool resolveParameterIndex(const std::string& name, unsigned index) = 0;

    virtual bool publish() = 0;
    virtual bool unpublish() = 0;

    virtual bool subscribe(bool) = 0;
    virtual bool unsubscribe() = 0;

    // virtual void send(const RTIData& tag) = 0;
    // virtual void send(const SGTimeStamp& timeStamp, const RTIData& tag) = 0;

private:
    HLAInteractionClass* _interactionClass;

    friend class HLAInteractionClass;
};

}

#endif
