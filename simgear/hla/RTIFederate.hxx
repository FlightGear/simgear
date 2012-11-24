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

#ifndef RTIFederate_hxx
#define RTIFederate_hxx

#include <string>
#include "simgear/structure/SGWeakReferenced.hxx"
#include "RTIInteractionClass.hxx"
#include "RTIObjectClass.hxx"
#include "RTIObjectInstance.hxx"

class SGTimeStamp;

namespace simgear {

class RTIFederate : public SGWeakReferenced {
public:
    RTIFederate();
    virtual ~RTIFederate();

    /// Create a federation execution
    /// Semantically this methods should be static,
    enum FederationManagementResult {
        FederationManagementSuccess,
        FederationManagementFail,
        FederationManagementFatal
    };

    virtual FederationManagementResult createFederationExecution(const std::string& federation, const std::string& objectModel) = 0;
    virtual FederationManagementResult destroyFederationExecution(const std::string& federation) = 0;

    /// Join with federateName the federation execution federation
    virtual FederationManagementResult join(const std::string& federateType, const std::string& federation) = 0;
    virtual bool resign() = 0;
    virtual bool getJoined() const = 0;

    /// Synchronization Point handling
    virtual bool registerFederationSynchronizationPoint(const std::string& label, const RTIData& tag) = 0;
    virtual bool getFederationSynchronizationPointAnnounced(const std::string& label) = 0;
    virtual bool synchronizationPointAchieved(const std::string& label) = 0;
    virtual bool getFederationSynchronized(const std::string& label) = 0;

    /// Time management
    virtual bool enableTimeConstrained() = 0;
    virtual bool disableTimeConstrained() = 0;
    virtual bool getTimeConstrainedEnabled() = 0;

    virtual bool enableTimeRegulation(const SGTimeStamp& lookahead) = 0;
    virtual bool disableTimeRegulation() = 0;
    virtual bool modifyLookahead(const SGTimeStamp& timeStamp) = 0;
    virtual bool getTimeRegulationEnabled() = 0;

    virtual bool timeAdvanceRequest(const SGTimeStamp& fedTime) = 0;
    virtual bool timeAdvanceRequestAvailable(const SGTimeStamp& timeStamp) = 0;
    virtual bool flushQueueRequest(const SGTimeStamp& timeStamp) = 0;
    virtual bool getTimeAdvancePending() = 0;

    virtual bool queryFederateTime(SGTimeStamp& timeStamp) = 0;
    virtual bool queryLookahead(SGTimeStamp& timeStamp) = 0;
    virtual bool queryGALT(SGTimeStamp& timeStamp) = 0;
    virtual bool queryLITS(SGTimeStamp& timeStamp) = 0;

    enum ProcessMessageResult {
        ProcessMessagePending,
        ProcessMessageLast,
        ProcessMessageFatal
    };

    /// Process messages
    virtual ProcessMessageResult processMessage() = 0;
    virtual ProcessMessageResult processMessages(const double& minimum, const double& maximum) = 0;

    virtual RTIObjectClass* createObjectClass(const std::string& name, HLAObjectClass* hlaObjectClass) = 0;
    virtual RTIInteractionClass* createInteractionClass(const std::string& name, HLAInteractionClass* interactionClass) = 0;

    virtual RTIObjectInstance* getObjectInstance(const std::string& name) = 0;

private:
    RTIFederate(const RTIFederate&);
    RTIFederate& operator=(const RTIFederate&);
};

}

#endif
