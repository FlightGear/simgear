set(HEADERS 
    Effect.hxx
    EffectBuilder.hxx
    EffectCullVisitor.hxx
    EffectGeode.hxx
    GLPredicate.hxx
    Noise.hxx
    Pass.hxx
    Technique.hxx
    TextureBuilder.hxx
    mat.hxx
    matlib.hxx
    matmodel.hxx
    mipmap.hxx
    )

set(SOURCES 
    Effect.cxx
    EffectBuilder.cxx
    EffectCullVisitor.cxx
    EffectGeode.cxx
    GLPredicate.cxx
    Noise.cxx
    Pass.cxx
    Technique.cxx
    TextureBuilder.cxx
    makeEffect.cxx
    mat.cxx
    matlib.cxx
    matmodel.cxx
    mipmap.cxx
    )

add_library(sgmaterial STATIC ${SOURCES} ${HEADERS} )

install (TARGETS sgmaterial ARCHIVE DESTINATION lib)
install (FILES ${HEADERS}  DESTINATION include/simgear/scene/material)

