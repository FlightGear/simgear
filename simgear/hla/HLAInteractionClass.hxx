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

#ifndef HLAInteractionClass_hxx
#define HLAInteractionClass_hxx

#include <map>
#include <string>
#include <vector>

#include <simgear/structure/SGWeakReferenced.hxx>

#include "HLADataElement.hxx"
#include "HLADataType.hxx"
#include "HLATypes.hxx"

namespace simgear {

class RTIInteractionClass;
class HLADataType;
class HLAFederate;

class HLAInteractionClass : public SGWeakReferenced {
public:
    HLAInteractionClass(const std::string& name, HLAFederate* federate);
    virtual ~HLAInteractionClass();

    const std::string& getName() const;

    /// return the federate this interaction class belongs to
    const SGWeakPtr<HLAFederate>& getFederate() const;

    HLASubscriptionType getSubscriptionType() const;
    void setSubscriptionType(HLASubscriptionType subscriptionType);

    HLAPublicationType getPublicationType() const;
    void setPublicationType(HLAPublicationType publicationType);

    unsigned getNumParameters() const;
    unsigned addParameter(const std::string& name);

    unsigned getParameterIndex(const std::string& name) const;
    std::string getParameterName(unsigned index) const;

    const HLADataType* getParameterDataType(unsigned index) const;
    void setParameterDataType(unsigned index, const SGSharedPtr<const HLADataType>& dataType);

    /// Get the attribute data element index for the given path, return true if successful
    bool getDataElementIndex(HLADataElementIndex& dataElementIndex, const std::string& path) const;
    HLADataElementIndex getDataElementIndex(const std::string& path) const;

    virtual bool subscribe();
    virtual bool unsubscribe();

    virtual bool publish();
    virtual bool unpublish();

private:
    HLAInteractionClass(const HLAInteractionClass&);
    HLAInteractionClass& operator=(const HLAInteractionClass&);

    void _setRTIInteractionClass(RTIInteractionClass* interactionClass);
    void _resolveParameterIndex(const std::string& name, unsigned index);
    void _clearRTIInteractionClass();

    struct Parameter {
        Parameter() {}
        Parameter(const std::string& name) : _name(name) {}
        std::string _name;
        SGSharedPtr<const HLADataType> _dataType;
    };
    typedef std::vector<Parameter> ParameterVector;
    typedef std::map<std::string,unsigned> NameIndexMap;

    /// The parent federate.
    SGWeakPtr<HLAFederate> _federate;

    /// The rti class if already instantiated.
    RTIInteractionClass* _rtiInteractionClass;

    /// The interaction class name
    std::string _name;

    /// The configured subscription and publication type
    HLASubscriptionType _subscriptionType;
    HLAPublicationType _publicationType;

    /// The parameter data
    ParameterVector _parameterVector;
    /// The mapping from parameter names to parameter indices
    NameIndexMap _nameIndexMap;

    friend class HLAFederate;
};

} // namespace simgear

#endif
