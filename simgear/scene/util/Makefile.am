includedir = @includedir@/scene/util

lib_LIBRARIES = libsgutil.a

noinst_HEADERS =

include_HEADERS = \
	SGNodeMasks.hxx \
	SGUpdateVisitor.hxx \
	SGEnlargeBoundingBox.hxx \
	SGDebugDrawCallback.hxx \
	SGPickCallback.hxx \
	SGSceneFeatures.hxx \
	SGSceneUserData.hxx \
	SGStateAttributeVisitor.hxx \
	SGTextureStateAttributeVisitor.hxx \
	RenderConstants.hxx \
	StateAttributeFactory.hxx \
	VectorArrayAdapter.hxx


libsgutil_a_SOURCES = \
	SGEnlargeBoundingBox.cxx \
	SGSceneFeatures.cxx \
	SGSceneUserData.cxx \
	SGStateAttributeVisitor.cxx \
	SGTextureStateAttributeVisitor.cxx \
	StateAttributeFactory.cxx

INCLUDES = -I$(top_srcdir)
