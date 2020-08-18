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
  add_simgear_autotest(test_streams sgstream_test.cxx )
  add_simgear_autotest(test_CharArrayStream CharArrayStream_test.cxx)
  add_simgear_autotest(test_zlibstream zlibstream_test.cxx)
endif(ENABLE_TESTS)
