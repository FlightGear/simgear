include(SimGearComponent)

#add_definitions(-DVG_API_EXPORT)

set(ShivaVG_Src
    ${SRCROOT}/shArrays.c
    ${SRCROOT}/shArrays.h
    ${SRCROOT}/shContext.c
    ${SRCROOT}/shContext.h
    ${SRCROOT}/shExtensions.c
    ${SRCROOT}/shExtensions.h
    ${SRCROOT}/shGeometry.c
    ${SRCROOT}/shGeometry.h
    ${SRCROOT}/shImage.c
    ${SRCROOT}/shImage.h
    ${SRCROOT}/shPaint.c
    ${SRCROOT}/shPaint.h
    ${SRCROOT}/shParams.c
    ${SRCROOT}/shPath.c
    ${SRCROOT}/shPath.h
    ${SRCROOT}/shPipeline.c
    ${SRCROOT}/shVectors.c
    ${SRCROOT}/shVectors.h
    ${SRCROOT}/shVgu.c
)

simgear_scene_component(ShivaVG canvas/ShivaVG "${ShivaVG_Src}" "")