includedir = @includedir@/scene/bvh

check_PROGRAMS  = bvhtest
TESTS           = $(check_PROGRAMS)

bvhtest_SOURCES = bvhtest.cxx
bvhtest_LDADD = libsgbvh.a -lsgstructure -lsgmath $(base_LIBS)

lib_LIBRARIES = libsgbvh.a

noinst_HEADERS =

include_HEADERS = \
	BVHBoundingBoxVisitor.hxx \
	BVHDebugCollectVisitor.hxx \
	BVHGroup.hxx \
	BVHLineSegmentVisitor.hxx \
	BVHLineGeometry.hxx \
	BVHMotionTransform.hxx \
	BVHNode.hxx \
	BVHStaticBinary.hxx \
	BVHStaticData.hxx \
	BVHStaticGeometry.hxx \
	BVHStaticGeometryBuilder.hxx \
	BVHStaticLeaf.hxx \
	BVHStaticNode.hxx \
	BVHStaticTriangle.hxx \
	BVHSubTreeCollector.hxx \
	BVHTransform.hxx \
	BVHVisitor.hxx

libsgbvh_a_SOURCES = \
	BVHGroup.cxx \
	BVHLineGeometry.cxx \
	BVHLineSegmentVisitor.cxx \
	BVHMotionTransform.cxx \
	BVHNode.cxx \
	BVHStaticBinary.cxx \
	BVHStaticGeometry.cxx \
	BVHStaticLeaf.cxx \
	BVHStaticNode.cxx \
	BVHStaticTriangle.cxx \
	BVHSubTreeCollector.cxx \
	BVHTransform.cxx

INCLUDES = -I$(top_srcdir)
