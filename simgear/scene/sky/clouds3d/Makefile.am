includedir = @includedir@/scene/sky/clouds3d

# enable the following line once extgl.c get's added to the distro
EXTRA_DIST = extgl.c extgl.h

if EXTGL_NEEDED
  EXTGL_SOURCE = extgl.c extgl.h
else
  EXTGL_SOURCE = 
endif

lib_LIBRARIES = libsgclouds3d.a

include_HEADERS = \
	SkySceneLoader.hpp \
	SkyUtil.hpp

libsgclouds3d_a_SOURCES = \
        $(EXTGL_SOURCE) \
	vec3fv.cpp vec3fv.hpp vec3f.hpp vec4f.hpp vec2f.hpp \
	mat16fv.cpp mat16fv.hpp \
	mat33.hpp mat33impl.hpp \
	mat44.hpp mat44impl.hpp \
	tri.cpp tri.hpp \
	plane.cpp plane.hpp \
	camera.cpp camera.hpp \
	camutils.cpp camutils.hpp \
	glut_shapes.c glut_shapes.h \
	minmaxbox.cpp minmaxbox.hpp \
	SkyAABBTree.hpp \
	SkyArchive.cpp SkyArchive.hpp \
	SkyBoundingVolume.hpp \
	SkyBVTree.hpp SkyBVTreeSplitter.hpp \
	SkyCloud.cpp SkyCloud.hpp \
	SkyCloudParticle.hpp \
	SkyContext.cpp SkyContext.hpp \
	SkyDynamicTextureManager.cpp SkyDynamicTextureManager.hpp \
	SkyLight.cpp SkyLight.hpp \
	SkyMaterial.cpp SkyMaterial.hpp \
	SkyMinMaxBox.cpp SkyMinMaxBox.hpp \
	SkyRenderable.hpp \
	SkyRenderableInstance.hpp \
	SkyRenderableInstanceCloud.cpp SkyRenderableInstanceCloud.hpp \
	SkyRenderableInstanceGeneric.hpp \
	SkyRenderableInstanceGroup.cpp SkyRenderableInstanceGroup.hpp \
	SkySceneLoader.cpp \
	SkySceneManager.cpp SkySceneManager.hpp \
	SkySingleton.hpp \
	SkyTexture.hpp \
	SkyTextureManager.cpp SkyTextureManager.hpp \
	SkyTextureState.cpp SkyTextureState.hpp \
	SkyUtil.cpp

INCLUDES = -I$(top_srcdir)
