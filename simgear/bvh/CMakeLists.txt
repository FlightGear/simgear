include (SimGearComponent)

set(HEADERS
    BVHBoundingBoxVisitor.hxx
    BVHGroup.hxx
    BVHLineGeometry.hxx
    BVHLineSegmentVisitor.hxx
    BVHMotionTransform.hxx
    BVHNearestPointVisitor.hxx
    BVHNode.hxx
    BVHPageNode.hxx
    BVHPageRequest.hxx
    BVHPager.hxx
    BVHStaticBinary.hxx
    BVHStaticData.hxx
    BVHStaticGeometry.hxx
    BVHStaticGeometryBuilder.hxx
    BVHStaticLeaf.hxx
    BVHStaticNode.hxx
    BVHStaticTriangle.hxx
    BVHSubTreeCollector.hxx
    BVHMaterial.hxx
    BVHTransform.hxx
    BVHVisitor.hxx
)

set(SOURCES
    BVHGroup.cxx
    BVHLineGeometry.cxx
    BVHLineSegmentVisitor.cxx
    BVHMotionTransform.cxx
    BVHNode.cxx
    BVHPageNode.cxx
    BVHPageRequest.cxx
    BVHPager.cxx
    BVHStaticBinary.cxx
    BVHStaticGeometry.cxx
    BVHStaticLeaf.cxx
    BVHStaticNode.cxx
    BVHStaticTriangle.cxx
    BVHSubTreeCollector.cxx
    BVHMaterial.cxx
    BVHTransform.cxx
)

simgear_component(bvh bvh "${SOURCES}" "${HEADERS}")

if(ENABLE_TESTS)
  add_simgear_autotest(bvhtest bvhtest.cxx)
endif(ENABLE_TESTS)
