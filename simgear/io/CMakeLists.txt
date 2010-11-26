

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
    )

simgear_component(io io "${SOURCES}" "${HEADERS}")