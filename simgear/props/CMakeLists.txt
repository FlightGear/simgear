
include (SimGearComponent)

set(HEADERS 
    AtomicChangeListener.hxx
    ExtendedPropertyAdapter.hxx
    condition.hxx
    propertyObject.hxx
    props.hxx
    props_io.hxx
    )

set(SOURCES 
    AtomicChangeListener.cxx
    condition.cxx
    propertyObject.cxx
    props.cxx
    props_io.cxx
    )

simgear_component(props props "${SOURCES}" "${HEADERS}")