include (SimGearComponent)

set(HEADERS
    sgstream.hxx
    gzfstream.hxx
    gzcontainerfile.hxx
    CharArrayStream.hxx
    zlibstream.hxx
    )

set(SOURCES
    sgstream.cxx
    gzfstream.cxx
    gzcontainerfile.cxx
    CharArrayStream.cxx
    zlibstream.cxx
    )

simgear_component(IOStreams io/iostreams "${SOURCES}" "${HEADERS}")

if(ENABLE_TESTS)

  add_executable(test_streams sgstream_test.cxx )
  target_link_libraries(test_streams ${TEST_LIBS})
  add_test(streams ${EXECUTABLE_OUTPUT_PATH}/test_streams)

  add_executable(test_CharArrayStream CharArrayStream_test.cxx)
  target_link_libraries(test_CharArrayStream ${TEST_LIBS})
  add_test(CharArrayStream ${EXECUTABLE_OUTPUT_PATH}/test_CharArrayStream)

  add_executable(test_zlibstream zlibstream_test.cxx)
  target_link_libraries(test_zlibstream ${TEST_LIBS})
  add_test(zlibstream ${EXECUTABLE_OUTPUT_PATH}/test_zlibstream)

endif(ENABLE_TESTS)
