include (SimGearComponent)

set(HEADERS
  canvas_fwd.hxx
  Canvas.hxx
  CanvasEvent.hxx
  CanvasEventListener.hxx
  CanvasEventManager.hxx
  CanvasEventTypes.hxx
  CanvasEventVisitor.hxx
  CanvasMgr.hxx
  CanvasObjectPlacement.hxx
  CanvasPlacement.hxx
  CanvasSystemAdapter.hxx
  MouseEvent.hxx
  ODGauge.hxx
  VGInitOperation.hxx
)

set(SOURCES
  Canvas.cxx
  CanvasEvent.cxx
  CanvasEventListener.cxx
  CanvasEventManager.cxx
  CanvasEventVisitor.cxx
  CanvasMgr.cxx
  CanvasObjectPlacement.cxx
  CanvasPlacement.cxx
  ODGauge.cxx
  VGInitOperation.cxx
)

add_subdirectory(ShivaVG/src)
add_subdirectory(elements)

simgear_scene_component(canvas canvas "${SOURCES}" "${HEADERS}")
