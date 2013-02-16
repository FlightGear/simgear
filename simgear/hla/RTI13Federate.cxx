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

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include <simgear/compiler.h>

#include "RTI13Federate.hxx"

#include "RTI13Ambassador.hxx"

namespace simgear {

static std::string toStdString(const char* n)
{
    if (!n)
        return std::string();
    return std::string(n);
}

/// Just the interface class doing the callbacks into the parent class
struct RTI13Federate::FederateAmbassador : public RTI::FederateAmbassador {
    FederateAmbassador() :
        _timeRegulationEnabled(false),
        _timeConstrainedEnabled(false),
        _timeAdvancePending(false)
    {
    }
    virtual ~FederateAmbassador()
        throw (RTI::FederateInternalError)
    {
    }

    /// Generic callback to execute some notification on objects in a way that they are not prone to
    /// ConcurrentAccess exceptions.
    class QueueCallback : public SGReferenced {
    public:
        virtual ~QueueCallback() {}
        virtual void operator()(FederateAmbassador& self) = 0;
    };
    class TagQueueCallback : public QueueCallback {
    public:
        TagQueueCallback(const char* tag)
        {
            if (tag)
                _tag.setData(tag, std::strlen(tag) + 1);
            else
                _tag.setData("", 1);
        }
        virtual ~TagQueueCallback()
        { }
        RTIData _tag;
    };

    /// RTI federate ambassador callback functions.
    virtual void synchronizationPointRegistrationSucceeded(const char* label)
        throw (RTI::FederateInternalError)
    {
    }

    virtual void synchronizationPointRegistrationFailed(const char* label)
        throw (RTI::FederateInternalError)
    {
    }

    virtual void announceSynchronizationPoint(const char* label, const char* tag)
        throw (RTI::FederateInternalError)
    {
        _pendingSyncLabels.insert(toStdString(label));
    }

    virtual void federationSynchronized(const char* label)
        throw (RTI::FederateInternalError)
    {
        std::string s = toStdString(label);
        _pendingSyncLabels.erase(s);
        _syncronizedSyncLabels.insert(s);
    }

    virtual void initiateFederateSave(const char* label)
        throw (RTI::UnableToPerformSave,
               RTI::FederateInternalError)
    {
    }

    virtual void federationSaved()
        throw (RTI::FederateInternalError)
    {
    }

    virtual void federationNotSaved()
        throw (RTI::FederateInternalError)
    {
    }

    virtual void requestFederationRestoreSucceeded(const char* label)
        throw (RTI::FederateInternalError)
    {
    }

    virtual void requestFederationRestoreFailed(const char* label, const char* reason)
        throw (RTI::FederateInternalError)
    {
    }

    virtual void federationRestoreBegun()
        throw (RTI::FederateInternalError)
    {
    }

    virtual void initiateFederateRestore(const char* label, RTI::FederateHandle federateHandle)
        throw (RTI::SpecifiedSaveLabelDoesNotExist,
               RTI::CouldNotRestore,
               RTI::FederateInternalError)
    {
    }

    virtual void federationRestored()
        throw (RTI::FederateInternalError)
    {
    }

    virtual void federationNotRestored()
        throw (RTI::FederateInternalError)
    {
    }

    // Declaration Management
    class StartRegistrationForObjectClassCallback : public QueueCallback {
    public:
        StartRegistrationForObjectClassCallback(RTI::ObjectClassHandle objectClassHandle) :
            _objectClassHandle(objectClassHandle)
        { }
        virtual void operator()(FederateAmbassador& self)
        { self.startRegistrationForObjectClassCallback(_objectClassHandle); }
    private:
        RTI::ObjectClassHandle _objectClassHandle;
    };
    virtual void startRegistrationForObjectClass(RTI::ObjectClassHandle objectClassHandle)
        throw (RTI::ObjectClassNotPublished,
               RTI::FederateInternalError)
    { _queueCallbackList.push_back(new StartRegistrationForObjectClassCallback(objectClassHandle)); }
    void startRegistrationForObjectClassCallback(RTI::ObjectClassHandle objectClassHandle)
    {
        ObjectClassMap::iterator i = _objectClassMap.find(objectClassHandle);
        if (i == _objectClassMap.end())
            return;
        if (!i->second.valid())
            return;
        i->second->startRegistration();
    }

    class StopRegistrationForObjectClassCallback : public QueueCallback {
    public:
        StopRegistrationForObjectClassCallback(RTI::ObjectClassHandle objectClassHandle) :
            _objectClassHandle(objectClassHandle)
        { }
        virtual void operator()(FederateAmbassador& self)
        { self.stopRegistrationForObjectClassCallback(_objectClassHandle); }
    private:
        RTI::ObjectClassHandle _objectClassHandle;
    };
    virtual void stopRegistrationForObjectClass(RTI::ObjectClassHandle objectClassHandle)
        throw (RTI::ObjectClassNotPublished,
               RTI::FederateInternalError)
    { _queueCallbackList.push_back(new StopRegistrationForObjectClassCallback(objectClassHandle)); }
    void stopRegistrationForObjectClassCallback(RTI::ObjectClassHandle objectClassHandle)
    {
        ObjectClassMap::iterator i = _objectClassMap.find(objectClassHandle);
        if (i == _objectClassMap.end())
            return;
        if (!i->second.valid())
            return;
        i->second->stopRegistration();
    }

    virtual void turnInteractionsOn(RTI::InteractionClassHandle interactionClassHandle)
        throw (RTI::InteractionClassNotPublished,
               RTI::FederateInternalError)
    {
    }

    virtual void turnInteractionsOff(RTI::InteractionClassHandle interactionClassHandle)
        throw (RTI::InteractionClassNotPublished,
               RTI::FederateInternalError)
    {
    }

    // Object Management
    class DiscoverObjectCallback : public TagQueueCallback {
    public:
        DiscoverObjectCallback(RTI::ObjectHandle objectHandle, RTI::ObjectClassHandle objectClassHandle, const char *tag) :
            TagQueueCallback(tag),
            _objectHandle(objectHandle),
            _objectClassHandle(objectClassHandle)
        { }
        virtual void operator()(FederateAmbassador& self)
        { self.discoverObjectInstanceCallback(_objectHandle, _objectClassHandle, _tag); }
    private:
        RTI::ObjectHandle _objectHandle;
        RTI::ObjectClassHandle _objectClassHandle;
    };
    virtual void discoverObjectInstance(RTI::ObjectHandle objectHandle, RTI::ObjectClassHandle objectClassHandle, const char* tag)
        throw (RTI::CouldNotDiscover,
               RTI::ObjectClassNotKnown,
               RTI::FederateInternalError)
    { _queueCallbackList.push_back(new DiscoverObjectCallback(objectHandle, objectClassHandle, tag)); }
    void discoverObjectInstanceCallback(RTI::ObjectHandle objectHandle, RTI::ObjectClassHandle objectClassHandle, const RTIData& tag)
    {
        ObjectClassMap::iterator i = _objectClassMap.find(objectClassHandle);
        if (i == _objectClassMap.end())
            return;
        if (!i->second.valid())
            return;
        SGSharedPtr<RTI13ObjectInstance> objectInstance = new RTI13ObjectInstance(objectHandle, 0, i->second, _rtiAmbassador.get());
        _objectInstanceMap[objectHandle] = objectInstance;
        i->second->discoverInstance(objectInstance.get(), tag);
    }

    class ReflectAttributeValuesTimestampCallback : public TagQueueCallback {
    public:
        ReflectAttributeValuesTimestampCallback(RTI::ObjectHandle objectHandle,
                                                RTI13AttributeHandleDataPairList& attributeHandleDataPairList,
                                                const SGTimeStamp& timeStamp, const char *tag) :
            TagQueueCallback(tag),
            _objectHandle(objectHandle),
            _timeStamp(timeStamp)
        {
            _attributeHandleDataPairList.swap(attributeHandleDataPairList);
        }
        virtual void operator()(FederateAmbassador& self)
        {
            self.reflectAttributeValuesCallback(_objectHandle, _attributeHandleDataPairList, _timeStamp, _tag);
            self.freeAttributeHandleDataPairList(_attributeHandleDataPairList);
        }
    private:
        RTI::ObjectHandle _objectHandle;
        RTI13AttributeHandleDataPairList _attributeHandleDataPairList;
        SGTimeStamp _timeStamp;
    };
    virtual void reflectAttributeValues(RTI::ObjectHandle objectHandle, const RTI::AttributeHandleValuePairSet& attributeValuePairSet,
                                        const RTI::FedTime& fedTime, const char* tag, RTI::EventRetractionHandle eventRetractionHandle)
        throw (RTI::ObjectNotKnown,
               RTI::AttributeNotKnown,
               RTI::FederateOwnsAttributes,
               RTI::InvalidFederationTime,
               RTI::FederateInternalError)
    {
        RTI13AttributeHandleDataPairList attributeHandleDataPairList;

        RTI::ULong numAttribs = attributeValuePairSet.size();
        for (RTI::ULong i = 0; i < numAttribs; ++i) {
            appendAttributeHandleDataPair(attributeHandleDataPairList);
            attributeHandleDataPairList.back().first = attributeValuePairSet.getHandle(i);
            RTI::ULong length = attributeValuePairSet.getValueLength(i);
            attributeHandleDataPairList.back().second.resize(length);
            attributeValuePairSet.getValue(i, attributeHandleDataPairList.back().second.data(), length);
        }

        _queueCallbackList.push_back(new ReflectAttributeValuesTimestampCallback(objectHandle, attributeHandleDataPairList,
                                                                                 RTI13Ambassador::toTimeStamp(fedTime), tag));
    }
    void reflectAttributeValuesCallback(RTI::ObjectHandle objectHandle, RTI13AttributeHandleDataPairList& attributeHandleDataPairList,
                                        const SGTimeStamp& timeStamp, const RTIData& tag)
    {
        ObjectInstanceMap::iterator i = _objectInstanceMap.find(objectHandle);
        if (i == _objectInstanceMap.end())
            return;
        if (!i->second.valid())
            return;
        i->second->reflectAttributeValues(attributeHandleDataPairList, timeStamp, tag, _indexPool);
    }

    class ReflectAttributeValuesCallback : public TagQueueCallback {
    public:
        ReflectAttributeValuesCallback(RTI::ObjectHandle objectHandle, RTI13AttributeHandleDataPairList& attributeHandleDataPairList,
                                       const char *tag) :
            TagQueueCallback(tag),
            _objectHandle(objectHandle)
        {
            _attributeHandleDataPairList.swap(attributeHandleDataPairList);
        }
        virtual void operator()(FederateAmbassador& self)
        {
            self.reflectAttributeValuesCallback(_objectHandle, _attributeHandleDataPairList, _tag);
            self.freeAttributeHandleDataPairList(_attributeHandleDataPairList);
        }
    private:
        RTI::ObjectHandle _objectHandle;
        RTI13AttributeHandleDataPairList _attributeHandleDataPairList;
    };
    virtual void reflectAttributeValues(RTI::ObjectHandle objectHandle, const RTI::AttributeHandleValuePairSet& attributeValuePairSet,
                                        const char* tag)
        throw (RTI::ObjectNotKnown,
               RTI::AttributeNotKnown,
               RTI::FederateOwnsAttributes,
               RTI::FederateInternalError)
    {
        RTI13AttributeHandleDataPairList attributeHandleDataPairList;

        RTI::ULong numAttribs = attributeValuePairSet.size();
        for (RTI::ULong i = 0; i < numAttribs; ++i) {
            appendAttributeHandleDataPair(attributeHandleDataPairList);
            attributeHandleDataPairList.back().first = attributeValuePairSet.getHandle(i);
            RTI::ULong length = attributeValuePairSet.getValueLength(i);
            attributeHandleDataPairList.back().second.resize(length);
            attributeValuePairSet.getValue(i, attributeHandleDataPairList.back().second.data(), length);
        }

        _queueCallbackList.push_back(new ReflectAttributeValuesCallback(objectHandle, attributeHandleDataPairList, tag));
    }
    void reflectAttributeValuesCallback(RTI::ObjectHandle objectHandle, RTI13AttributeHandleDataPairList& attributeHandleDataPairList,
                                        const RTIData& tag)
    {
        ObjectInstanceMap::iterator i = _objectInstanceMap.find(objectHandle);
        if (i == _objectInstanceMap.end())
            return;
        if (!i->second.valid())
            return;
        i->second->reflectAttributeValues(attributeHandleDataPairList, tag, _indexPool);
    }

    virtual void receiveInteraction(RTI::InteractionClassHandle interactionClassHandle, const RTI::ParameterHandleValuePairSet& parameters,
                                    const RTI::FedTime& fedTime, const char* tag, RTI::EventRetractionHandle eventRetractionHandle)
        throw (RTI::InteractionClassNotKnown,
               RTI::InteractionParameterNotKnown,
               RTI::InvalidFederationTime,
               RTI::FederateInternalError)
    {
    }

    virtual void receiveInteraction(RTI::InteractionClassHandle interactionClassHandle,
                                    const RTI::ParameterHandleValuePairSet& parameters, const char* tag)
        throw (RTI::InteractionClassNotKnown,
               RTI::InteractionParameterNotKnown,
               RTI::FederateInternalError)
    {
    }

    class RemoveObjectTimestampCallback : public TagQueueCallback {
    public:
        RemoveObjectTimestampCallback(RTI::ObjectHandle objectHandle, const SGTimeStamp& timeStamp, const char* tag) :
            TagQueueCallback(tag),
            _objectHandle(objectHandle),
            _timeStamp(timeStamp)
        { }
        virtual void operator()(FederateAmbassador& self)
        { self.removeObjectInstanceCallback(_objectHandle, _timeStamp, _tag); }
    private:
        RTI::ObjectHandle _objectHandle;
        SGTimeStamp _timeStamp;
    };
    virtual void removeObjectInstance(RTI::ObjectHandle objectHandle, const RTI::FedTime& fedTime,
                                      const char* tag, RTI::EventRetractionHandle eventRetractionHandle)
        throw (RTI::ObjectNotKnown,
               RTI::InvalidFederationTime,
               RTI::FederateInternalError)
    { _queueCallbackList.push_back(new RemoveObjectTimestampCallback(objectHandle, RTI13Ambassador::toTimeStamp(fedTime), tag)); }
    void removeObjectInstanceCallback(RTI::ObjectHandle objectHandle, const SGTimeStamp& timeStamp, const RTIData& tag)
    {
        ObjectInstanceMap::iterator i = _objectInstanceMap.find(objectHandle);
        if (i == _objectInstanceMap.end())
            return;
        if (i->second.valid())
            i->second->removeInstance(tag);
        _objectInstanceMap.erase(i);
    }

    class RemoveObjectCallback : public TagQueueCallback {
    public:
        RemoveObjectCallback(RTI::ObjectHandle objectHandle, const char* tag) :
            TagQueueCallback(tag),
            _objectHandle(objectHandle)
        { }
        virtual void operator()(FederateAmbassador& self)
        { self.removeObjectInstanceCallback(_objectHandle, _tag); }
    private:
        RTI::ObjectHandle _objectHandle;
    };
    virtual void removeObjectInstance(RTI::ObjectHandle objectHandle, const char* tag)
        throw (RTI::ObjectNotKnown,
               RTI::FederateInternalError)
    { _queueCallbackList.push_back(new RemoveObjectCallback(objectHandle, tag)); }
    void removeObjectInstanceCallback(RTI::ObjectHandle objectHandle, const RTIData& tag)
    {
        ObjectInstanceMap::iterator i = _objectInstanceMap.find(objectHandle);
        if (i == _objectInstanceMap.end())
            return;
        if (i->second.valid())
            i->second->removeInstance(tag);
        _objectInstanceMap.erase(i);
    }

    class AttributeHandleSetCallback : public QueueCallback {
    public:
        AttributeHandleSetCallback(RTI::ObjectHandle objectHandle, const RTI::AttributeHandleSet& attributeHandleSet) :
            _objectHandle(objectHandle)
        {
            RTI::ULong numAttribs = attributeHandleSet.size();
            _attributes.reserve(numAttribs);
            for (RTI::ULong i = 0; i < numAttribs; ++i)
                _attributes.push_back(attributeHandleSet.getHandle(i));
        }
    protected:
        RTI::ObjectHandle _objectHandle;
        std::vector<RTI::AttributeHandle> _attributes;
    };
    class AttributesInScopeCallback : public AttributeHandleSetCallback {
    public:
        AttributesInScopeCallback(RTI::ObjectHandle objectHandle, const RTI::AttributeHandleSet& attributes) :
            AttributeHandleSetCallback(objectHandle, attributes)
        { }
        virtual void operator()(FederateAmbassador& self)
        { self.attributesInScopeCallback(_objectHandle, _attributes); }
    };
    virtual void attributesInScope(RTI::ObjectHandle objectHandle, const RTI::AttributeHandleSet& attributes)
        throw (RTI::ObjectNotKnown,
               RTI::AttributeNotKnown,
               RTI::FederateInternalError)
    { _queueCallbackList.push_back(new AttributesInScopeCallback(objectHandle, attributes)); }
    void attributesInScopeCallback(RTI::ObjectHandle objectHandle, const std::vector<RTI::AttributeHandle>& attributes)
    {
        ObjectInstanceMap::iterator i = _objectInstanceMap.find(objectHandle);
        if (i == _objectInstanceMap.end())
            return;
        if (!i->second.valid())
            return;
        i->second->attributesInScope(attributes);
    }

    class AttributesOutOfScopeCallback : public AttributeHandleSetCallback {
    public:
        AttributesOutOfScopeCallback(RTI::ObjectHandle objectHandle, const RTI::AttributeHandleSet& attributes) :
            AttributeHandleSetCallback(objectHandle, attributes)
        { }
        virtual void operator()(FederateAmbassador& self)
        { self.attributesOutOfScopeCallback(_objectHandle, _attributes); }
    };
    virtual void attributesOutOfScope(RTI::ObjectHandle objectHandle, const RTI::AttributeHandleSet& attributes)
        throw (RTI::ObjectNotKnown,
               RTI::AttributeNotKnown,
               RTI::FederateInternalError)
    { _queueCallbackList.push_back(new AttributesOutOfScopeCallback(objectHandle, attributes)); }
    void attributesOutOfScopeCallback(RTI::ObjectHandle objectHandle, const std::vector<RTI::AttributeHandle>& attributes)
    {
        ObjectInstanceMap::iterator i = _objectInstanceMap.find(objectHandle);
        if (i == _objectInstanceMap.end())
            return;
        if (!i->second.valid())
            return;
        i->second->attributesOutOfScope(attributes);
    }

    class ProvideAttributeValueUpdateCallback : public AttributeHandleSetCallback {
    public:
        ProvideAttributeValueUpdateCallback(RTI::ObjectHandle objectHandle, const RTI::AttributeHandleSet& attributes) :
            AttributeHandleSetCallback(objectHandle, attributes)
        { }
        virtual void operator()(FederateAmbassador& self)
        { self.provideAttributeValueUpdateCallback(_objectHandle, _attributes); }
    };
    virtual void provideAttributeValueUpdate(RTI::ObjectHandle objectHandle, const RTI::AttributeHandleSet& attributes)
        throw (RTI::ObjectNotKnown,
               RTI::AttributeNotKnown,
               RTI::AttributeNotOwned,
               RTI::FederateInternalError)
    { _queueCallbackList.push_back(new ProvideAttributeValueUpdateCallback(objectHandle, attributes)); }
    void provideAttributeValueUpdateCallback(RTI::ObjectHandle objectHandle, const std::vector<RTI::AttributeHandle>& attributes)
    {
        ObjectInstanceMap::iterator i = _objectInstanceMap.find(objectHandle);
        if (i == _objectInstanceMap.end())
            return;
        if (!i->second.valid())
            return;
        i->second->provideAttributeValueUpdate(attributes);
    }

    class TurnUpdatesOnForObjectInstanceCallback : public AttributeHandleSetCallback {
    public:
        TurnUpdatesOnForObjectInstanceCallback(RTI::ObjectHandle objectHandle, const RTI::AttributeHandleSet& attributes) :
            AttributeHandleSetCallback(objectHandle, attributes)
        { }
        virtual void operator()(FederateAmbassador& self)
        { self.turnUpdatesOnForObjectInstanceCallback(_objectHandle, _attributes); }
    };
    virtual void turnUpdatesOnForObjectInstance(RTI::ObjectHandle objectHandle, const RTI::AttributeHandleSet& attributes)
        throw (RTI::ObjectNotKnown,
               RTI::AttributeNotOwned,
               RTI::FederateInternalError)
    { _queueCallbackList.push_back(new TurnUpdatesOnForObjectInstanceCallback(objectHandle, attributes)); }
    void turnUpdatesOnForObjectInstanceCallback(RTI::ObjectHandle objectHandle, const std::vector<RTI::AttributeHandle>& attributes)
    {
        ObjectInstanceMap::iterator i = _objectInstanceMap.find(objectHandle);
        if (i == _objectInstanceMap.end())
            return;
        if (!i->second.valid())
            return;
        i->second->turnUpdatesOnForObjectInstance(attributes);
    }

    class TurnUpdatesOffForObjectInstanceCallback : public AttributeHandleSetCallback {
    public:
        TurnUpdatesOffForObjectInstanceCallback(RTI::ObjectHandle objectHandle, const RTI::AttributeHandleSet& attributes) :
            AttributeHandleSetCallback(objectHandle, attributes)
        { }
        virtual void operator()(FederateAmbassador& self)
        { self.turnUpdatesOffForObjectInstanceCallback(_objectHandle, _attributes); }
    };
    virtual void turnUpdatesOffForObjectInstance(RTI::ObjectHandle objectHandle, const RTI::AttributeHandleSet& attributes)
        throw (RTI::ObjectNotKnown,
               RTI::AttributeNotOwned,
               RTI::FederateInternalError)
    { _queueCallbackList.push_back(new TurnUpdatesOffForObjectInstanceCallback(objectHandle, attributes)); }
    void turnUpdatesOffForObjectInstanceCallback(RTI::ObjectHandle objectHandle, const std::vector<RTI::AttributeHandle>& attributes)
    {
        ObjectInstanceMap::iterator i = _objectInstanceMap.find(objectHandle);
        if (i == _objectInstanceMap.end())
            return;
        if (!i->second.valid())
            return;
        i->second->turnUpdatesOffForObjectInstance(attributes);
    }

    // Ownership Management
    class RequestAttributeOwnershipAssumptionCallback : public AttributeHandleSetCallback {
    public:
        RequestAttributeOwnershipAssumptionCallback(RTI::ObjectHandle objectHandle, const RTI::AttributeHandleSet& attributes, const RTIData& tag) :
            AttributeHandleSetCallback(objectHandle, attributes),
            _tag(tag)
        { }
        virtual void operator()(FederateAmbassador& self)
        { self.requestAttributeOwnershipAssumptionCallback(_objectHandle, _attributes, _tag); }
    protected:
        RTIData _tag;
    };
    virtual void requestAttributeOwnershipAssumption(RTI::ObjectHandle objectHandle,
                                                     const RTI::AttributeHandleSet& attributes, const char* tag)
        throw (RTI::ObjectNotKnown,
               RTI::AttributeNotKnown,
               RTI::AttributeAlreadyOwned,
               RTI::AttributeNotPublished,
               RTI::FederateInternalError)
    { _queueCallbackList.push_back(new RequestAttributeOwnershipAssumptionCallback(objectHandle, attributes, tagToData(tag))); }
    void requestAttributeOwnershipAssumptionCallback(RTI::ObjectHandle objectHandle, std::vector<RTI::AttributeHandle>& attributes, const RTIData& tag)
    {
        ObjectInstanceMap::iterator i = _objectInstanceMap.find(objectHandle);
        if (i == _objectInstanceMap.end())
            return;
        if (!i->second.valid())
            return;
        i->second->requestAttributeOwnershipAssumption(attributes, tag);
    }

    class AttributeOwnershipDivestitureNotificationCallback : public AttributeHandleSetCallback {
    public:
        AttributeOwnershipDivestitureNotificationCallback(RTI::ObjectHandle objectHandle, const RTI::AttributeHandleSet& attributes) :
            AttributeHandleSetCallback(objectHandle, attributes)
        { }
        virtual void operator()(FederateAmbassador& self)
        { self.attributeOwnershipDivestitureNotificationCallback(_objectHandle, _attributes); }
    };
    virtual void attributeOwnershipDivestitureNotification(RTI::ObjectHandle objectHandle, const RTI::AttributeHandleSet& attributes)
        throw (RTI::ObjectNotKnown,
               RTI::AttributeNotKnown,
               RTI::AttributeNotOwned,
               RTI::AttributeDivestitureWasNotRequested,
               RTI::FederateInternalError)
    { _queueCallbackList.push_back(new AttributeOwnershipDivestitureNotificationCallback(objectHandle, attributes)); }
    void attributeOwnershipDivestitureNotificationCallback(RTI::ObjectHandle objectHandle, const std::vector<RTI::AttributeHandle>& attributes)
    {
        ObjectInstanceMap::iterator i = _objectInstanceMap.find(objectHandle);
        if (i == _objectInstanceMap.end())
            return;
        if (!i->second.valid())
            return;
        i->second->attributeOwnershipDivestitureNotification(attributes);
    }

    class AttributeOwnershipAcquisitionNotificationCallback : public AttributeHandleSetCallback {
    public:
        AttributeOwnershipAcquisitionNotificationCallback(RTI::ObjectHandle objectHandle, const RTI::AttributeHandleSet& attributes) :
            AttributeHandleSetCallback(objectHandle, attributes)
        { }
        virtual void operator()(FederateAmbassador& self)
        { self.attributeOwnershipAcquisitionNotificationCallback(_objectHandle, _attributes); }
    };
    virtual void attributeOwnershipAcquisitionNotification(RTI::ObjectHandle objectHandle, const RTI::AttributeHandleSet& attributes)
        throw (RTI::ObjectNotKnown,
               RTI::AttributeNotKnown,
               RTI::AttributeAcquisitionWasNotRequested,
               RTI::AttributeAlreadyOwned,
               RTI::AttributeNotPublished,
               RTI::FederateInternalError)
    { _queueCallbackList.push_back(new AttributeOwnershipAcquisitionNotificationCallback(objectHandle, attributes)); }
    void attributeOwnershipAcquisitionNotificationCallback(RTI::ObjectHandle objectHandle, const std::vector<RTI::AttributeHandle>& attributes)
    {
        ObjectInstanceMap::iterator i = _objectInstanceMap.find(objectHandle);
        if (i == _objectInstanceMap.end())
            return;
        if (!i->second.valid())
            return;
        i->second->attributeOwnershipAcquisitionNotification(attributes);
    }

    class AttributeOwnershipUnavailableCallback : public AttributeHandleSetCallback {
    public:
        AttributeOwnershipUnavailableCallback(RTI::ObjectHandle objectHandle, const RTI::AttributeHandleSet& attributes) :
            AttributeHandleSetCallback(objectHandle, attributes)
        { }
        virtual void operator()(FederateAmbassador& self)
        { self.attributeOwnershipUnavailableCallback(_objectHandle, _attributes); }
    };
    virtual void attributeOwnershipUnavailable(RTI::ObjectHandle objectHandle, const RTI::AttributeHandleSet& attributes)
        throw (RTI::ObjectNotKnown,
               RTI::AttributeNotKnown,
               RTI::AttributeAlreadyOwned,
               RTI::AttributeAcquisitionWasNotRequested,
               RTI::FederateInternalError)
    { _queueCallbackList.push_back(new AttributeOwnershipUnavailableCallback(objectHandle, attributes)); }
    void attributeOwnershipUnavailableCallback(RTI::ObjectHandle objectHandle, const std::vector<RTI::AttributeHandle>& attributes)
    {
        ObjectInstanceMap::iterator i = _objectInstanceMap.find(objectHandle);
        if (i == _objectInstanceMap.end())
            return;
        if (!i->second.valid())
            return;
        i->second->attributeOwnershipUnavailable(attributes);
    }

    class RequestAttributeOwnershipReleaseCallback : public AttributeHandleSetCallback {
    public:
        RequestAttributeOwnershipReleaseCallback(RTI::ObjectHandle objectHandle, const RTI::AttributeHandleSet& attributes, const RTIData& tag) :
            AttributeHandleSetCallback(objectHandle, attributes),
            _tag(tag)
        { }
        virtual void operator()(FederateAmbassador& self)
        { self.requestAttributeOwnershipReleaseCallback(_objectHandle, _attributes, _tag); }
    protected:
        RTIData _tag;
    };
    virtual void requestAttributeOwnershipRelease(RTI::ObjectHandle objectHandle,
                                                  const RTI::AttributeHandleSet& attributes, const char* tag)
        throw (RTI::ObjectNotKnown,
               RTI::AttributeNotKnown,
               RTI::AttributeNotOwned,
               RTI::FederateInternalError)
    { _queueCallbackList.push_back(new RequestAttributeOwnershipReleaseCallback(objectHandle, attributes, tagToData(tag))); }
    void requestAttributeOwnershipReleaseCallback(RTI::ObjectHandle objectHandle, std::vector<RTI::AttributeHandle>& attributes, const RTIData& tag)
    {
        ObjectInstanceMap::iterator i = _objectInstanceMap.find(objectHandle);
        if (i == _objectInstanceMap.end())
            return;
        if (!i->second.valid())
            return;
        i->second->requestAttributeOwnershipRelease(attributes, tag);
    }

    class ConfirmAttributeOwnershipAcquisitionCancellationCallback : public AttributeHandleSetCallback {
    public:
        ConfirmAttributeOwnershipAcquisitionCancellationCallback(RTI::ObjectHandle objectHandle, const RTI::AttributeHandleSet& attributes) :
            AttributeHandleSetCallback(objectHandle, attributes)
        { }
        virtual void operator()(FederateAmbassador& self)
        { self.confirmAttributeOwnershipAcquisitionCancellationCallback(_objectHandle, _attributes); }
    };
    virtual void confirmAttributeOwnershipAcquisitionCancellation(RTI::ObjectHandle objectHandle, const RTI::AttributeHandleSet& attributes)
        throw (RTI::ObjectNotKnown,
               RTI::AttributeNotKnown,
               RTI::AttributeAlreadyOwned,
               RTI::AttributeAcquisitionWasNotCanceled,
               RTI::FederateInternalError)
    { _queueCallbackList.push_back(new ConfirmAttributeOwnershipAcquisitionCancellationCallback(objectHandle, attributes)); }
    void confirmAttributeOwnershipAcquisitionCancellationCallback(RTI::ObjectHandle objectHandle, const std::vector<RTI::AttributeHandle>& attributes)
    {
        ObjectInstanceMap::iterator i = _objectInstanceMap.find(objectHandle);
        if (i == _objectInstanceMap.end())
            return;
        if (!i->second.valid())
            return;
        i->second->confirmAttributeOwnershipAcquisitionCancellation(attributes);
    }

    class InformAttributeOwnershipCallback : public QueueCallback {
    public:
        InformAttributeOwnershipCallback(RTI::ObjectHandle objectHandle, RTI::AttributeHandle attributeHandle, RTI::FederateHandle federateHandle) :
            _objectHandle(objectHandle),
            _attributeHandle(attributeHandle),
            _federateHandle(federateHandle)
        { }
        virtual void operator()(FederateAmbassador& self)
        { self.informAttributeOwnershipCallback(_objectHandle, _attributeHandle, _federateHandle); }
    private:
        RTI::ObjectHandle _objectHandle;
        RTI::AttributeHandle _attributeHandle;
        RTI::FederateHandle _federateHandle;
    };
    virtual void informAttributeOwnership(RTI::ObjectHandle objectHandle, RTI::AttributeHandle attributeHandle,
                                          RTI::FederateHandle federateHandle)
        throw (RTI::ObjectNotKnown,
               RTI::AttributeNotKnown,
               RTI::FederateInternalError)
    { _queueCallbackList.push_back(new InformAttributeOwnershipCallback(objectHandle, attributeHandle, federateHandle)); }
    void informAttributeOwnershipCallback(RTI::ObjectHandle objectHandle, RTI::AttributeHandle attributeHandle, RTI::FederateHandle federateHandle)
    {
        ObjectInstanceMap::iterator i = _objectInstanceMap.find(objectHandle);
        if (i == _objectInstanceMap.end())
            return;
        if (!i->second.valid())
            return;
        i->second->informAttributeOwnership(attributeHandle, federateHandle);
    }

    class AttributeIsNotOwnedCallback : public QueueCallback {
    public:
        AttributeIsNotOwnedCallback(RTI::ObjectHandle objectHandle, RTI::AttributeHandle attributeHandle) :
            _objectHandle(objectHandle),
            _attributeHandle(attributeHandle)
        { }
        virtual void operator()(FederateAmbassador& self)
        { self.attributeIsNotOwnedCallback(_objectHandle, _attributeHandle); }
    private:
        RTI::ObjectHandle _objectHandle;
        RTI::AttributeHandle _attributeHandle;
    };
    virtual void attributeIsNotOwned(RTI::ObjectHandle objectHandle, RTI::AttributeHandle attributeHandle)
        throw (RTI::ObjectNotKnown,
               RTI::AttributeNotKnown,
               RTI::FederateInternalError)
    { _queueCallbackList.push_back(new AttributeIsNotOwnedCallback(objectHandle, attributeHandle)); }
    void attributeIsNotOwnedCallback(RTI::ObjectHandle objectHandle, RTI::AttributeHandle attributeHandle)
    {
        ObjectInstanceMap::iterator i = _objectInstanceMap.find(objectHandle);
        if (i == _objectInstanceMap.end())
            return;
        if (!i->second.valid())
            return;
        i->second->attributeIsNotOwned(attributeHandle);
    }

    class AttributeOwnedByRTICallback : public QueueCallback {
    public:
        AttributeOwnedByRTICallback(RTI::ObjectHandle objectHandle, RTI::AttributeHandle attributeHandle) :
            _objectHandle(objectHandle),
            _attributeHandle(attributeHandle)
        { }
        virtual void operator()(FederateAmbassador& self)
        { self.attributeOwnedByRTICallback(_objectHandle, _attributeHandle); }
    private:
        RTI::ObjectHandle _objectHandle;
        RTI::AttributeHandle _attributeHandle;
    };
    virtual void attributeOwnedByRTI(RTI::ObjectHandle objectHandle, RTI::AttributeHandle attributeHandle)
        throw (RTI::ObjectNotKnown,
               RTI::AttributeNotKnown,
               RTI::FederateInternalError)
    { _queueCallbackList.push_back(new AttributeOwnedByRTICallback(objectHandle, attributeHandle)); }
    void attributeOwnedByRTICallback(RTI::ObjectHandle objectHandle, RTI::AttributeHandle attributeHandle)
    {
        ObjectInstanceMap::iterator i = _objectInstanceMap.find(objectHandle);
        if (i == _objectInstanceMap.end())
            return;
        if (!i->second.valid())
            return;
        i->second->attributeOwnedByRTI(attributeHandle);
    }

    // Time Management
    virtual void timeRegulationEnabled(const RTI::FedTime& fedTime)
        throw (RTI::InvalidFederationTime,
               RTI::EnableTimeRegulationWasNotPending,
               RTI::FederateInternalError)
    {
        _timeRegulationEnabled = true;
        SG_LOG(SG_NETWORK, SG_INFO, "RTI: timeRegulationEnabled: " << RTI13Ambassador::toTimeStamp(fedTime));
    }

    virtual void timeConstrainedEnabled(const RTI::FedTime& fedTime)
        throw (RTI::InvalidFederationTime,
               RTI::EnableTimeConstrainedWasNotPending,
               RTI::FederateInternalError)
    {
        _timeConstrainedEnabled = true;
        SG_LOG(SG_NETWORK, SG_INFO, "RTI: timeConstrainedEnabled: " << RTI13Ambassador::toTimeStamp(fedTime));
    }

    virtual void timeAdvanceGrant(const RTI::FedTime& fedTime)
        throw (RTI::InvalidFederationTime,
               RTI::TimeAdvanceWasNotInProgress,
               RTI::FederateInternalError)
    {
        _timeAdvancePending = false;
        // SG_LOG(SG_NETWORK, SG_INFO, "RTI: timeAdvanceGrant: " << RTI13Ambassador::toTimeStamp(fedTime));
    }

    virtual void requestRetraction(RTI::EventRetractionHandle eventRetractionHandle)
        throw (RTI::EventNotKnown,
               RTI::FederateInternalError)
    {
        // No retraction concept yet
    }

    // processes the queues that filled up during the past
    void processQueues()
    {
        while (!_queueCallbackList.empty()) {
            (*_queueCallbackList.front())(*this);
            // _queueCallbackListPool.splice();
            _queueCallbackList.pop_front();
        }
    }

    bool getFederationSynchronizationPointAnnounced(const std::string& label)
    { return _pendingSyncLabels.find(label) != _pendingSyncLabels.end(); }
    bool getFederationSynchronized(const std::string& label)
    {
        std::set<std::string>::iterator i = _syncronizedSyncLabels.find(label);
        if (i == _syncronizedSyncLabels.end())
            return false;
        _syncronizedSyncLabels.erase(i);
        return true;
    }

    // The rtiambassador to issue requests
    SGSharedPtr<RTI13Ambassador> _rtiAmbassador;

    // All the sync labels we got an announcement for
    std::set<std::string> _pendingSyncLabels;
    std::set<std::string> _syncronizedSyncLabels;

    // All that calls back into user code is just queued.
    // That is to make sure we do not call recursively into the RTI
    typedef std::list<SGSharedPtr<QueueCallback> > QueueCallbackList;
    QueueCallbackList _queueCallbackList;
    // QueueCallbackList _queueCallbackListPool;

    RTI13AttributeHandleDataPairList _attributeHandleDataPairPool;
    void appendAttributeHandleDataPair(RTI13AttributeHandleDataPairList& attributeHandleDataPairList)
    {
        if (_attributeHandleDataPairPool.empty())
            attributeHandleDataPairList.push_back(RTI13AttributeHandleDataPair());
        else
            attributeHandleDataPairList.splice(attributeHandleDataPairList.end(),
                                               _attributeHandleDataPairPool, _attributeHandleDataPairPool.begin());
    }
    void freeAttributeHandleDataPairList(RTI13AttributeHandleDataPairList& attributeHandleDataPairList)
    { _attributeHandleDataPairPool.splice(_attributeHandleDataPairPool.end(), attributeHandleDataPairList); }

    // For attribute reflection, pool or indices
    HLAIndexList _indexPool;

    // Top level information for dispatching federate object attribute updates
    typedef std::map<RTI::ObjectHandle, SGSharedPtr<RTI13ObjectInstance> > ObjectInstanceMap;
    // Map of all available objects
    ObjectInstanceMap _objectInstanceMap;

    // Top level information for dispatching creation of federate objects
    typedef std::map<RTI::ObjectClassHandle, SGSharedPtr<RTI13ObjectClass> > ObjectClassMap;
    ObjectClassMap _objectClassMap;

    // Top level information for dispatching creation of federate objects
    typedef std::map<RTI::InteractionClassHandle, SGSharedPtr<RTI13InteractionClass> > InteractionClassMap;
    InteractionClassMap _interactionClassMap;

    bool _timeRegulationEnabled;
    bool _timeConstrainedEnabled;
    bool _timeAdvancePending;

private:
    const RTIData& tagToData(const char* tag)
    {
        if (tag)
            _cachedTag.setData(tag, std::strlen(tag) + 1);
        else
            _cachedTag.setData("", 1);
        return _cachedTag;
    }
    RTIData _cachedTag;
};

RTI13Federate::RTI13Federate(const std::list<std::string>& stringList) :
    _joined(false),
    _ambassador(new RTI13Ambassador),
    _federateAmbassador(new FederateAmbassador)
{
    _ambassador->_federate = this;
    _federateAmbassador->_rtiAmbassador = _ambassador;
    if (stringList.empty()) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Ignoring non empty connect arguments while connecting to an RTI13 federation!");
    }
}

RTI13Federate::~RTI13Federate()
{
    if (_joined)
        _ambassador->resignFederationExecution();
    delete _federateAmbassador;
}

RTI13Federate::FederationManagementResult
RTI13Federate::createFederationExecution(const std::string& federationName, const std::string& objectModel)
{
    try {
        _ambassador->createFederationExecution(federationName, objectModel);
        return FederationManagementSuccess;
    } catch (RTI::FederationExecutionAlreadyExists& e) {
        return FederationManagementFail;
    } catch (RTI::CouldNotOpenFED& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not create federation execution: " << e._name << " " << e._reason);
        return FederationManagementFatal;
    } catch (RTI::ErrorReadingFED& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not create federation execution: " << e._name << " " << e._reason);
        return FederationManagementFatal;
    } catch (RTI::ConcurrentAccessAttempted& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not create federation execution: " << e._name << " " << e._reason);
        return FederationManagementFatal;
    } catch (RTI::RTIinternalError& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not create federation execution: " << e._name << " " << e._reason);
        return FederationManagementFatal;
    }
}

RTI13Federate::FederationManagementResult
RTI13Federate::destroyFederationExecution(const std::string& federation)
{
    try {
        _ambassador->destroyFederationExecution(federation);
        return FederationManagementSuccess;
    } catch (RTI::FederatesCurrentlyJoined& e) {
        return FederationManagementFail;
    } catch (RTI::FederationExecutionDoesNotExist& e) {
        return FederationManagementFail;
    } catch (RTI::ConcurrentAccessAttempted& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not destroy federation execution: " << e._name << " " << e._reason);
        return FederationManagementFatal;
    } catch (RTI::RTIinternalError& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not destroy federation execution: " << e._name << " " << e._reason);
        return FederationManagementFatal;
    }
}

RTI13Federate::FederationManagementResult
RTI13Federate::join(const std::string& federateType, const std::string& federationName)
{
    try {
        _federateHandle = _ambassador->joinFederationExecution(federateType, federationName, _federateAmbassador);
        SG_LOG(SG_NETWORK, SG_INFO, "RTI: Joined federation \""
               << federationName << "\" as \"" << federateType << "\"");
        _joined = true;
        return FederationManagementSuccess;
    } catch (RTI::FederateAlreadyExecutionMember& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not join federation execution: " << e._name << " " << e._reason);
        return FederationManagementFatal;
    } catch (RTI::FederationExecutionDoesNotExist& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not join federation execution: " << e._name << " " << e._reason);
        return FederationManagementFail;
    } catch (RTI::CouldNotOpenFED& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not join federation execution: " << e._name << " " << e._reason);
        return FederationManagementFatal;
    } catch (RTI::ErrorReadingFED& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not join federation execution: " << e._name << " " << e._reason);
        return FederationManagementFatal;
    } catch (RTI::ConcurrentAccessAttempted& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not join federation execution: " << e._name << " " << e._reason);
        return FederationManagementFatal;
    } catch (RTI::SaveInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not join federation execution: " << e._name << " " << e._reason);
        return FederationManagementFatal;
    } catch (RTI::RestoreInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not join federation execution: " << e._name << " " << e._reason);
        return FederationManagementFatal;
    } catch (RTI::RTIinternalError& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not join federation execution: " << e._name << " " << e._reason);
        return FederationManagementFatal;
    }
}

bool
RTI13Federate::resign()
{
    try {
        _ambassador->resignFederationExecution();
        SG_LOG(SG_NETWORK, SG_INFO, "RTI: Resigned from federation.");
        _joined = false;
        _federateHandle = -1;
        return true;
    } catch (RTI::FederateOwnsAttributes& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not resign federation execution: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::FederateNotExecutionMember& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not resign federation execution: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::InvalidResignAction& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not resign federation execution: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::ConcurrentAccessAttempted& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not resign federation execution: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::RTIinternalError& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not resign federation execution: " << e._name << " " << e._reason);
        return false;
    }
}

bool
RTI13Federate::getJoined() const
{
    return _joined;
}

bool
RTI13Federate::registerFederationSynchronizationPoint(const std::string& label, const RTIData& tag)
{
    try {
        _ambassador->registerFederationSynchronizationPoint(label, tag);
        SG_LOG(SG_NETWORK, SG_INFO, "RTI: registerFederationSynchronizationPoint(" << label << ", tag )");
        return true;
    } catch (RTI::FederateNotExecutionMember& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not register federation synchronization point: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::ConcurrentAccessAttempted& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not register federation synchronization point: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::SaveInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not register federation synchronization point: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::RestoreInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not register federation synchronization point: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::RTIinternalError& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not register federation synchronization point: " << e._name << " " << e._reason);
        return false;
    }
}

bool
RTI13Federate::getFederationSynchronizationPointAnnounced(const std::string& label)
{
    return _federateAmbassador->getFederationSynchronizationPointAnnounced(label);
}

bool
RTI13Federate::synchronizationPointAchieved(const std::string& label)
{
    try {
        _ambassador->synchronizationPointAchieved(label);
        SG_LOG(SG_NETWORK, SG_INFO, "RTI: synchronizationPointAchieved(" << label << ")");
        return true;
    } catch (RTI::SynchronizationPointLabelWasNotAnnounced& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not signal synchronization point: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::FederateNotExecutionMember& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not signal synchronization point: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::ConcurrentAccessAttempted& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not signal synchronization point: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::SaveInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not signal synchronization point: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::RestoreInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not signal synchronization point: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::RTIinternalError& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not signal synchronization point: " << e._name << " " << e._reason);
        return false;
    }
}

bool
RTI13Federate::getFederationSynchronized(const std::string& label)
{
    return _federateAmbassador->getFederationSynchronized(label);
}

bool
RTI13Federate::enableTimeConstrained()
{
    if (!_ambassador.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not enable time constrained at unconnected federate.");
        return false;
    }

    if (_federateAmbassador->_timeConstrainedEnabled) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Time constrained is already enabled.");
        return false;
    }

    try {
        _ambassador->enableTimeConstrained();
    } catch (RTI::TimeConstrainedAlreadyEnabled& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not enable time constrained: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::EnableTimeConstrainedPending& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not enable time constrained: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::TimeAdvanceAlreadyInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not enable time constrained: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::ConcurrentAccessAttempted& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not enable time constrained: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::FederateNotExecutionMember& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not enable time constrained: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::SaveInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not enable time constrained: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::RestoreInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not enable time constrained: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::RTIinternalError& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not enable time constrained: " << e._name << " " << e._reason);
        return false;
    }

    return true;
}

bool
RTI13Federate::disableTimeConstrained()
{
    if (!_ambassador.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not disable time constrained at unconnected federate.");
        return false;
    }

    if (!_federateAmbassador->_timeConstrainedEnabled) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Time constrained is not enabled.");
        return false;
    }

    try {
        _ambassador->disableTimeConstrained();
        _federateAmbassador->_timeConstrainedEnabled = false;
    } catch (RTI::TimeConstrainedWasNotEnabled& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not disable time constrained: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::FederateNotExecutionMember& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not disable time constrained: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::ConcurrentAccessAttempted& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not disable time constrained: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::SaveInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not disable time constrained: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::RestoreInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not disable time constrained: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::RTIinternalError& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not disable time constrained: " << e._name << " " << e._reason);
        return false;
    }

    return true;
}

bool
RTI13Federate::getTimeConstrainedEnabled()
{
    return _federateAmbassador->_timeConstrainedEnabled;
}

bool
RTI13Federate::enableTimeRegulation(const SGTimeStamp& lookahead)
{
    if (!_ambassador.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not enable time regulation at unconnected federate.");
        return false;
    }

    if (_federateAmbassador->_timeRegulationEnabled) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Time regulation already enabled.");
        return false;
    }

    try {
        _ambassador->enableTimeRegulation(lookahead);
    } catch (RTI::TimeRegulationAlreadyEnabled& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not enable time regulation: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::EnableTimeRegulationPending& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not enable time regulation: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::TimeAdvanceAlreadyInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not enable time regulation: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::InvalidFederationTime& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not enable time regulation: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::InvalidLookahead& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not enable time regulation: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::ConcurrentAccessAttempted& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not enable time regulation: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::FederateNotExecutionMember& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not enable time regulation: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::SaveInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not enable time regulation: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::RestoreInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not enable time regulation: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::RTIinternalError& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not enable time regulation: " << e._name << " " << e._reason);
        return false;
    }

    return true;
}

bool
RTI13Federate::disableTimeRegulation()
{
    if (!_ambassador.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not disable time regulation at unconnected federate.");
        return false;
    }

    if (!_federateAmbassador->_timeRegulationEnabled) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Time regulation is not enabled.");
        return false;
    }

    try {
        _ambassador->disableTimeRegulation();
        _federateAmbassador->_timeRegulationEnabled = false;
    } catch (RTI::TimeRegulationWasNotEnabled& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not disable time regulation: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::ConcurrentAccessAttempted& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not disable time regulation: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::FederateNotExecutionMember& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not disable time regulation: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::SaveInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not disable time regulation: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::RestoreInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not disable time regulation: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::RTIinternalError& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not disable time regulation: " << e._name << " " << e._reason);
        return false;
    }

    return true;
}

bool
RTI13Federate::modifyLookahead(const SGTimeStamp& timeStamp)
{
    if (!_ambassador.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not modify lookahead.");
        return false;
    }
    try {
        _ambassador->modifyLookahead(timeStamp);
    } catch (RTI::InvalidLookahead& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not modify lookahead: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::FederateNotExecutionMember& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not modify lookahead: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::ConcurrentAccessAttempted& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not modify lookahead: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::SaveInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not modify lookahead: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::RestoreInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not modify lookahead: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::RTIinternalError& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not modify lookahead: " << e._name << " " << e._reason);
        return false;
    }
    return true;
}

bool
RTI13Federate::getTimeRegulationEnabled()
{
    return _federateAmbassador->_timeRegulationEnabled;
}

bool
RTI13Federate::timeAdvanceRequest(const SGTimeStamp& timeStamp)
{
    if (!_ambassador.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not advance time at unconnected federate.");
        return false;
    }

    try {
        _ambassador->timeAdvanceRequest(timeStamp);
        _federateAmbassador->_timeAdvancePending = true;
    } catch (RTI::InvalidFederationTime& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not advance time: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::FederationTimeAlreadyPassed& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not advance time: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::TimeAdvanceAlreadyInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not advance time: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::EnableTimeRegulationPending& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not advance time: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::EnableTimeConstrainedPending& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not advance time: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::FederateNotExecutionMember& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not advance time: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::ConcurrentAccessAttempted& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not advance time: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::SaveInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not advance time: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::RestoreInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not advance time: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::RTIinternalError& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not advance time: " << e._name << " " << e._reason);
        return false;
    }

    return true;
}

bool
RTI13Federate::timeAdvanceRequestAvailable(const SGTimeStamp& timeStamp)
{
    if (!_ambassador.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not advance time at unconnected federate.");
        return false;
    }

    try {
        _ambassador->timeAdvanceRequestAvailable(timeStamp);
        _federateAmbassador->_timeAdvancePending = true;
    } catch (RTI::InvalidFederationTime& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not advance time: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::FederationTimeAlreadyPassed& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not advance time: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::TimeAdvanceAlreadyInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not advance time: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::EnableTimeRegulationPending& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not advance time: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::EnableTimeConstrainedPending& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not advance time: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::FederateNotExecutionMember& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not advance time: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::ConcurrentAccessAttempted& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not advance time: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::SaveInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not advance time: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::RestoreInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not advance time: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::RTIinternalError& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not advance time: " << e._name << " " << e._reason);
        return false;
    }

    return true;
}

bool
RTI13Federate::flushQueueRequest(const SGTimeStamp& timeStamp)
{
    if (!_ambassador.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not flush queue at unconnected federate.");
        return false;
    }

    try {
        _ambassador->flushQueueRequest(timeStamp);
        _federateAmbassador->_timeAdvancePending = true;
    } catch (RTI::InvalidFederationTime& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not flush queue: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::FederationTimeAlreadyPassed& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not flush queue: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::TimeAdvanceAlreadyInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not flush queue: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::EnableTimeRegulationPending& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not flush queue: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::EnableTimeConstrainedPending& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not flush queue: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::FederateNotExecutionMember& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not flush queue: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::ConcurrentAccessAttempted& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not flush queue: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::SaveInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not flush queue: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::RestoreInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not flush queue: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::RTIinternalError& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not flush queue: " << e._name << " " << e._reason);
        return false;
    }

    return true;
}

bool
RTI13Federate::getTimeAdvancePending()
{
    return _federateAmbassador->_timeAdvancePending;
}

bool
RTI13Federate::queryFederateTime(SGTimeStamp& timeStamp)
{
    if (!_ambassador.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not query federate time.");
        return false;
    }

    try {
        _ambassador->queryFederateTime(timeStamp);
    } catch (RTI::FederateNotExecutionMember& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not query federate time: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::ConcurrentAccessAttempted& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not query federate time: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::SaveInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not query federate time: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::RestoreInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not query federate time: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::RTIinternalError& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not query federate time: " << e._name << " " << e._reason);
        return false;
    }
    return true;
}

bool
RTI13Federate::queryLookahead(SGTimeStamp& timeStamp)
{
    if (!_ambassador.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not query lookahead.");
        return false;
    }

    try {
        _ambassador->queryLookahead(timeStamp);
    } catch (RTI::FederateNotExecutionMember& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not query lookahead: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::ConcurrentAccessAttempted& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not query lookahead: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::SaveInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not query lookahead: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::RestoreInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not query lookahead: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::RTIinternalError& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not query lookahead: " << e._name << " " << e._reason);
        return false;
    }
    return true;
}

bool
RTI13Federate::queryGALT(SGTimeStamp& timeStamp)
{
    if (!_ambassador.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not query GALT.");
        return false;
    }

    try {
        return _ambassador->queryGALT(timeStamp);
    } catch (RTI::FederateNotExecutionMember& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not query GALT: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::ConcurrentAccessAttempted& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not query GALT: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::SaveInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not query GALT: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::RestoreInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not query GALT: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::RTIinternalError& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not query GALT: " << e._name << " " << e._reason);
        return false;
    }
    return true;
}

bool
RTI13Federate::queryLITS(SGTimeStamp& timeStamp)
{
    if (!_ambassador.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not query LITS.");
        return false;
    }

    try {
        return _ambassador->queryLITS(timeStamp);
    } catch (RTI::FederateNotExecutionMember& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not query LITS: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::ConcurrentAccessAttempted& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not query LITS: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::SaveInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not query LITS: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::RestoreInProgress& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not query LITS: " << e._name << " " << e._reason);
        return false;
    } catch (RTI::RTIinternalError& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not query LITS: " << e._name << " " << e._reason);
        return false;
    }
    return true;
}

RTI13Federate::ProcessMessageResult
RTI13Federate::processMessage()
{
    ProcessMessageResult result = _tick();
    _federateAmbassador->processQueues();
    return result;
}

RTI13Federate::ProcessMessageResult
RTI13Federate::processMessages(const double& minimum, const double& maximum)
{
    ProcessMessageResult result = _tick(minimum, 0);
    _federateAmbassador->processQueues();
    if (result != ProcessMessagePending)
        return result;
    SGTimeStamp timeStamp = SGTimeStamp::now() + SGTimeStamp::fromSec(maximum);
    do {
        result = _tick(0, 0);
        _federateAmbassador->processQueues();
    } while (result == ProcessMessagePending && SGTimeStamp::now() <= timeStamp);
    return result;
}

RTI13Federate::ProcessMessageResult
RTI13Federate::_tick()
{
    if (!_ambassador.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Ambassador is zero while calling _tick().");
        return ProcessMessageFatal;
    }

    try {
        if (_ambassador->tick())
            return ProcessMessagePending;
        return ProcessMessageLast;
    } catch (RTI::SpecifiedSaveLabelDoesNotExist& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Specified save label does not exist: " << e._name << " " << e._reason);
        return ProcessMessageFatal;
    } catch (RTI::ConcurrentAccessAttempted& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Concurrent access attempted: " << e._name << " " << e._reason);
        return ProcessMessageFatal;
    } catch (RTI::RTIinternalError& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Internal error: " << e._name << " " << e._reason);
        return ProcessMessageFatal;
    }
    return ProcessMessageFatal;
}

RTI13Federate::ProcessMessageResult
RTI13Federate::_tick(const double& minimum, const double& maximum)
{
    if (!_ambassador.valid()) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Ambassador is zero while calling _tick().");
        return ProcessMessageFatal;
    }

    try {
        if (_ambassador->tick(minimum, maximum))
            return ProcessMessagePending;
        return ProcessMessageLast;
    } catch (RTI::SpecifiedSaveLabelDoesNotExist& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Specified save label does not exist: " << e._name << " " << e._reason);
        return ProcessMessageFatal;
    } catch (RTI::ConcurrentAccessAttempted& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Concurrent access attempted: " << e._name << " " << e._reason);
        return ProcessMessageFatal;
    } catch (RTI::RTIinternalError& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Internal error: " << e._name << " " << e._reason);
        return ProcessMessageFatal;
    }
    return ProcessMessageFatal;
}


RTI13ObjectClass*
RTI13Federate::createObjectClass(const std::string& objectClassName, HLAObjectClass* hlaObjectClass)
{
    try {
        RTI::ObjectClassHandle objectClassHandle;
        objectClassHandle = _ambassador->getObjectClassHandle(objectClassName);
        if (_federateAmbassador->_objectClassMap.find(objectClassHandle) != _federateAmbassador->_objectClassMap.end()) {
            SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not create object class, object class already exists!");
            return 0;
        }
        RTI13ObjectClass* rtiObjectClass;
        rtiObjectClass = new RTI13ObjectClass(hlaObjectClass, objectClassHandle, _ambassador.get());
        _federateAmbassador->_objectClassMap[objectClassHandle] = rtiObjectClass;
        return rtiObjectClass;
    } catch (RTI::NameNotFound& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not get object class: " << e._name << " " << e._reason);
        return 0;
    } catch (RTI::FederateNotExecutionMember& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not get object class: " << e._name << " " << e._reason);
        return 0;
    } catch (RTI::ConcurrentAccessAttempted& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not get object class: " << e._name << " " << e._reason);
        return 0;
    } catch (RTI::RTIinternalError& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not get object class: " << e._name << " " << e._reason);
        return 0;
    }
}

RTI13InteractionClass*
RTI13Federate::createInteractionClass(const std::string& interactionClassName, HLAInteractionClass* interactionClass)
{
    try {
        RTI::InteractionClassHandle interactionClassHandle;
        interactionClassHandle = _ambassador->getInteractionClassHandle(interactionClassName);
        if (_federateAmbassador->_interactionClassMap.find(interactionClassHandle) != _federateAmbassador->_interactionClassMap.end()) {
            SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not create interaction class, interaction class already exists!");
            return 0;
        }
        RTI13InteractionClass* rtiInteractionClass;
        rtiInteractionClass = new RTI13InteractionClass(interactionClass, interactionClassHandle, _ambassador.get());
        _federateAmbassador->_interactionClassMap[interactionClassHandle] = rtiInteractionClass;
        return rtiInteractionClass;
    } catch (RTI::NameNotFound& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not get interaction class: " << e._name << " " << e._reason);
        return 0;
    } catch (RTI::FederateNotExecutionMember& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not get interaction class: " << e._name << " " << e._reason);
        return 0;
    } catch (RTI::ConcurrentAccessAttempted& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not get interaction class: " << e._name << " " << e._reason);
        return 0;
    } catch (RTI::RTIinternalError& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not get interaction class: " << e._name << " " << e._reason);
        return 0;
    }
}

RTI13ObjectInstance*
RTI13Federate::getObjectInstance(const std::string& objectInstanceName)
{
    try {
        RTI::ObjectHandle objectHandle;
        objectHandle = _ambassador->getObjectInstanceHandle(objectInstanceName);
        FederateAmbassador::ObjectInstanceMap::iterator i = _federateAmbassador->_objectInstanceMap.find(objectHandle);
        if (i == _federateAmbassador->_objectInstanceMap.end()) {
            SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not get object instance: ObjectInstance not found.");
            return 0;
        }
        return i->second;
    } catch (RTI::ObjectNotKnown& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not get object instance: " << e._name << " " << e._reason);
        return 0;
    } catch (RTI::FederateNotExecutionMember& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not get object instance: " << e._name << " " << e._reason);
        return 0;
    } catch (RTI::ConcurrentAccessAttempted& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not get object instance: " << e._name << " " << e._reason);
        return 0;
    } catch (RTI::RTIinternalError& e) {
        SG_LOG(SG_NETWORK, SG_WARN, "RTI: Could not get object instance: " << e._name << " " << e._reason);
        return 0;
    }
}

void
RTI13Federate::insertObjectInstance(RTI13ObjectInstance* objectInstance)
{
    _federateAmbassador->_objectInstanceMap[objectInstance->getHandle()] = objectInstance;
}

}
