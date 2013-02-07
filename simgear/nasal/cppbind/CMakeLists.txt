include (SimGearComponent)

set(HEADERS
  Ghost.hxx
  NasalHash.hxx
  NasalString.hxx
  from_nasal_detail.hxx
  from_nasal.hxx
  nasal_traits.hxx
  to_nasal.hxx
)

set(SOURCES
  NasalHash.cxx
  NasalString.cxx
  from_nasal.cxx
  to_nasal.cxx
)

simgear_component(nasal/cppbind nasal/cppbind "${SOURCES}" "${HEADERS}")

if(ENABLE_TESTS)

if (SIMGEAR_SHARED)
    set(TEST_LIBS SimGearCore)
else()
    set(TEST_LIBS SimGearCore ${CMAKE_THREAD_LIBS_INIT})
endif()

  add_executable(test_cppbind cppbind_test.cxx)
  add_test(test_cppbind ${EXECUTABLE_OUTPUT_PATH}/test_cppbind)
  target_link_libraries(test_cppbind ${TEST_LIBS})
endif(ENABLE_TESTS)