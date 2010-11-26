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

add_library(sgutil STATIC ${SOURCES} ${HEADERS} )

install (TARGETS sgutil ARCHIVE DESTINATION lib)
install (FILES ${HEADERS}  DESTINATION include/simgear/scene/util)

