includedir = @includedir@/sky/clouds3d

EXTRA_DIST = extlg.c
if EXTGL_NEEDED
  EXTGL_SOURCE = extgl.c
else
  EXTGL_SOURCE = 
endif

lib_LIBRARIES = libsgclouds3d.a

include_HEADERS = \
	SkySceneLoader.hpp \
	SkyUtil.hpp

libsgclouds3d_a_SOURCES = \
        $(EXTRA_SOURCE) \
	vec3fv.cpp \
	mat16fv.cpp \
	tri.cpp \
	plane.cpp \
	camera.cpp \
	camutils.cpp \
	minmaxbox.cpp \
	SkyMinMaxBox.cpp \
	SkyLight.cpp \
	SkyMaterial.cpp \
	SkyTextureManager.cpp \
	SkyTextureState.cpp \
	SkyDynamicTextureManager.cpp \
	SkyRenderableInstanceCloud.cpp \
	SkyRenderableInstanceGroup.cpp \
	SkyCloud.cpp  \
	SkyArchive.cpp \
	SkyUtil.cpp \
	SkyContext.cpp \
	SkySceneManager.cpp \
	SkySceneLoader.cpp

INCLUDES = -I$(top_srcdir)
