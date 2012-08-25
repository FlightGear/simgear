include (SimGearComponent)

set(HEADERS 
    CopyOp.hxx
    DeletionManager.hxx
    NodeAndDrawableVisitor.hxx
    Noise.hxx
    OptionsReadFileCallback.hxx
    OsgMath.hxx
    OsgSingleton.hxx
    PrimitiveUtils.hxx
    QuadTreeBuilder.hxx
    RenderConstants.hxx
    SGDebugDrawCallback.hxx
    SGEnlargeBoundingBox.hxx
    SGNodeMasks.hxx
    SGPickCallback.hxx
    SGReaderWriterOptions.hxx
    SGSceneFeatures.hxx
    SGSceneUserData.hxx
    SGStateAttributeVisitor.hxx
    SGTextureStateAttributeVisitor.hxx
    SGUpdateVisitor.hxx
    SplicingVisitor.hxx
    StateAttributeFactory.hxx
    UpdateOnceCallback.hxx
    VectorArrayAdapter.hxx
    project.hxx
    )

set(SOURCES 
    CopyOp.cxx
    DeletionManager.cxx
    NodeAndDrawableVisitor.cxx
    Noise.cxx
    OptionsReadFileCallback.cxx
    PrimitiveUtils.cxx
    QuadTreeBuilder.cxx
    SGEnlargeBoundingBox.cxx
    SGReaderWriterOptions.cxx
    SGSceneFeatures.cxx
    SGSceneUserData.cxx
    SGStateAttributeVisitor.cxx
    SGTextureStateAttributeVisitor.cxx
    SplicingVisitor.cxx
    StateAttributeFactory.cxx
    UpdateOnceCallback.cxx
    project.cxx
    )

simgear_scene_component(util scene/util "${SOURCES}" "${HEADERS}")
