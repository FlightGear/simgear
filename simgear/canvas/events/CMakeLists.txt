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
  LIBRARIES SimGearScene
)

add_simgear_test(input_event_demo input_event_demo.cxx)
target_link_libraries(input_event_demo
  SimGearScene
)