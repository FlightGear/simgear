include (SimGearComponent)

set(HEADERS 
    BVHBoundingBoxVisitor.hxx
    BVHDebugCollectVisitor.hxx
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

simgear_scene_component(bvh scene/bvh "${SOURCES}" "${HEADERS}")
