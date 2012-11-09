include (SimGearComponent)

set(HEADERS
  NasalHash.hxx
  to_nasal.hxx
)

set(SOURCES
  NasalHash.cxx
  to_nasal.cxx
)

simgear_component(nasal/cppbind nasal/cppbind "${SOURCES}" "${HEADERS}")

if(ENABLE_TESTS)
  add_executable(test_cppbind cppbind_test.cxx)
  add_test(test_cppbind ${EXECUTABLE_OUTPUT_PATH}/test_cppbind)
  target_link_libraries(test_cppbind SimGearCore)
endif(ENABLE_TESTS)