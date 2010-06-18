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

#ifndef RTIInteractionClass_hxx
#define RTIInteractionClass_hxx

#include <set>
#include <string>
#include <vector>
#include "simgear/structure/SGReferenced.hxx"
#include "simgear/structure/SGSharedPtr.hxx"
#include "simgear/structure/SGReferenced.hxx"
#include "RTIData.hxx"

namespace simgear {

class RTIInteractionClass : public SGReferenced {
public:
    RTIInteractionClass(const std::string& name);
    virtual ~RTIInteractionClass();

    const std::string& getName() const
    { return _name; }

    virtual unsigned getNumParameters() const = 0;
    virtual unsigned getParameterIndex(const std::string& name) const = 0;
    virtual unsigned getOrCreateParameterIndex(const std::string& name) = 0;

    virtual bool publish(const std::set<unsigned>& indexSet) = 0;
    virtual bool unpublish() = 0;

    virtual bool subscribe(const std::set<unsigned>& indexSet, bool) = 0;
    virtual bool unsubscribe() = 0;

    virtual void send(const RTIData& tag) = 0;
    virtual void send(const SGTimeStamp& timeStamp, const RTIData& tag) = 0;

    class ReceiveCallback : public SGReferenced {
    public:
        virtual ~ReceiveCallback() {}
    };

private:
    std::string _name;
};

}

#endif
