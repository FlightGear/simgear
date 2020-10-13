add_subdirectory(iostreams)

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
    HTTPFileRequest.hxx
    HTTPMemoryRequest.hxx
    HTTPRequest.hxx
    HTTPRepository.hxx
    untar.hxx
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
    HTTPTestApi_private.hxx
    HTTPFileRequest.cxx
    HTTPMemoryRequest.cxx
    HTTPRequest.cxx
    HTTPRepository.cxx
    HTTPRepository_private.hxx
    untar.cxx
    )

if(ENABLE_DNS)
  list(APPEND SOURCES DNSClient.cxx)
  list(APPEND HEADERS DNSClient.hxx)
endif()

simgear_component(io io "${SOURCES}" "${HEADERS}")

if(ENABLE_TESTS)

add_executable(test_sock socktest.cxx)
target_link_libraries(test_sock ${TEST_LIBS})

add_executable(test_http test_HTTP.cxx)
target_link_libraries(test_http ${TEST_LIBS})

add_test(http ${EXECUTABLE_OUTPUT_PATH}/test_http)

if(ENABLE_DNS)
	add_executable(test_dns test_DNS.cxx)
	target_link_libraries(test_dns ${TEST_LIBS})
	add_test(dns ${EXECUTABLE_OUTPUT_PATH}/test_dns)
endif()

add_executable(httpget httpget.cxx)
target_link_libraries(httpget ${TEST_LIBS})

add_executable(http_repo_sync http_repo_sync.cxx)
target_link_libraries(http_repo_sync ${TEST_LIBS})

add_executable(decode_binobj decode_binobj.cxx)
target_link_libraries(decode_binobj ${TEST_LIBS})

add_executable(test_binobj test_binobj.cxx)
target_link_libraries(test_binobj ${TEST_LIBS})

add_test(binobj ${EXECUTABLE_OUTPUT_PATH}/test_binobj)

add_executable(test_repository test_repository.cxx)
target_link_libraries(test_repository ${TEST_LIBS})
target_compile_definitions(test_repository PUBLIC BUILDING_TESTSUITE)
add_test(http_repository ${EXECUTABLE_OUTPUT_PATH}/test_repository)

add_executable(test_untar test_untar.cxx)

set_target_properties(test_untar PROPERTIES
  COMPILE_DEFINITIONS "SRC_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}\"" )

target_link_libraries(test_untar ${TEST_LIBS})
add_test(untar ${EXECUTABLE_OUTPUT_PATH}/test_untar)

endif(ENABLE_TESTS)
