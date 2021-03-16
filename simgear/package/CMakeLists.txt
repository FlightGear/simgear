
include (SimGearComponent)

set(HEADERS
    Catalog.hxx
    Package.hxx
    Install.hxx
    Root.hxx
    Delegate.hxx
    PackageCommon.hxx
    )

set(SOURCES
    Catalog.cxx
    Package.cxx
    Install.cxx
    Root.cxx
	Delegate.cxx
# internal helpers
    md5.h md5.c
    ioapi.c ioapi_mem.c ioapi.h
    unzip.h unzip.c
    )

simgear_component(package package "${SOURCES}" "${HEADERS}")

if(ENABLE_PKGUTIL)
  add_simgear_test(sg_pkgutil pkgutil.cxx)
endif()

if(ENABLE_TESTS)

add_simgear_autotest(catalog_test CatalogTest.cxx)
set_target_properties(catalog_test PROPERTIES
        COMPILE_DEFINITIONS "SRC_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}\"" )

endif(ENABLE_TESTS)
