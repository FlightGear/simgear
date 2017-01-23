include (SimGearComponent)

set(HEADERS
    sgstream.hxx
    gzfstream.hxx
    gzcontainerfile.hxx
    )

set(SOURCES
    sgstream.cxx
    gzfstream.cxx
    gzcontainerfile.cxx
    )

simgear_component(IOStreams io/iostreams "${SOURCES}" "${HEADERS}")

if(ENABLE_TESTS)

  add_executable(test_streams sgstream_test.cxx )
  target_link_libraries(test_streams ${TEST_LIBS})
  add_test(streams ${EXECUTABLE_OUTPUT_PATH}/test_streams)

endif(ENABLE_TESTS)
