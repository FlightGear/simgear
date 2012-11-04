include (SimGearComponent)

set(HEADERS
  canvas_fwd.hxx
  Canvas.hxx
  CanvasMgr.hxx
  CanvasPlacement.hxx
  CanvasSystemAdapter.hxx
  MouseEvent.hxx
  ODGauge.hxx
  VGInitOperation.hxx
)

set(SOURCES
  Canvas.cxx
  CanvasMgr.cxx
  CanvasPlacement.cxx
  ODGauge.cxx
)

add_subdirectory(ShivaVG/src)
include_directories(ShivaVG/include)

add_subdirectory(elements)
simgear_scene_component(canvas canvas "${SOURCES}" "${HEADERS}")
