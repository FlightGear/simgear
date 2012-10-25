

include (SimGearComponent)

set(HEADERS
    iochannel.hxx
    lowlevel.hxx
    raw_socket.hxx
    sg_binobj.hxx
    sg_file.hxx
    sg_netBuffer.hxx
    sg_netChannel.hxx
    sg_netChat.hxx
    sg_serial.hxx
    sg_socket.hxx
    sg_socket_udp.hxx
    HTTPClient.hxx
    HTTPRequest.hxx
    DAVMultiStatus.hxx
    SVNRepository.hxx
    SVNDirectory.hxx
    SVNReportParser.hxx
    )

set(SOURCES
    iochannel.cxx
    lowlevel.cxx
    raw_socket.cxx
    sg_binobj.cxx
    sg_file.cxx
    sg_netBuffer.cxx
    sg_netChannel.cxx
    sg_netChat.cxx
    sg_serial.cxx
    sg_socket.cxx
    sg_socket_udp.cxx
    HTTPClient.cxx
    HTTPRequest.cxx
    DAVMultiStatus.cxx
    SVNRepository.cxx
    SVNDirectory.cxx
    SVNReportParser.cxx
    )

simgear_component(io io "${SOURCES}" "${HEADERS}")

if(ENABLE_TESTS)
    
if (SIMGEAR_SHARED)
    set(TEST_LIBS SimGearCore)
else()
    set(TEST_LIBS SimGearCore
            ${CMAKE_THREAD_LIBS_INIT}
            ${WINSOCK_LIBRARY}
            ${ZLIB_LIBRARY}
            ${RT_LIBRARY})
endif()

add_executable(http_svn http_svn.cxx)
target_link_libraries(http_svn ${TEST_LIBS})

add_executable(test_sock socktest.cxx)
target_link_libraries(test_sock ${TEST_LIBS})

add_executable(test_http test_HTTP.cxx)
target_link_libraries(test_http ${TEST_LIBS})

add_test(http ${EXECUTABLE_OUTPUT_PATH}/test_http)

add_executable(httpget httpget.cxx)
target_link_libraries(httpget ${TEST_LIBS})

add_executable(decode_binobj decode_binobj.cxx)
target_link_libraries(decode_binobj ${TEST_LIBS})

add_executable(test_binobj test_binobj.cxx)
target_link_libraries(test_binobj ${TEST_LIBS}) 
    
add_test(binobj ${EXECUTABLE_OUTPUT_PATH}/test_binobj)

endif(ENABLE_TESTS)
