include (SimGearComponent)

set(HEADERS
  CanvasElement.hxx
  CanvasGroup.hxx
  CanvasImage.hxx
  CanvasMap.hxx
  CanvasPath.hxx
  CanvasText.hxx
)

set(DETAIL_HEADERS
  detail/add_segment_variadic.hxx
)

set(SOURCES
  CanvasElement.cxx
  CanvasGroup.cxx
  CanvasImage.cxx
  CanvasMap.cxx
  CanvasPath.cxx
  CanvasText.cxx
)

simgear_scene_component(canvas-elements canvas/elements "${SOURCES}" "${HEADERS}")
simgear_component(canvas-elements/detail canvas/elements/detail "" "${DETAIL_HEADERS}")

add_boost_test(canvas_element
  SOURCES canvas_element_test.cpp
  LIBRARIES ${TEST_LIBS}
)