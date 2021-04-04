include (SimGearComponent)

set(HEADERS 
    GroundLightManager.hxx
    AreaFeatureBin.hxx
    LineFeatureBin.hxx
    ReaderWriterSPT.hxx
    ReaderWriterSTG.hxx
    SGBuildingBin.hxx
    SGDirectionalLightBin.hxx
    SGLightBin.hxx
    SGModelBin.hxx
    SGNodeTriangles.hxx
    SGOceanTile.hxx
    SGReaderWriterBTG.hxx
    SGTexturedTriangleBin.hxx
    SGTileDetailsCallback.hxx
    SGTileGeometryBin.hxx
    SGTriangleBin.hxx
    SGVasiDrawable.hxx
    SGVertexArrayBin.hxx
    ShaderGeometry.hxx
    TreeBin.hxx
    VPBElevationSlice.hxx
    VPBTechnique.hxx
    apt_signs.hxx
    obj.hxx
    pt_lights.hxx
    userdata.hxx
    )

set(SOURCES 
    GroundLightManager.cxx
    AreaFeatureBin.cxx
    LineFeatureBin.cxx
    ReaderWriterSPT.cxx
    ReaderWriterSTG.cxx
    SGBuildingBin.cxx
    SGOceanTile.cxx
    SGReaderWriterBTG.cxx
    SGVasiDrawable.cxx
    ShaderGeometry.cxx
    TreeBin.cxx
    VPBElevationSlice.cxx
    VPBTechnique.cxx
    apt_signs.cxx
    obj.cxx
    pt_lights.cxx
    userdata.cxx
    )

simgear_scene_component(tgdb scene/tgdb "${SOURCES}" "${HEADERS}")

if(ENABLE_TESTS)
  add_simgear_scene_autotest(BucketBoxTest BucketBoxTest.cxx)
endif(ENABLE_TESTS)
