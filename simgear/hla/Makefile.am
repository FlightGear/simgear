INCLUDES = -I$(top_srcdir)

lib_LIBRARIES = libsghla.a

libsghla_adir = @includedir@/hla

libsghla_a_HEADERS = \
	RTIData.hxx \
	HLAArrayDataElement.hxx \
	HLAArrayDataType.hxx \
	HLABasicDataElement.hxx \
	HLABasicDataType.hxx \
	HLADataElement.hxx \
	HLADataType.hxx \
	HLADataTypeVisitor.hxx \
	HLAEnumeratedDataElement.hxx \
	HLAEnumeratedDataType.hxx \
	HLAFixedRecordDataElement.hxx \
	HLAFixedRecordDataType.hxx \
	HLAFederate.hxx \
	HLAInteractionClass.hxx \
	HLALocation.hxx \
	HLAObjectClass.hxx \
	HLAObjectInstance.hxx \
	HLAOMTXmlVisitor.hxx \
	HLAPropertyDataElement.hxx \
	HLARawDataElement.hxx \
	HLAVariantDataElement.hxx \
	HLAVariantDataType.hxx

libsghla_a_SOURCES = \
	RTIObjectClass.cxx \
	RTIObjectInstance.cxx \
	RTIFederate.cxx \
	HLAArrayDataElement.cxx \
	HLAArrayDataType.cxx \
	HLABasicDataElement.cxx \
	HLABasicDataType.cxx \
	HLADataElement.cxx \
	HLADataType.cxx \
	HLAEnumeratedDataElement.cxx \
	HLAEnumeratedDataType.cxx \
	HLAFederate.cxx \
	HLAFixedRecordDataElement.cxx \
	HLAFixedRecordDataType.cxx \
	HLAObjectClass.cxx \
	HLAObjectInstance.cxx \
	HLAOMTXmlVisitor.cxx \
	HLAPropertyDataElement.cxx \
	HLARawDataElement.cxx \
	HLAVariantDataElement.cxx \
	HLAVariantDataType.cxx

if ENABLE_HLA13

lib_LIBRARIES += libsghla13.a

libsghla13_adir = @includedir@/hla

libsghla13_a_HEADERS = \
	HLA13Federate.hxx

libsghla13_a_SOURCES = \
	RTI13ObjectClass.cxx \
	RTI13ObjectInstance.cxx \
	RTI13Federate.cxx \
	HLA13Federate.cxx

endif
