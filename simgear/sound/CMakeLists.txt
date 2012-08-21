include (SimGearComponent)

set(HEADERS 
    sample_group.hxx
    sample_openal.hxx
    sample_queue.hxx
    soundmgr_openal.hxx
    xmlsound.hxx
    readwav.hxx
    )
    
set(SOURCES 
    sample_group.cxx
    sample_openal.cxx
    sample_queue.cxx
    soundmgr_openal.cxx
    xmlsound.cxx
    readwav.cxx
    )

simgear_scene_component(sound sound "${SOURCES}" "${HEADERS}")

if(ENABLE_TESTS)

if (SIMGEAR_SHARED)
    set(SOUND_TEST_LIBS SimGearScene)
else()
    set(SOUND_TEST_LIBS
        sgsound sgio sgmath sgstructure sgthreads sgtiming sgmisc sgdebug
        ${CMAKE_THREAD_LIBS_INIT}
        ${RT_LIBRARY}
        ${OPENAL_LIBRARY}
        ${ZLIB_LIBRARY})
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
create_test(openal_test4)
endif(ENABLE_TESTS)
