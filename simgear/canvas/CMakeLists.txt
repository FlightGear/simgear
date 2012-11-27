include (SimGearComponent)

set(HEADERS
  canvas_fwd.hxx
  Canvas.hxx
  CanvasEvent.hxx
  CanvasEventListener.hxx
  CanvasEventTypes.hxx
  CanvasEventVisitor.hxx
  CanvasMgr.hxx
  CanvasPlacement.hxx
  CanvasSystemAdapter.hxx
  MouseEvent.hxx
  ODGauge.hxx
  VGInitOperation.hxx
)

set(SOURCES
  Canvas.cxx
  Canvas.cxx
  CanvasEvent.cxx
  CanvasEventListener.cxx
  CanvasEventVisitor.cxx
  CanvasMgr.cxx
  CanvasPlacement.cxx
  ODGauge.cxx
  VGInitOperation.cxx
)

add_subdirectory(ShivaVG/src)
add_subdirectory(elements)

simgear_scene_component(canvas canvas "${SOURCES}" "${HEADERS}")
