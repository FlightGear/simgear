includedir = @includedir@/scene/tgdb

lib_LIBRARIES = libsgtgdb.a

noinst_HEADERS =

include_HEADERS = \
	apt_signs.hxx \
	obj.hxx \
	pt_lights.hxx \
	userdata.hxx \
	ReaderWriterSTG.hxx \
	SGVasiDrawable.hxx \
	SGDirectionalLightBin.hxx \
	SGLightBin.hxx \
	SGOceanTile.hxx \
	SGReaderWriterBTG.hxx \
	SGReaderWriterBTGOptions.hxx \
	SGTexturedTriangleBin.hxx \
	SGTriangleBin.hxx \
	SGVertexArrayBin.hxx \
	GroundLightManager.hxx \
	ShaderGeometry.hxx \
	TileCache.hxx \
	TileEntry.hxx

libsgtgdb_a_SOURCES = \
	apt_signs.cxx \
	obj.cxx \
	pt_lights.cxx \
	userdata.cxx \
	ReaderWriterSTG.cxx \
	SGOceanTile.cxx \
	SGReaderWriterBTG.cxx SGReaderWriterBTG.hxx \
	SGVasiDrawable.cxx \
	GroundLightManager.cxx \
	SGModelBin.hxx \
	ShaderGeometry.cxx \
	TileCache.cxx \
	TileEntry.cxx \
	TreeBin.cxx TreeBin.hxx

INCLUDES = -I$(top_srcdir)
