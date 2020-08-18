include (SimGearComponent)

set(HEADERS 
    sample.hxx
    sample_group.hxx
    xmlsound.hxx
    soundmgr.hxx
    filters.hxx
    )
    
set(SOURCES 
    sample.cxx
    sample_group.cxx
    xmlsound.cxx
    filters.cxx
    )

if (USE_AEONWAVE)
    set(SOURCES ${SOURCES}
        soundmgr_aeonwave.cxx
        )
else()
    set(HEADERS ${HEADERS}
        readwav.hxx
        )
    set(SOURCES ${SOURCES}
        soundmgr_openal.cxx
        readwav.cxx
        )
endif()

simgear_scene_component(sound sound "${SOURCES}" "${HEADERS}")

if(ENABLE_TESTS AND ENABLE_SOUND)
    function(create_test TEST_NAME)
        add_executable(${TEST_NAME} ${TEST_NAME}.cxx)
        target_link_libraries(${TEST_NAME} SimGearScene ${SOUND_TEST_LIBS})
        set_target_properties(${TEST_NAME} PROPERTIES
            COMPILE_DEFINITIONS "SRC_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}\"" )

        # for simgear_config.h
        target_include_directories(${TEST_NAME} PRIVATE ${PROJECT_BINARY_DIR}/simgear)
    endfunction()

    if (USE_AEONWAVE)
        set(SOUND_TEST_LIBS ${AAX_LIBRARY})
        create_test(aeonwave_test1)
    else ()
        set(SOUND_TEST_LIBS ${OPENAL_LIBRARY})
        create_test(openal_test1)
    endif()

    create_test(soundmgr_test)
    create_test(soundmgr_test2)
endif()
