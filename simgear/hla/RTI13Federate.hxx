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

#ifndef RTI13Federate_hxx
#define RTI13Federate_hxx

#ifndef RTI_USES_STD_FSTREAM
#define RTI_USES_STD_FSTREAM
#endif

#include <RTI.hh>

#include "RTIFederate.hxx"
#include "RTI13ObjectClass.hxx"
#include "RTI13ObjectInstance.hxx"

namespace simgear {

class RTI13Ambassador;

class RTI13Federate : public RTIFederate {
public:
    RTI13Federate();
    virtual ~RTI13Federate();

    virtual bool createFederationExecution(const std::string& federation, const std::string& objectModel);
    virtual bool destroyFederationExecution(const std::string& federation);

    /// Join with federateName the federation execution federation
    virtual bool join(const std::string& federateType, const std::string& federation);
    virtual bool resign();

    /// Synchronization Point handling
    virtual bool registerFederationSynchronizationPoint(const std::string& label, const RTIData& tag);
    virtual bool waitForFederationSynchronizationPointAnnounced(const std::string& label);
    virtual bool synchronizationPointAchieved(const std::string& label);
    virtual bool waitForFederationSynchronized(const std::string& label);

    /// Time management
    virtual bool enableTimeConstrained();
    virtual bool disableTimeConstrained();

    virtual bool enableTimeRegulation(const SGTimeStamp& lookahead);
    virtual bool disableTimeRegulation();

    virtual bool timeAdvanceRequestBy(const SGTimeStamp& dt);
    virtual bool timeAdvanceRequest(const SGTimeStamp& fedTime);

    virtual bool queryFederateTime(SGTimeStamp& timeStamp);
    virtual bool modifyLookahead(const SGTimeStamp& timeStamp);
    virtual bool queryLookahead(SGTimeStamp& timeStamp);

    /// Process messages
    virtual bool tick();
    virtual bool tick(const double& minimum, const double& maximum);

    virtual RTI13ObjectClass* createObjectClass(const std::string& name, HLAObjectClass* hlaObjectClass);

    virtual RTI13ObjectInstance* getObjectInstance(const std::string& name);

private:
    RTI13Federate(const RTI13Federate&);
    RTI13Federate& operator=(const RTI13Federate&);

    /// The federate handle
    RTI::FederateHandle _federateHandle;

    /// The timeout for the single callback tick function in
    /// syncronous operations that need to wait for a callback
    double _tickTimeout;

    /// RTI connection
    SGSharedPtr<RTI13Ambassador> _ambassador;
};

}

#endif
