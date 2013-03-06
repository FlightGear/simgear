include (SimGearComponent)

set(HEADERS
  Ghost.hxx
  NasalCallContext.hxx
  NasalHash.hxx
  NasalString.hxx
  from_nasal.hxx
  to_nasal.hxx
)

set(DETAIL_HEADERS
  detail/from_nasal_helper.hxx
  detail/functor_templates.hxx
  detail/nasal_traits.hxx
  detail/to_nasal_helper.hxx
)

set(SOURCES
  NasalHash.cxx
  NasalString.cxx
  detail/from_nasal_helper.cxx
  detail/to_nasal_helper.cxx
)

simgear_component(nasal/cppbind nasal/cppbind "${SOURCES}" "${HEADERS}")
simgear_component(nasal/cppbind/detail nasal/cppbind/detail "" "${DETAIL_HEADERS}")

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