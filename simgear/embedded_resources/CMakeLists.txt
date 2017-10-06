include (SimGearComponent)

set(HEADERS EmbeddedResource.hxx
            EmbeddedResourceManager.hxx
            EmbeddedResourceProxy.hxx)
set(SOURCES EmbeddedResource.cxx
            EmbeddedResourceManager.cxx
            EmbeddedResourceProxy.cxx)

simgear_component(embedded_resources embedded_resources
                  "${SOURCES}" "${HEADERS}")

if(ENABLE_TESTS)
    add_executable(test_embedded_resources embedded_resources_test.cxx)
    target_link_libraries(test_embedded_resources ${TEST_LIBS})
    add_test(embedded_resources
             ${EXECUTABLE_OUTPUT_PATH}/test_embedded_resources)
endif(ENABLE_TESTS)
