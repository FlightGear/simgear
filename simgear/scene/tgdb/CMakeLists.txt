include (SimGearComponent)

set(HEADERS 
    GroundLightManager.hxx
    ReaderWriterSTG.hxx
    SGDirectionalLightBin.hxx
    SGLightBin.hxx
    SGModelBin.hxx
    SGOceanTile.hxx
    SGReaderWriterBTG.hxx
    SGTexturedTriangleBin.hxx
    SGTriangleBin.hxx
    SGVasiDrawable.hxx
    SGVertexArrayBin.hxx
    ShaderGeometry.hxx
    TileCache.hxx
    TileEntry.hxx
    TreeBin.hxx
    apt_signs.hxx
    obj.hxx
    pt_lights.hxx
    userdata.hxx
    )

set(SOURCES 
    GroundLightManager.cxx
    ReaderWriterSTG.cxx
    SGOceanTile.cxx
    SGReaderWriterBTG.cxx
    SGVasiDrawable.cxx
    ShaderGeometry.cxx
    TileCache.cxx
    TileEntry.cxx
    TreeBin.cxx
    apt_signs.cxx
    obj.cxx
    pt_lights.cxx
    userdata.cxx
    )

simgear_scene_component(tgdb scene/tgdb "${SOURCES}" "${HEADERS}")
