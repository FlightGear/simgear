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

#ifndef RTIFederate_hxx
#define RTIFederate_hxx

#include <string>
#include "simgear/structure/SGWeakReferenced.hxx"
#include "RTIObjectClass.hxx"
#include "RTIObjectInstance.hxx"

class SGTimeStamp;

namespace simgear {

class RTIFederate : public SGWeakReferenced {
public:
    RTIFederate();
    virtual ~RTIFederate();

    /// Get the name of the joined federate/federation
    const std::string& getFederateType() const
    { return _federateType; }
    const std::string& getFederationName() const
    { return _federationName; }

    /// Create a federation execution
    /// Semantically this methods should be static,
    /// but the nonstatic case could reuse the connection to the server
    /// FIXME: cannot determine from the return value if we created the execution
    virtual bool createFederationExecution(const std::string& federation, const std::string& objectModel) = 0;
    virtual bool destroyFederationExecution(const std::string& federation) = 0;

    /// Join with federateName the federation execution federation
    virtual bool join(const std::string& federateType, const std::string& federation) = 0;
    virtual bool resign() = 0;

    /// Synchronization Point handling
    virtual bool registerFederationSynchronizationPoint(const std::string& label, const RTIData& tag) = 0;
    virtual bool waitForFederationSynchronizationPointAnnounced(const std::string& label) = 0;
    virtual bool synchronizationPointAchieved(const std::string& label) = 0;
    virtual bool waitForFederationSynchronized(const std::string& label) = 0;

    /// Time management
    virtual bool enableTimeConstrained() = 0;
    virtual bool disableTimeConstrained() = 0;

    virtual bool enableTimeRegulation(const SGTimeStamp& lookahead) = 0;
    virtual bool disableTimeRegulation() = 0;

    virtual bool timeAdvanceRequestBy(const SGTimeStamp& dt) = 0;
    virtual bool timeAdvanceRequest(const SGTimeStamp& fedTime) = 0;

    virtual bool queryFederateTime(SGTimeStamp& timeStamp) = 0;
    virtual bool modifyLookahead(const SGTimeStamp& timeStamp) = 0;
    virtual bool queryLookahead(SGTimeStamp& timeStamp) = 0;

    /// Process messages
    virtual bool tick() = 0;
    virtual bool tick(const double& minimum, const double& maximum) = 0;

    virtual RTIObjectClass* createObjectClass(const std::string& name, HLAObjectClass* hlaObjectClass) = 0;
    // virtual RTIInteractionClass* createInteractionClass(const std::string& name) = 0;

    virtual RTIObjectInstance* getObjectInstance(const std::string& name) = 0;

protected:
    void setFederateType(const std::string& federateType)
    { _federateType = federateType; }
    void setFederationName(const std::string& federationName)
    { _federationName = federationName; }

private:
    RTIFederate(const RTIFederate&);
    RTIFederate& operator=(const RTIFederate&);

    /// The federates name
    std::string _federateType;

    /// The federation execution name
    std::string _federationName;
};

}

#endif
