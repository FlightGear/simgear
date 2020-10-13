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
    DNSClient.hxx
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
    DNSClient.cxx
    )

simgear_component(io io "${SOURCES}" "${HEADERS}")

if(ENABLE_TESTS)

add_simgear_test(test_sock socktest.cxx)
add_simgear_autotest(test_http test_HTTP.cxx)
add_simgear_autotest(test_dns test_DNS.cxx)
add_simgear_test(httpget httpget.cxx)
add_simgear_test(http_repo_sync http_repo_sync.cxx)
add_simgear_test(decode_binobj decode_binobj.cxx)
add_simgear_autotest(test_binobj test_binobj.cxx)
add_simgear_autotest(test_repository test_repository.cxx)


add_simgear_autotest(test_untar test_untar.cxx)
set_target_properties(test_untar PROPERTIES
  COMPILE_DEFINITIONS "SRC_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}\"" )

endif(ENABLE_TESTS)
