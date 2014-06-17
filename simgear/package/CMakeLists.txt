
include (SimGearComponent)

set(HEADERS 
    Catalog.hxx
    Package.hxx
    Install.hxx
    Root.hxx
    Delegate.hxx
    md5.h # TODO expose somehow more elegant (eg. a function accepting a string)
    )

set(SOURCES 
    Catalog.cxx
    Package.cxx
    Install.cxx
    Root.cxx
# internal helpers
    md5.c
    ioapi.c ioapi_mem.c ioapi.h
    unzip.h unzip.c
    )

simgear_component(package package "${SOURCES}" "${HEADERS}")

if(ENABLE_PKGUTIL)
  add_executable(sg_pkgutil pkgutil.cxx)
  target_link_libraries(sg_pkgutil ${TEST_LIBS})
endif()
