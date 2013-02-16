include(SimGearComponent)

set(HEADERS
  ../include/vg/openvg.h
  ../include/vg/vgu.h
)

set(SOURCES
  shArrays.c
  shArrays.h
  shContext.c
  shContext.h
  shExtensions.c
  shExtensions.h
  shGeometry.c
  shGeometry.h
  shImage.c
  shImage.h
  shPaint.c
  shPaint.h
  shParams.c
  shPath.c
  shPath.h
  shPipeline.c
  shVectors.c
  shVectors.h
  shVgu.c
)

simgear_scene_component(ShivaVG canvas/ShivaVG "${SOURCES}" "${HEADERS}")