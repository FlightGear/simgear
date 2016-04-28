include (SimGearComponent)
INCLUDE (CheckFunctionExists)
INCLUDE (CheckSymbolExists) 

CHECK_FUNCTION_EXISTS(poll HAVE_POLL)
CHECK_FUNCTION_EXISTS(getopt HAVE_GETOPT)
CHECK_FUNCTION_EXISTS(inet_ntop HAVE_INET_PTON_NTOP)
CHECK_SYMBOL_EXISTS(AF_INET6 "netinet/in.h" HAVE_IPv6) 

CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_BINARY_DIR}/config.h)
include_directories(${CMAKE_BINARY_DIR})

set(HEADERS
	config.h
)

set(SOURCES
	udns_resolver.c
	udns_dn.c 
	udns_dntosp.c 
	udns_parse.c 
	udns_resolver.c 
	udns_init.c
    udns_misc.c 
	udns_XtoX.c
    udns_rr_a.c 
	udns_rr_ptr.c 
	udns_rr_mx.c 
	udns_rr_txt.c 
	udns_bl.c
    udns_rr_srv.c 
	udns_rr_naptr.c 
	udns_codes.c 
	udns_jran.c
)

simgear_component(udns 3rdparty/udns "${SOURCES}" "${HEADERS}")
#simgear_component(utf8-internal 3rdparty/utf8/utf8 "" "${HEADERS_utf8}")

