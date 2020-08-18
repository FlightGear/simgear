include (SimGearComponent)

set(HEADERS
  CanvasElement.hxx
  CanvasGroup.hxx
  CanvasImage.hxx
  CanvasMap.hxx
  CanvasPath.hxx
  CanvasText.hxx
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

add_boost_test(canvas_element
  SOURCES canvas_element_test.cpp
  LIBRARIES SimGearScene
)