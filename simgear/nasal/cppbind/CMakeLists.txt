include (SimGearComponent)

set(HEADERS
  Ghost.hxx
  NasalHash.hxx
  from_nasal_detail.hxx
  from_nasal.hxx
  to_nasal.hxx
)

set(SOURCES
  NasalHash.cxx
  from_nasal.cxx
  to_nasal.cxx
)

simgear_component(nasal/cppbind nasal/cppbind "${SOURCES}" "${HEADERS}")

if(ENABLE_TESTS)
  add_executable(test_cppbind cppbind_test.cxx)
  add_test(test_cppbind ${EXECUTABLE_OUTPUT_PATH}/test_cppbind)
  target_link_libraries(test_cppbind SimGearCore)
endif(ENABLE_TESTS)