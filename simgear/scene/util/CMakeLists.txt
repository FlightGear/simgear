include (SimGearComponent)

set(HEADERS
    ColorInterpolator.hxx
    color_space.hxx
    CopyOp.hxx
    DeletionManager.hxx
    MemoryBarrier.hxx
    NodeAndDrawableVisitor.hxx
    Noise.hxx
    OptionsReadFileCallback.hxx
    OrthophotoManager.hxx
    OsgDebug.hxx
    OsgMath.hxx
    OsgSingleton.hxx
    OsgUtils.hxx
    OsgIoCapture.hxx
    parse_color.hxx
    PrimitiveUtils.hxx
    ProjectionMatrix.hxx
    QuadTreeBuilder.hxx
    RenderConstants.hxx
    SGDebugDrawCallback.hxx
    SGEnlargeBoundingBox.hxx
    SGImageUtils.hxx
    SGNodeMasks.hxx
    SGPickCallback.hxx
    SGReaderWriterOptions.hxx
    SGSceneFeatures.hxx
    SGSceneUserData.hxx
    SGStateAttributeVisitor.hxx
    SGTextureStateAttributeVisitor.hxx
    SGTransientModelData.hxx
    SGUpdateVisitor.hxx
    SplicingVisitor.hxx
    StateAttributeFactory.hxx
    UpdateOnceCallback.hxx
    VectorArrayAdapter.hxx
    project.hxx
    SGProgram.hxx
    load_shader.hxx
    )

set(SOURCES
    ColorInterpolator.cxx
    color_space.cxx
    CopyOp.cxx
    DeletionManager.cxx
    NodeAndDrawableVisitor.cxx
    Noise.cxx
    OptionsReadFileCallback.cxx
    OrthophotoManager.cxx
    OsgDebug.cxx
    OsgUtils.cxx
    parse_color.cxx
    PrimitiveUtils.cxx
    ProjectionMatrix.cxx
    QuadTreeBuilder.cxx
    SGEnlargeBoundingBox.cxx
    SGImageUtils.cxx
    SGReaderWriterOptions.cxx
    SGSceneFeatures.cxx
    SGSceneUserData.cxx
    SGStateAttributeVisitor.cxx
    SGTextureStateAttributeVisitor.cxx
    SplicingVisitor.cxx
    StateAttributeFactory.cxx
    UpdateOnceCallback.cxx
    project.cxx
    SGProgram.cxx
    load_shader.cxx
    )

simgear_scene_component(util scene/util "${SOURCES}" "${HEADERS}")

if(ENABLE_TESTS)

add_simgear_autotest(test_parse_color parse_color_test.cxx )
target_link_libraries(test_parse_color SimGearScene)

endif(ENABLE_TESTS)
