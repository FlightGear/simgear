include (SimGearComponent)

set(HEADERS
  integer_sequence.hxx
  type_traits.hxx
)

set(SOURCES
)

simgear_component(std std "${SOURCES}" "${HEADERS}")

if(ENABLE_TESTS)
  add_simgear_autotest(test_integer_sequence integer_sequence_test.cxx)
  add_simgear_autotest(test_type_traits type_traits_test.cxx)
endif()
