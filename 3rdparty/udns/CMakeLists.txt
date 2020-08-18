include (SimGearComponent)
INCLUDE (CheckFunctionExists)
INCLUDE (CheckSymbolExists) 
include (CheckIncludeFile)


CHECK_FUNCTION_EXISTS(poll HAVE_POLL)
CHECK_FUNCTION_EXISTS(getopt HAVE_GETOPT)
CHECK_FUNCTION_EXISTS(inet_ntop HAVE_INET_PTON_NTOP)
CHECK_SYMBOL_EXISTS(AF_INET6 "netinet/in.h" HAVE_IPv6) 
# WINDOWS should be defined by msvc, should it?
# somehow it is not, if somebody know a better way to define WINDOWS, please fix
check_include_file(windows.h  WINDOWS)

CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_BINARY_DIR}/config.h)

set(HEADERS
)

set(SOURCES
    udns_resolver.c
    udns_dn.c
    udns_dntosp.c
    udns_parse.c
    udns_resolver.c
    udns_init.c
    udns_misc.c
    udns_rr_a.c
    udns_rr_ptr.c
    udns_rr_mx.c
    udns_rr_txt.c
    udns_bl.c
    udns_rr_srv.c
    udns_rr_naptr.c
    udns_codes.c
    udns_jran.c
    udns_XtoX.c
)

add_library(FGUdns STATIC ${SOURCES})

target_include_directories(FGUdns BEFORE PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>)
    
target_include_directories(FGUdns PRIVATE ${CMAKE_BINARY_DIR})

# match the name of imported UDns 
add_librarY(Udns::Udns ALIAS FGUdns)

# in the non-shared case, we need to export these as well
if (NOT SIMGEAR_SHARED)
    install(TARGETS FGUdns
        EXPORT SimGearTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()
