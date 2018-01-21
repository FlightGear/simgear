include (SimGearComponent)

set(HEADERS
  integer_sequence.hxx
  type_traits.hxx
)

set(SOURCES
)

simgear_component(std std "${SOURCES}" "${HEADERS}")

if(ENABLE_TESTS)
  add_executable(test_integer_sequence integer_sequence_test.cxx)
  add_test(integer_sequence ${EXECUTABLE_OUTPUT_PATH}/test_integer_sequence)
  target_link_libraries(test_integer_sequence ${TEST_LIBS})

  add_executable(test_type_traits type_traits_test.cxx)
  add_test(type_traits ${EXECUTABLE_OUTPUT_PATH}/test_type_traits)
  target_link_libraries(test_type_traits ${TEST_LIBS})
endif()
