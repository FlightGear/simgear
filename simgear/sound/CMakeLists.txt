include (SimGearComponent)

set(HEADERS 
    sample.hxx
    sample_group.hxx
    xmlsound.hxx
    readwav.hxx
    ${CMAKE_CURRENT_BINARY_DIR}/soundmgr.hxx
    )
    
set(SOURCES 
    sample.cxx
    sample_group.cxx
    xmlsound.cxx
    readwav.cxx
    soundmgr_openal_private.hxx
    )

if (USE_AEONWAVE)
    CONFIGURE_FILE(
        "soundmgr_aeonwave.hxx"
        "soundmgr.hxx")
    set(SOURCES ${SOURCES}
        soundmgr_aeonwave.cxx
        )
else()
    CONFIGURE_FILE(
        "soundmgr_openal.hxx"
        "soundmgr.hxx")
    set(SOURCES ${SOURCES}
        soundmgr_openal.cxx
        )
endif()

simgear_scene_component(sound sound "${SOURCES}" "${HEADERS}")

if(ENABLE_TESTS AND ENABLE_SOUND)

    set( SOUND_TEST_LIBS
        ${TEST_LIBS}
    )

    if (USE_AEONWAVE)
    else ()
        if (SIMGEAR_SHARED)
        else()
            set(SOUND_TEST_LIBS ${SOUND_TEST_LIBS}
                ${OPENAL_LIBRARY}
            )
        endif()

        function(create_test TEST_NAME)
            add_executable(${TEST_NAME} ${TEST_NAME}.cxx)
            target_link_libraries(${TEST_NAME} ${SOUND_TEST_LIBS})
            set_target_properties(${TEST_NAME} PROPERTIES
                COMPILE_DEFINITIONS "SRC_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}\"" )
        endfunction()
    
        create_test(openal_test1)
        create_test(openal_test2)
        create_test(openal_test3)
    endif()
endif()
