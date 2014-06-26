include (SimGearComponent)

set(HEADERS
  Ghost.hxx
  NasalCallContext.hxx
  NasalHash.hxx
  NasalObject.hxx
  NasalObjectHolder.hxx
  NasalString.hxx
  from_nasal.hxx
  to_nasal.hxx
)

set(DETAIL_HEADERS
  detail/from_nasal_function_templates.hxx
  detail/from_nasal_helper.hxx
  detail/functor_templates.hxx
  detail/nasal_traits.hxx
  detail/to_nasal_helper.hxx
)

set(SOURCES
  Ghost.cxx
  NasalHash.cxx
  NasalString.cxx
  NasalObject.cxx
  detail/from_nasal_helper.cxx
  detail/to_nasal_helper.cxx
)

simgear_component(nasal/cppbind nasal/cppbind "${SOURCES}" "${HEADERS}")
simgear_component(nasal/cppbind/detail nasal/cppbind/detail "" "${DETAIL_HEADERS}")

if(ENABLE_TESTS)
  add_executable(test_cppbind cppbind_test.cxx)
  add_test(cppbind ${EXECUTABLE_OUTPUT_PATH}/test_cppbind)
  target_link_libraries(test_cppbind ${TEST_LIBS})
endif(ENABLE_TESTS)

add_boost_test(cppbind_ghost
  SOURCES cppbind_test_ghost.cxx
  LIBRARIES ${TEST_LIBS}
)

add_boost_test(nasal_num
  SOURCES nasal_num_test.cxx
  LIBRARIES ${TEST_LIBS}
)