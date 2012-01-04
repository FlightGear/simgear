include (SimGearComponent)

set(HEADERS 
    BoundingVolumeBuildVisitor.hxx
    CheckSceneryVisitor.hxx
    ConditionNode.hxx
    ModelRegistry.hxx
    SGClipGroup.hxx
    SGInteractionAnimation.hxx
    SGMaterialAnimation.hxx
    SGOffsetTransform.hxx
    SGReaderWriterXML.hxx
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
    SGLightAnimation.cxx
    SGMaterialAnimation.cxx
    SGOffsetTransform.cxx
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
    shadanim.cxx
    )

simgear_scene_component(model scene/model "${SOURCES}" "${HEADERS}")
