

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
    )

simgear_component(io io "${SOURCES}" "${HEADERS}")

add_executable(test_sock socktest.cxx)
target_link_libraries(test_sock sgio sgstructure sgthreads sgdebug
    ${CMAKE_THREAD_LIBS_INIT}
    ${RT_LIBRARY})

add_executable(test_http test_HTTP.cxx)
target_link_libraries(test_http
    sgio sgstructure sgthreads sgtiming sgmisc sgdebug
    ${CMAKE_THREAD_LIBS_INIT}
    ${RT_LIBRARY})

add_test(http ${EXECUTABLE_OUTPUT_PATH}/test_http)

add_executable(httpget httpget.cxx)
target_link_libraries(httpget
    sgio sgstructure sgthreads sgtiming sgmisc sgdebug
    ${CMAKE_THREAD_LIBS_INIT}
    ${RT_LIBRARY})
