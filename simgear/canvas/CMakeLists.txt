include (SimGearComponent)

set(HEADERS
  canvas_fwd.hxx
  Canvas.hxx
  CanvasEvent.hxx
  CanvasEventManager.hxx
  CanvasEventTypes.hxx
  CanvasEventVisitor.hxx
  CanvasMgr.hxx
  CanvasObjectPlacement.hxx
  CanvasPlacement.hxx
  CanvasSystemAdapter.hxx
  CanvasWindow.hxx
  ODGauge.hxx
  VGInitOperation.hxx
)

set(SOURCES
  Canvas.cxx
  CanvasEvent.cxx
  CanvasEventManager.cxx
  CanvasEventVisitor.cxx
  CanvasMgr.cxx
  CanvasObjectPlacement.cxx
  CanvasPlacement.cxx
  CanvasWindow.cxx
  ODGauge.cxx
  VGInitOperation.cxx
)

add_subdirectory(ShaderVG/src)
add_subdirectory(elements)
add_subdirectory(events)
add_subdirectory(layout)

simgear_scene_component(canvas canvas "${SOURCES}" "${HEADERS}")
