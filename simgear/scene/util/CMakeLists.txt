include (SimGearComponent)

set(HEADERS 
    CopyOp.hxx
    NodeAndDrawableVisitor.hxx
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
    )

set(SOURCES 
    CopyOp.cxx
    NodeAndDrawableVisitor.cxx
    PrimitiveUtils.cxx
    QuadTreeBuilder.cxx
    SGEnlargeBoundingBox.cxx
    SGSceneFeatures.cxx
    SGSceneUserData.cxx
    SGStateAttributeVisitor.cxx
    SGTextureStateAttributeVisitor.cxx
    SplicingVisitor.cxx
    StateAttributeFactory.cxx
    UpdateOnceCallback.cxx
    )

simgear_scene_component(util scene/util "${SOURCES}" "${HEADERS}")
