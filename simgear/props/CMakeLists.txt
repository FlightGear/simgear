
include (SimGearComponent)

set(HEADERS 
    AtomicChangeListener.hxx
    condition.hxx
    easing_functions.hxx
    ExtendedPropertyAdapter.hxx
    PropertyBasedElement.hxx
    PropertyBasedMgr.hxx
    PropertyInterpolationMgr.hxx
    PropertyInterpolator.hxx
    propertyObject.hxx
    props.hxx
    props_io.hxx
    propsfwd.hxx
    tiedpropertylist.hxx
    vectorPropTemplates.hxx
    )

set(SOURCES 
    AtomicChangeListener.cxx
    condition.cxx
    easing_functions.cxx
    PropertyBasedElement.cxx
    PropertyBasedMgr.cxx
    PropertyInterpolationMgr.cxx
    PropertyInterpolator.cxx
    propertyObject.cxx
    props.cxx
    props_io.cxx
    )

simgear_component(props props "${SOURCES}" "${HEADERS}")

if(ENABLE_TESTS)

add_executable(test_props props_test.cxx)
target_link_libraries(test_props ${TEST_LIBS})
add_test(props ${EXECUTABLE_OUTPUT_PATH}/test_props)

add_executable(test_propertyObject propertyObject_test.cxx)
target_link_libraries(test_propertyObject ${TEST_LIBS})
add_test(propertyObject ${EXECUTABLE_OUTPUT_PATH}/test_propertyObject)

add_executable(test_easing_functions easing_functions_test.cxx)
target_link_libraries(test_easing_functions ${TEST_LIBS})
add_test(easing_functions ${EXECUTABLE_OUTPUT_PATH}/test_easing_functions)

endif(ENABLE_TESTS)
