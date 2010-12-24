include (SimGearComponent)

set(HLA_HEADERS
    RTIData.hxx
    HLAArrayDataElement.hxx
    HLAArrayDataType.hxx
    HLABasicDataElement.hxx
    HLABasicDataType.hxx
    HLADataElement.hxx
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
    HLAVariantDataElement.hxx
    HLAVariantDataType.hxx
    )

set(HLA_SOURCES
    RTIObjectClass.cxx
    RTIObjectInstance.cxx
    RTIFederate.cxx
    HLAArrayDataElement.cxx
    HLAArrayDataType.cxx
    HLABasicDataElement.cxx
    HLABasicDataType.cxx
    HLADataElement.cxx
    HLADataType.cxx
    HLAEnumeratedDataElement.cxx
    HLAEnumeratedDataType.cxx
    HLAFederate.cxx
    HLAFixedRecordDataElement.cxx
    HLAFixedRecordDataType.cxx
    HLAObjectClass.cxx
    HLAObjectInstance.cxx
    HLAOMTXmlVisitor.cxx
    HLAPropertyDataElement.cxx
    HLARawDataElement.cxx
    HLAVariantDataElement.cxx
    HLAVariantDataType.cxx
    )
simgear_component(hla hla "${HLA_SOURCES}" "${HLA_HEADERS}")

if(RTI_FOUND)
  set(HLA13_HEADERS
    HLA13Federate.hxx
    )
  set(HLA13_SOURCES
    RTI13ObjectClass.cxx
    RTI13ObjectInstance.cxx
    RTI13Federate.cxx
    HLA13Federate.cxx
    )
  simgear_component(hla13 hla "${HLA13_SOURCES}" "${HLA13_HEADERS}")
  set_property(TARGET sghla13 APPEND PROPERTY COMPILE_FLAGS "-I${RTI_INCLUDE_DIR}")
endif()
