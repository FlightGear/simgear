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

#ifndef RTIFederateFactoryRegistry_hxx
#define RTIFederateFactoryRegistry_hxx

#include <list>
#include <string>
#include "simgear/structure/SGReferenced.hxx"
#include "simgear/structure/SGSharedPtr.hxx"
#include "RTIFederateFactory.hxx"

#include "simgear/threads/SGThread.hxx"

namespace simgear {

class RTIFederateFactoryRegistry : public SGReferenced {
public:
    ~RTIFederateFactoryRegistry();

    SGSharedPtr<RTIFederate> create(const std::string& name, const std::list<std::string>& stringList) const;

    void registerFactory(RTIFederateFactory* factory);

    static const SGSharedPtr<RTIFederateFactoryRegistry>& instance();

private:
    RTIFederateFactoryRegistry();

    RTIFederateFactoryRegistry(const RTIFederateFactoryRegistry&);
    RTIFederateFactoryRegistry& operator=(const RTIFederateFactoryRegistry&);

    mutable SGMutex _mutex;
    typedef std::list<SGSharedPtr<RTIFederateFactory> > FederateFactoryList;
    FederateFactoryList _federateFactoryList;
};

}

#endif
