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

#ifndef HLAFederate_hxx
#define HLAFederate_hxx

#include <map>

#include "HLAObjectInstance.hxx"
#include "HLAObjectClass.hxx"
#include "HLAInteractionClass.hxx"

class SGTimeStamp;

namespace simgear {

class RTIFederate;

class HLAFederate : public SGWeakReferenced {
public:
    HLAFederate();
    virtual ~HLAFederate();

    enum Version {
        RTI13,
        RTI1516,
        RTI1516E
    };

    Version getVersion() const;
    bool setVersion(HLAFederate::Version version);

    const std::list<std::string>& getConnectArguments() const;
    bool setConnectArguments(const std::list<std::string>& connectArguments);

    const std::string& getFederationExecutionName() const;
    bool setFederationExecutionName(const std::string& federationExecutionName);

    const std::string& getFederationObjectModel() const;
    bool setFederationObjectModel(const std::string& federationObjectModel);

    const std::string& getFederateType() const;
    bool setFederateType(const std::string& federateType);

    const std::string& getFederateName() const;
    bool setFederateName(const std::string& federateName);

    /// connect to an rti
    bool connect(Version version, const std::list<std::string>& stringList);
    bool connect();
    bool disconnect();

    /// Create a federation execution
    /// Semantically this methods should be static,
    /// but the nonstatic case could reuse the connection to the server
    bool createFederationExecution(const std::string& federation, const std::string& objectModel);
    bool destroyFederationExecution(const std::string& federation);
    bool createFederationExecution();
    bool destroyFederationExecution();

    /// Join with federateType the federation execution
    bool join(const std::string& federateType, const std::string& federation);
    bool join();
    bool resign();
    
    /// Try to create and join the federation execution.
    bool createJoinFederationExecution();
    bool resignDestroyFederationExecution();


    /// Time management

    bool enableTimeConstrained();
    bool disableTimeConstrained();

    bool enableTimeRegulation(const SGTimeStamp& lookahead);
    bool disableTimeRegulation();
    bool modifyLookahead(const SGTimeStamp& lookahead);

    /// Advance the logical time by the given time increment.
    /// Depending on the time constrained mode, this might
    /// block until the time advance is granted.
    bool timeAdvanceBy(const SGTimeStamp& timeIncrement);
    /// Advance the logical time to the given time.
    /// Depending on the time constrained mode, this might
    /// block until the time advance is granted.
    bool timeAdvance(const SGTimeStamp& timeStamp);
    /// Advance the logical time as far as time advances are available.
    /// This call should not block and advance the logical time
    /// as far as currently possible.
    bool timeAdvanceAvailable();

    bool queryFederateTime(SGTimeStamp& timeStamp);
    bool queryLookahead(SGTimeStamp& timeStamp);

    /// Process one messsage
    bool processMessage();
    /// Process one message but do not wait longer than the relative timeout.
    bool processMessage(const SGTimeStamp& timeout);
    /// Process messages until the federate can proceed with the
    /// next simulation step. That is flush all pending messages and
    /// depending on the time constrained mode process messages until
    /// a pending time advance is granted.
    bool processMessages();

    /// Legacy tick call
    bool tick(const double& minimum, const double& maximum);

    class ObjectModelFactory {
    public:
        virtual ~ObjectModelFactory()
        { }

        virtual HLAObjectClass* createObjectClass(const std::string& name, HLAFederate& federate)
        { return new HLAObjectClass(name, federate); }
        virtual bool subscribeObjectClass(const std::string& objectClassName, const std::string& sharing)
        { return sharing.find("Subscribe") != std::string::npos; }
        virtual bool publishObjectClass(const std::string& objectClassName, const std::string& sharing)
        { return sharing.find("Publish") != std::string::npos; }
        virtual bool subscribeAttribute(const std::string& objectClassName, const std::string& attributeName, const std::string& sharing)
        { return sharing.find("Subscribe") != std::string::npos; }
        virtual bool publishAttribute(const std::string& objectClassName, const std::string& attributeName, const std::string& sharing)
        { return sharing.find("Publish") != std::string::npos; }

    };

    /// Read an omt xml file
    bool readObjectModelTemplate(const std::string& objectModel,
                                 ObjectModelFactory& objectModelFactory);

    HLAObjectClass* getObjectClass(const std::string& name);
    const HLAObjectClass* getObjectClass(const std::string& name) const;

    HLAInteractionClass* getInteractionClass(const std::string& name);
    const HLAInteractionClass* getInteractionClass(const std::string& name) const;

private:
    HLAFederate(const HLAFederate&);
    HLAFederate& operator=(const HLAFederate&);

    /// The underlying interface to the rti implementation
    SGSharedPtr<RTIFederate> _rtiFederate;

    /// Parameters required to connect to an rti
    Version _version;
    std::list<std::string> _connectArguments;

    /// Parameters for the federation execution
    std::string _federationExecutionName;
    std::string _federationObjectModel;

    /// Parameters for the federate
    std::string _federateType;
    std::string _federateName;

    typedef std::map<std::string, SGSharedPtr<HLAObjectClass> > ObjectClassMap;
    ObjectClassMap _objectClassMap;

    typedef std::map<std::string, SGSharedPtr<HLAInteractionClass> > InteractionClassMap;
    InteractionClassMap _interactionClassMap;

    friend class HLAInteractionClass;
    friend class HLAObjectClass;
};

} // namespace simgear

#endif
