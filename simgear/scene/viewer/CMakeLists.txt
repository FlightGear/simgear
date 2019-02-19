set(HEADERS
  ClusteredForward.hxx
  Compositor.hxx
  CompositorCommon.hxx
  CompositorBuffer.hxx
  CompositorPass.hxx
  )

set(SOURCES
  ClusteredForward.cxx
  Compositor.cxx
  CompositorBuffer.cxx
  CompositorPass.cxx
  )

simgear_scene_component(viewer scene/viewer "${SOURCES}" "${HEADERS}")
