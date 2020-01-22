include (SimGearComponent)

set(HEADERS 
    SGQueue.hxx
    SGThread.hxx)

set(SOURCES SGThread.cxx)
simgear_component(threads threads "${SOURCES}" "${HEADERS}")
