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
    add_simgear_autotest(test_embedded_resources embedded_resources_test.cxx)
endif(ENABLE_TESTS)
