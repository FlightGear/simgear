
include (SimGearComponent)

set(HEADERS
    Catalog.hxx
    Package.hxx
    Install.hxx
    Root.hxx
    Delegate.hxx
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
  add_executable(sg_pkgutil pkgutil.cxx)
  target_link_libraries(sg_pkgutil ${TEST_LIBS})
endif()

if(ENABLE_TESTS)

add_executable(catalog_test CatalogTest.cxx)
target_link_libraries(catalog_test ${TEST_LIBS})

set_target_properties(catalog_test PROPERTIES
        COMPILE_DEFINITIONS "SRC_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}\"" )

add_test(catalog_test ${EXECUTABLE_OUTPUT_PATH}/catalog_test)

endif(ENABLE_TESTS)
