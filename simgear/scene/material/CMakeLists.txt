set(HEADERS 
    Effect.hxx
    EffectBuilder.hxx
    EffectCullVisitor.hxx
    EffectGeode.hxx
    GLPredicate.hxx
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
    Pass.cxx
    Technique.cxx
    TextureBuilder.cxx
    makeEffect.cxx
    mat.cxx
    matlib.cxx
    matmodel.cxx
    mipmap.cxx
    )

simgear_scene_component(material scene/material "${SOURCES}" "${HEADERS}")

