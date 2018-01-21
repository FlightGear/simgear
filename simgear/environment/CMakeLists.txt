
include (SimGearComponent)

set(HEADERS metar.hxx precipitation.hxx)
set(SOURCES metar.cxx precipitation.cxx)

simgear_scene_component(environment environment "${SOURCES}" "${HEADERS}")

if(ENABLE_TESTS)
add_executable(test_metar test_metar.cxx)

if (SIMGEAR_SHARED)
    target_link_libraries(test_metar SimGearScene ${GDAL_LIBRARY}) 
else()
    target_link_libraries(test_metar
        SimGearScene SimGearCore
        ${CMAKE_THREAD_LIBS_INIT}
        ${ZLIB_LIBRARY}
        ${RT_LIBRARY}
        ${GDAL_LIBRARY})
endif()

add_test(metar ${EXECUTABLE_OUTPUT_PATH}/test_metar)

add_executable(test_precipitation test_precipitation.cxx)
target_link_libraries(test_precipitation ${TEST_LIBS})
add_test(precipitation ${EXECUTABLE_OUTPUT_PATH}/test_precipitation)

endif(ENABLE_TESTS)
