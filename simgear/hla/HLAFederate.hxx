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

    /// The rti version backend to connect
    Version getVersion() const;
    bool setVersion(HLAFederate::Version version);
    bool setVersion(const std::string& version);

    /// The rti backends connect arguments, depends on the version
    const std::list<std::string>& getConnectArguments() const;
    bool setConnectArguments(const std::list<std::string>& connectArguments);

    /// If true try to create on join and try to destroy on resign
    bool getCreateFederationExecution() const;
    bool setCreateFederationExecution(bool createFederationExecution);

    /// The federation execution name to use on create, join and destroy
    const std::string& getFederationExecutionName() const;
    bool setFederationExecutionName(const std::string& federationExecutionName);

    /// The federation object model name to use on create and possibly join
    const std::string& getFederationObjectModel() const;
    bool setFederationObjectModel(const std::string& federationObjectModel);

    /// The federate type used on join
    const std::string& getFederateType() const;
    bool setFederateType(const std::string& federateType);

    /// The federate name possibly used on join
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

    /// If set to true, time constrained mode is entered on init
    bool getTimeConstrained() const;
    bool setTimeConstrained(bool timeConstrained);

    /// If set to true, time advance is constrained by the local system clock
    bool getTimeConstrainedByLocalClock() const;
    bool setTimeConstrainedByLocalClock(bool timeConstrainedByLocalClock);

    /// If set to true, time regulation mode is entered on init
    bool getTimeRegulating() const;
    bool setTimeRegulating(bool timeRegulating);

    /// If set to a non zero value, this federate leads the federations
    /// locical time advance by this amount of time.
    const SGTimeStamp& getLeadTime() const;
    bool setLeadTime(const SGTimeStamp& leadTime);

    /// The time increment for use in the default update method.
    const SGTimeStamp& getTimeIncrement() const;
    bool setTimeIncrement(const SGTimeStamp& timeIncrement);

    /// Actually enable time constrained mode.
    /// This method blocks until time constrained mode is enabled.
    bool enableTimeConstrained();
    /// Actually disable time constrained mode.
    bool disableTimeConstrained();

    /// Actually enable time constrained by local clock mode.
    bool enableTimeConstrainedByLocalClock();

    /// Actually enable time regulation mode.
    /// This method blocks until time regulation mode is enabled.
    bool enableTimeRegulation(const SGTimeStamp& lookahead);
    bool enableTimeRegulation();
    /// Actually disable time regulation mode.
    bool disableTimeRegulation();
    /// Actually modify the lookahead time.
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

    /// Get the current federates time
    bool queryFederateTime(SGTimeStamp& timeStamp);
    /// Get the current federates lookahead
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

    /// Read an rti1.3 omt xml file
    bool readRTI13ObjectModelTemplate(const std::string& objectModel);
    /// Read an rti1516 omt xml file
    bool readRTI1516ObjectModelTemplate(const std::string& objectModel);
    /// Read an rti1516e omt xml file
    bool readRTI1516EObjectModelTemplate(const std::string& objectModel);

    /// Is called past a successful join to populate the rti classes
    bool resolveObjectModel();

    /// Access data types
    const HLADataType* getDataType(const std::string& name) const;
    // virtual const HLADataType* createDataType(const std::string& name);
    bool insertDataType(const std::string& name, const SGSharedPtr<HLADataType>& dataType);
    void recomputeDataTypeAlignment();

    /// Get the interaction class of a given name
    HLAInteractionClass* getInteractionClass(const std::string& name);
    const HLAInteractionClass* getInteractionClass(const std::string& name) const;
    /// Default create function. Creates a default interaction class
    virtual HLAInteractionClass* createInteractionClass(const std::string& name);

    /// Get the object class of a given name
    HLAObjectClass* getObjectClass(const std::string& name);
    const HLAObjectClass* getObjectClass(const std::string& name) const;
    /// Default create function. Creates a default object class
    virtual HLAObjectClass* createObjectClass(const std::string& name);

    /// Get the object instance of a given name
    HLAObjectInstance* getObjectInstance(const std::string& name);
    const HLAObjectInstance* getObjectInstance(const std::string& name) const;
    virtual HLAObjectInstance* createObjectInstance(HLAObjectClass* objectClass, const std::string& name);

    /// Tells the main exec loop to continue or not.
    void setDone(bool done);
    bool getDone() const;

    /// The user overridable slot that is called to set up an object model
    /// By default, depending on the set up rti version, the apropriate
    ///  bool read{RTI13,RTI1516,RTI1516E}ObjectModelTemplate(const std::string& objectModel);
    /// method is called.
    /// Note that the RTI13 files do not contain any information about the data types.
    /// A user needs to set up the data types and assign them to the object classes/
    /// interaction classes theirselves.
    /// Past reading the object model, it is still possible to change the subscription/publication
    /// types without introducing traffic on the backend rti.
    virtual bool readObjectModel();

    virtual bool subscribe();
    virtual bool publish();

    virtual bool init();
    virtual bool update();
    virtual bool shutdown();

    virtual bool exec();

private:
    HLAFederate(const HLAFederate&);
    HLAFederate& operator=(const HLAFederate&);

    void _clearRTI();

    /// Internal helpers for interaction classes
    bool _insertInteractionClass(HLAInteractionClass* interactionClass);
    /// Internal helpers for object classes
    bool _insertObjectClass(HLAObjectClass* objectClass);
    /// Internal helpers for object instances
    bool _insertObjectInstance(HLAObjectInstance* objectInstance);
    void _eraseObjectInstance(const std::string& name);

    /// The underlying interface to the rti implementation
    SGSharedPtr<RTIFederate> _rtiFederate;

    /// Parameters required to connect to an rti
    Version _version;
    std::list<std::string> _connectArguments;

    /// Parameters for the federation execution
    std::string _federationExecutionName;
    std::string _federationObjectModel;
    bool _createFederationExecution;

    /// Parameters for the federate
    std::string _federateType;
    std::string _federateName;

    /// Time management related parameters
    /// If true, the federate is expected to enter time constrained mode
    bool _timeConstrained;
    /// If true, the federate is expected to enter time regulating mode
    bool _timeRegulating;
    /// The amount of time this federate leads the others.
    SGTimeStamp _leadTime;
    /// The regular time increment we do on calling update()
    SGTimeStamp _timeIncrement;
    /// The reference system time at initialization time.
    /// Is used to implement being time constrained on the
    /// local system time.
    bool _timeConstrainedByLocalClock;
    SGTimeStamp _localClockOffset;

    /// If true the exec method returns.
    bool _done;

    /// The Data Types by name
    typedef std::map<std::string, SGSharedPtr<HLADataType> > DataTypeMap;
    DataTypeMap _dataTypeMap;

    /// The Interaction Classes by name
    typedef std::map<std::string, SGSharedPtr<HLAInteractionClass> > InteractionClassMap;
    InteractionClassMap _interactionClassMap;

    /// The Object Classes by name
    typedef std::map<std::string, SGSharedPtr<HLAObjectClass> > ObjectClassMap;
    ObjectClassMap _objectClassMap;

    /// The Object Instances by name
    typedef std::map<std::string, SGSharedPtr<HLAObjectInstance> > ObjectInstanceMap;
    ObjectInstanceMap _objectInstanceMap;
    /// The Object Instances by name, the ones that have an explicit given name, may be not yet registered
    // ObjectInstanceMap _explicitNamedObjectInstanceMap;

    friend class HLAInteractionClass;
    friend class HLAObjectClass;
    friend class HLAObjectInstance;
};

} // namespace simgear

#endif
