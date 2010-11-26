set(HEADERS 
    BoundingVolumeBuildVisitor.hxx
    CheckSceneryVisitor.hxx
    ConditionNode.hxx
    ModelRegistry.hxx
    SGClipGroup.hxx
    SGInteractionAnimation.hxx
    SGMaterialAnimation.hxx
    SGOffsetTransform.hxx
    SGPagedLOD.hxx
    SGReaderWriterXML.hxx
    SGReaderWriterXMLOptions.hxx
    SGRotateTransform.hxx
    SGScaleTransform.hxx
    SGText.hxx
    SGTranslateTransform.hxx
    animation.hxx
    model.hxx
    modellib.hxx
    particles.hxx
    persparam.hxx
    placement.hxx
    )

set(SOURCES 
    CheckSceneryVisitor.cxx
    ConditionNode.cxx
    ModelRegistry.cxx
    SGClipGroup.cxx
    SGInteractionAnimation.cxx
    SGMaterialAnimation.cxx
    SGOffsetTransform.cxx
    SGPagedLOD.cxx
    SGReaderWriterXML.cxx
    SGRotateTransform.cxx
    SGScaleTransform.cxx
    SGText.cxx
    SGTranslateTransform.cxx
    animation.cxx
    model.cxx
    modellib.cxx
    particles.cxx
    persparam.cxx
    placement.cxx    
    )

add_library(sgmodel STATIC ${SOURCES} ${HEADERS} )

install (TARGETS sgmodel ARCHIVE DESTINATION lib)
install (FILES ${HEADERS}  DESTINATION include/simgear/scene/model)

