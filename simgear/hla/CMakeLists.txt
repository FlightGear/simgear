include (SimGearComponent)

set(HLA_HEADERS
    RTIData.hxx
    HLAArrayDataElement.hxx
    HLAArrayDataType.hxx
    HLABasicDataElement.hxx
    HLABasicDataType.hxx
    HLADataElement.hxx
    HLADataElementVisitor.hxx
    HLADataType.hxx
    HLADataTypeVisitor.hxx
    HLAEnumeratedDataElement.hxx
    HLAEnumeratedDataType.hxx
    HLAFixedRecordDataElement.hxx
    HLAFixedRecordDataType.hxx
    HLAFederate.hxx
    HLAInteractionClass.hxx
    HLALocation.hxx
    HLAObjectClass.hxx
    HLAObjectInstance.hxx
    HLAOMTXmlVisitor.hxx
    HLAPropertyDataElement.hxx
    HLARawDataElement.hxx
    HLATypes.hxx
    HLAVariantRecordDataElement.hxx
    HLAVariantRecordDataType.hxx
    )

set(HLA_SOURCES
    HLAArrayDataElement.cxx
    HLAArrayDataType.cxx
    HLABasicDataElement.cxx
    HLABasicDataType.cxx
    HLADataElement.cxx
    HLADataType.cxx
    HLADataTypeVisitor.cxx
    HLAEnumeratedDataElement.cxx
    HLAEnumeratedDataType.cxx
    HLAFederate.cxx
    HLAInteractionClass.cxx
    HLAFixedRecordDataElement.cxx
    HLAFixedRecordDataType.cxx
    HLALocation.cxx
    HLAObjectClass.cxx
    HLAObjectInstance.cxx
    HLAOMTXmlVisitor.cxx
    HLAPropertyDataElement.cxx
    HLARawDataElement.cxx
    HLAVariantRecordDataElement.cxx
    HLAVariantRecordDataType.cxx
    )
simgear_component(hla hla "${HLA_SOURCES}" "${HLA_HEADERS}")

if(RTI_FOUND)
  set(RTI13_SOURCES
    RTI13InteractionClass.cxx
    RTI13ObjectClass.cxx
    RTI13ObjectInstance.cxx
    RTI13Federate.cxx
    RTI13FederateFactory.cxx
    )
  simgear_component(rti13 hla "${RTI13_SOURCES}" "")
endif()

set(RTI_SOURCES
  RTIInteractionClass.cxx
  RTIObjectClass.cxx
  RTIObjectInstance.cxx
  RTIFederate.cxx
  RTIFederateFactory.cxx
  RTIFederateFactoryRegistry.cxx
  )
simgear_component(rti hla "${RTI_SOURCES}" "")
