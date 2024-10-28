include (SimGearComponent)

set(HEADERS 
    BoundingVolumeBuildVisitor.hxx
    BVHDebugCollectVisitor.hxx
    BVHPageNodeOSG.hxx
    CheckSceneryVisitor.hxx
    ConditionNode.hxx
    ModelRegistry.hxx
    PrimitiveCollector.hxx
    ReaderWriterAC3D.hxx
    ReaderWriterGLTF.hxx
    SGInteractionAnimation.hxx
    SGLight.hxx
    SGPBRAnimation.hxx
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
    ReaderWriterAC3D.cxx
    ReaderWriterGLTF.cxx
    SGInteractionAnimation.cxx
    SGLight.cxx
    SGLightAnimation.cxx
    SGPickAnimation.cxx
    SGPBRAnimation.cxx
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
    )

simgear_scene_component(model scene/model "${SOURCES}" "${HEADERS}")

if(ENABLE_TESTS)
  add_simgear_scene_autotest(test_animations animation_test.cxx)
endif(ENABLE_TESTS)
