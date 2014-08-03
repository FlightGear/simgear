include (SimGearComponent)

set(HEADERS
  AlignFlag_values.hxx
  BoxLayout.hxx
  Layout.hxx
  LayoutItem.hxx
  NasalWidget.hxx
  SpacerItem.hxx
)

set(SOURCES
  BoxLayout.cxx
  Layout.cxx
  LayoutItem.cxx
  NasalWidget.cxx
  SpacerItem.cxx
)

simgear_scene_component(canvas-layout canvas/layout "${SOURCES}" "${HEADERS}")

add_boost_test(canvas_layout
  SOURCES canvas_layout_test.cxx
  LIBRARIES ${TEST_LIBS}
)