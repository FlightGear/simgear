
include (SimGearComponent)

set(HEADERS metar.hxx precipitation.hxx)
set(SOURCES metar.cxx precipitation.cxx)

simgear_scene_component(environment environment "${SOURCES}" "${HEADERS}")

if(ENABLE_TESTS)

add_simgear_autotest(test_metar test_metar.cxx)
target_link_libraries(test_metar SimGearScene) 

add_simgear_autotest(test_precipitation test_precipitation.cxx)
target_link_libraries(test_precipitation SimGearScene)

endif(ENABLE_TESTS)
