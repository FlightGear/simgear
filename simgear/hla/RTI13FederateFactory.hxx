// Copyright (C) 2011 - 2012  Mathias Froehlich - Mathias.Froehlich@web.de
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

#ifndef RTI13FederateFactory_hxx
#define RTI13FederateFactory_hxx

#include "RTIFederateFactory.hxx"

#include "simgear/structure/SGSharedPtr.hxx"

namespace simgear {

class RTI13FederateFactory : public RTIFederateFactory {
public:
    RTI13FederateFactory();
    virtual ~RTI13FederateFactory();

    virtual RTIFederate* create(const std::string& name, const std::list<std::string>& stringList) const;

    static const SGSharedPtr<RTI13FederateFactory>& instance();
};

}

#endif
