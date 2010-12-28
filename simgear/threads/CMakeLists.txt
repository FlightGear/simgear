include (SimGearComponent)

set(HEADERS 
    SGGuard.hxx
    SGQueue.hxx
    SGThread.hxx)


if (MSVC)
  install (FILES ${HEADERS}  DESTINATION include/simgear/threads)
else (MSVC)
  set(SOURCES SGThread.cxx)
  simgear_component(threads threads "${SOURCES}" "${HEADERS}")
endif(MSVC)
