set(HEADERS
  ClusteredShading.hxx
  Compositor.hxx
  CompositorBuffer.hxx
  CompositorPass.hxx
  CompositorUtil.hxx
  )

set(SOURCES
  ClusteredShading.cxx
  Compositor.cxx
  CompositorBuffer.cxx
  CompositorPass.cxx
  CompositorUtil.cxx
  )

simgear_scene_component(viewer scene/viewer "${SOURCES}" "${HEADERS}")
