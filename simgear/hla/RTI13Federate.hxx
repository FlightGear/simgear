// Copyright (C) 2009 - 2011  Mathias Froehlich - Mathias.Froehlich@web.de
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
#include "RTI13InteractionClass.hxx"
#include "RTI13ObjectClass.hxx"
#include "RTI13ObjectInstance.hxx"

namespace simgear {

class RTI13Ambassador;

class RTI13Federate : public RTIFederate {
public:
    RTI13Federate(const std::list<std::string>& stringList);
    virtual ~RTI13Federate();

    /// Create a federation execution
    /// Semantically this methods should be static,
    virtual FederationManagementResult createFederationExecution(const std::string& federation, const std::string& objectModel);
    virtual FederationManagementResult destroyFederationExecution(const std::string& federation);

    /// Join with federateName the federation execution federation
    virtual FederationManagementResult join(const std::string& federateType, const std::string& federation);
    virtual bool resign();
    virtual bool getJoined() const;

    /// Synchronization Point handling
    virtual bool registerFederationSynchronizationPoint(const std::string& label, const RTIData& tag);
    virtual bool getFederationSynchronizationPointAnnounced(const std::string& label);
    virtual bool synchronizationPointAchieved(const std::string& label);
    virtual bool getFederationSynchronized(const std::string& label);

    /// Time management
    virtual bool enableTimeConstrained();
    virtual bool disableTimeConstrained();
    virtual bool getTimeConstrainedEnabled();

    virtual bool enableTimeRegulation(const SGTimeStamp& lookahead);
    virtual bool disableTimeRegulation();
    virtual bool modifyLookahead(const SGTimeStamp& timeStamp);
    virtual bool getTimeRegulationEnabled();

    virtual bool timeAdvanceRequest(const SGTimeStamp& timeStamp);
    virtual bool timeAdvanceRequestAvailable(const SGTimeStamp& timeStamp);
    virtual bool flushQueueRequest(const SGTimeStamp& timeStamp);
    virtual bool getTimeAdvancePending();

    virtual bool queryFederateTime(SGTimeStamp& timeStamp);
    virtual bool queryLookahead(SGTimeStamp& timeStamp);
    virtual bool queryGALT(SGTimeStamp& timeStamp);
    virtual bool queryLITS(SGTimeStamp& timeStamp);

    /// Process messages
    virtual ProcessMessageResult processMessage();
    virtual ProcessMessageResult processMessages(const double& minimum, const double& maximum);

    // helper functions for the above
    ProcessMessageResult _tick();
    ProcessMessageResult _tick(const double& minimum, const double& maximum);

    virtual RTI13ObjectClass* createObjectClass(const std::string& name, HLAObjectClass* hlaObjectClass);
    virtual RTI13InteractionClass* createInteractionClass(const std::string& name, HLAInteractionClass* interactionClass);

    virtual RTI13ObjectInstance* getObjectInstance(const std::string& name);
    void insertObjectInstance(RTI13ObjectInstance* objectInstance);

private:
    RTI13Federate(const RTI13Federate&);
    RTI13Federate& operator=(const RTI13Federate&);

    /// The federate handle
    RTI::FederateHandle _federateHandle;
    bool _joined;

    /// RTI connection
    SGSharedPtr<RTI13Ambassador> _ambassador;

    /// Callbacks from the rti are handled here.
    struct FederateAmbassador;
    FederateAmbassador* _federateAmbassador;
};

}

#endif
