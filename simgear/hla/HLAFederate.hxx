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
    virtual ~HLAFederate();

    /// Get the name of the joined federate/federation
    const std::string& getFederateType() const;
    const std::string& getFederationName() const;

    /// Create a federation execution
    /// Semantically this methods should be static,
    /// but the nonstatic case could reuse the connection to the server
    /// FIXME: cannot determine from the return value if we created the execution
    bool createFederationExecution(const std::string& federation, const std::string& objectModel);
    bool destroyFederationExecution(const std::string& federation);

    /// Join with federateType the federation execution
    bool join(const std::string& federateType, const std::string& federation);
    bool resign();

    bool enableTimeConstrained();
    bool disableTimeConstrained();

    bool enableTimeRegulation(const SGTimeStamp& lookahead);
    bool disableTimeRegulation();

    bool timeAdvanceRequestBy(const SGTimeStamp& dt);
    bool timeAdvanceRequest(const SGTimeStamp& dt);

    bool queryFederateTime(SGTimeStamp& timeStamp);
    bool modifyLookahead(const SGTimeStamp& timeStamp);
    bool queryLookahead(SGTimeStamp& timeStamp);

    /// Process messages
    bool tick();
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

protected:
    HLAFederate(const SGSharedPtr<RTIFederate>& rtiFederate);

private:
    SGSharedPtr<RTIFederate> _rtiFederate;

    typedef std::map<std::string, SGSharedPtr<HLAObjectClass> > ObjectClassMap;
    ObjectClassMap _objectClassMap;

    typedef std::map<std::string, SGSharedPtr<HLAInteractionClass> > InteractionClassMap;
    InteractionClassMap _interactionClassMap;

    friend class HLAInteractionClass;
    friend class HLAObjectClass;
};

} // namespace simgear

#endif
