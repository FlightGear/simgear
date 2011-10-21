
include (SimGearComponent)

set(HEADERS metar.hxx precipitation.hxx)
set(SOURCES metar.cxx precipitation.cxx)

simgear_component(environment environment "${SOURCES}" "${HEADERS}")

add_executable(test_metar test_metar.cxx)
target_link_libraries(test_metar
    sgenvironment sgstructure sgmisc sgdebug
    ${CMAKE_THREAD_LIBS_INIT}
    ${ZLIB_LIBRARY}
    ${RT_LIBRARY}) 
    
add_test(metar ${EXECUTABLE_OUTPUT_PATH}/test_metar)