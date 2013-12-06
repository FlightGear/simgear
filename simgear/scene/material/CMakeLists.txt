set(HEADERS 
    Effect.hxx
    EffectBuilder.hxx
    EffectCullVisitor.hxx
    EffectGeode.hxx
    Pass.hxx
    Technique.hxx
    TextureBuilder.hxx
    mat.hxx
    matlib.hxx
    matmodel.hxx
    mipmap.hxx
    parseBlendFunc.hxx
    )

set(SOURCES 
    Effect.cxx
    EffectBuilder.cxx
    EffectCullVisitor.cxx
    EffectGeode.cxx
    Pass.cxx
    Technique.cxx
    TextureBuilder.cxx
    makeEffect.cxx
    mat.cxx
    matlib.cxx
    matmodel.cxx
    mipmap.cxx
    parseBlendFunc.cxx
    )

simgear_scene_component(material scene/material "${SOURCES}" "${HEADERS}")


if(ENABLE_TESTS)

add_executable(test_parseBlendFunc parseBlendFunc_test.cxx )
add_test(parseBlendFunc ${EXECUTABLE_OUTPUT_PATH}/test_parseBlendFunc)
target_link_libraries(test_parseBlendFunc ${TEST_LIBS} ${OPENSCENEGRAPH_LIBRARIES})

endif(ENABLE_TESTS)