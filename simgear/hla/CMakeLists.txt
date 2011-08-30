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
  set(RTI13_SOURCES
    RTI13ObjectClass.cxx
    RTI13ObjectInstance.cxx
    RTI13Federate.cxx
    )
  simgear_component(rti13 hla "${RTI13_SOURCES}" "")
  set_property(TARGET sgrti13 APPEND PROPERTY COMPILE_FLAGS "-I${RTI_INCLUDE_DIR}")
endif()

set(RTI_SOURCES
  RTIObjectClass.cxx
  RTIObjectInstance.cxx
  RTIFederate.cxx
  )
simgear_component(rti hla "${RTI_SOURCES}" "")
