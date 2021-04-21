include (SimGearComponent)

set(HEADERS
  cppbind_fwd.hxx
  Ghost.hxx
  NasalCallContext.hxx
  NasalContext.hxx
  NasalHash.hxx
  NasalMe.hxx
  NasalMethodHolder.hxx
  NasalObject.hxx
  NasalObjectHolder.hxx
  NasalString.hxx
  from_nasal.hxx
  to_nasal.hxx
)

set(DETAIL_HEADERS
  detail/from_nasal_helper.hxx
  detail/nasal_traits.hxx
  detail/to_nasal_helper.hxx
)

set(SOURCES
  Ghost.cxx
  NasalContext.cxx
  NasalHash.cxx
  NasalString.cxx
  NasalObject.cxx
  detail/from_nasal_helper.cxx
  detail/to_nasal_helper.cxx
)

simgear_component(nasal/cppbind nasal/cppbind "${SOURCES}" "${HEADERS}")
simgear_component(nasal/cppbind/detail nasal/cppbind/detail "" "${DETAIL_HEADERS}")

add_boost_test(cppbind_ghost
  SOURCES test/cppbind_test_ghost.cxx
  LIBRARIES SimGearCore
)

add_boost_test(cppbind_misc
  SOURCES test/cppbind_test.cxx
  LIBRARIES SimGearCore
)

add_boost_test(nasal_gc_test
  SOURCES test/nasal_gc_test.cxx
  LIBRARIES SimGearCore
)

add_boost_test(nasal_num
  SOURCES test/nasal_num_test.cxx
  LIBRARIES SimGearCore
)
