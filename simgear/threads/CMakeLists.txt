include (SimGearComponent)

set(HEADERS 
    SGGuard.hxx
    SGQueue.hxx
    SGThread.hxx)

set(SOURCES SGThread.cxx)
simgear_component(threads threads "${SOURCES}" "${HEADERS}")
