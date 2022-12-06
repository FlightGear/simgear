include (SimGearComponent)

set(HEADERS
    ColorInterpolator.hxx
    CopyOp.hxx
    DeletionManager.hxx
    NodeAndDrawableVisitor.hxx
    Noise.hxx
    OptionsReadFileCallback.hxx
    OrthophotoManager.hxx
    OsgDebug.hxx
    OsgMath.hxx
    OsgSingleton.hxx
    parse_color.hxx
    PrimitiveUtils.hxx
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
    )

set(SOURCES
    ColorInterpolator.cxx
    CopyOp.cxx
    DeletionManager.cxx
    NodeAndDrawableVisitor.cxx
    Noise.cxx
    OptionsReadFileCallback.cxx
    OrthophotoManager.cxx
    OsgDebug.cxx
    parse_color.cxx
    PrimitiveUtils.cxx
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
    )

simgear_scene_component(util scene/util "${SOURCES}" "${HEADERS}")

if(ENABLE_TESTS)

add_executable(test_parse_color parse_color_test.cxx )
add_test(parse_color ${EXECUTABLE_OUTPUT_PATH}/test_parse_color)
target_link_libraries(test_parse_color ${TEST_LIBS})

endif(ENABLE_TESTS)
