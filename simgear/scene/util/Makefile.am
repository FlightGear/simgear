includedir = @includedir@/scene/util

lib_LIBRARIES = libsgutil.a

noinst_HEADERS =

include_HEADERS = \
	SGDebugDrawCallback.hxx \
	SGEnlargeBoundingBox.hxx \
	SGNodeMasks.hxx \
	SGPickCallback.hxx \
	SGSceneFeatures.hxx \
	SGSceneUserData.hxx \
	SGStateAttributeVisitor.hxx \
	SGTextureStateAttributeVisitor.hxx \
	SGUpdateVisitor.hxx \
	QuadTreeBuilder.hxx \
	RenderConstants.hxx \
	StateAttributeFactory.hxx \
	VectorArrayAdapter.hxx


libsgutil_a_SOURCES = \
	SGEnlargeBoundingBox.cxx \
	SGSceneFeatures.cxx \
	SGSceneUserData.cxx \
	SGStateAttributeVisitor.cxx \
	SGTextureStateAttributeVisitor.cxx \
	StateAttributeFactory.cxx \
	QuadTreeBuilder.cxx

INCLUDES = -I$(top_srcdir)
