
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

add_simgear_autotest(test_props props_test.cxx)
add_simgear_autotest(test_propertyObject propertyObject_test.cxx)
add_simgear_autotest(test_easing_functions easing_functions_test.cxx)

endif(ENABLE_TESTS)
