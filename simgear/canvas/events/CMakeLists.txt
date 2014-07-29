include (SimGearComponent)

set(HEADERS
  CustomEvent.hxx
  DeviceEvent.hxx
  KeyboardEvent.hxx
  MouseEvent.hxx
)

set(SOURCES
  CustomEvent.cxx
  DeviceEvent.cxx
  KeyboardEvent.cxx
  MouseEvent.cxx
)

simgear_scene_component(canvas-events canvas/events "${SOURCES}" "${HEADERS}")

add_boost_test(canvas_event
  SOURCES event_test.cpp
  LIBRARIES ${TEST_LIBS}
)