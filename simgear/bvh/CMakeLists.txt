include (SimGearComponent)

set(HEADERS
    BVHBoundingBoxVisitor.hxx
    BVHGroup.hxx
    BVHLineGeometry.hxx
    BVHLineSegmentVisitor.hxx
    BVHMotionTransform.hxx
    BVHNearestPointVisitor.hxx
    BVHNode.hxx
    BVHStaticBinary.hxx
    BVHStaticData.hxx
    BVHStaticGeometry.hxx
    BVHStaticGeometryBuilder.hxx
    BVHStaticLeaf.hxx
    BVHStaticNode.hxx
    BVHStaticTriangle.hxx
    BVHSubTreeCollector.hxx
    BVHTransform.hxx
    BVHVisitor.hxx
)

set(SOURCES
    BVHGroup.cxx
    BVHLineGeometry.cxx
    BVHLineSegmentVisitor.cxx
    BVHMotionTransform.cxx
    BVHNode.cxx
    BVHStaticBinary.cxx
    BVHStaticGeometry.cxx
    BVHStaticLeaf.cxx
    BVHStaticNode.cxx
    BVHStaticTriangle.cxx
    BVHSubTreeCollector.cxx
    BVHTransform.cxx
)

simgear_component(bvh bvh "${SOURCES}" "${HEADERS}")

if(ENABLE_TESTS)
  if (SIMGEAR_SHARED)
    set(TEST_LIBS SimGearCore)
  else()
    set(TEST_LIBS sgbvh sgmath sgstructure sgdebug)
  endif()

  add_executable(bvhtest bvhtest.cxx)
  target_link_libraries(bvhtest ${TEST_LIBS})
  add_test(bvhtest ${EXECUTABLE_OUTPUT_PATH}/bvhtest)
endif(ENABLE_TESTS)
