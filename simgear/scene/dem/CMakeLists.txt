include (SimGearComponent)

set(HEADERS
    ReaderWriterPGT.hxx
    SGDem.hxx
    SGDemLevel.hxx
    SGDemRoot.hxx
    SGDemSession.hxx
    SGDemTile.hxx
    SGMesh.hxx
)

set(SOURCES
    ReaderWriterPGT.cxx
    SGDem.cxx
    SGDemLevel.cxx
    SGDemRoot.cxx
    SGDemSession.cxx
    SGDemTile.cxx
    SGDemTile_gdal.cxx
    SGMesh.cxx
)

simgear_scene_component(dem scene/dem "${SOURCES}" "${HEADERS}")
