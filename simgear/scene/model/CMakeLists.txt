include (SimGearComponent)

set(HEADERS 
    BoundingVolumeBuildVisitor.hxx
    BVHDebugCollectVisitor.hxx
    BVHPageNodeOSG.hxx
    CheckSceneryVisitor.hxx
    ConditionNode.hxx
    ModelRegistry.hxx
    PrimitiveCollector.hxx
    ReaderWriterGLTF.hxx
    SGClipGroup.hxx
    SGInteractionAnimation.hxx
    SGLight.hxx
    SGMaterialAnimation.hxx
    SGPickAnimation.hxx
    SGOffsetTransform.hxx
    SGReaderWriterXML.hxx
    SGRotateTransform.hxx
    SGScaleTransform.hxx
    SGText.hxx
    SGTrackToAnimation.hxx
    SGTranslateTransform.hxx
    animation.hxx
    model.hxx
    modellib.hxx
    particles.hxx
    persparam.hxx
    placement.hxx
    )

set(SOURCES 
    BVHPageNodeOSG.cxx
    CheckSceneryVisitor.cxx
    ConditionNode.cxx
    ModelRegistry.cxx
    PrimitiveCollector.cxx
    ReaderWriterGLTF.cxx
    SGClipGroup.cxx
    SGInteractionAnimation.cxx
    SGLight.cxx
    SGLightAnimation.cxx
    SGPickAnimation.cxx
    SGMaterialAnimation.cxx
    SGOffsetTransform.cxx
    SGReaderWriterXML.cxx
    SGRotateTransform.cxx
    SGScaleTransform.cxx
    SGText.cxx
    SGTrackToAnimation.cxx
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

if(ENABLE_TESTS)
  add_simgear_scene_autotest(test_animations animation_test.cxx)
endif(ENABLE_TESTS)
