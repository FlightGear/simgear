
file(WRITE ${PROJECT_BINARY_DIR}/simgear/version.h "#define SIMGEAR_VERSION ${SIMGEAR_VERSION}")

foreach( mylibfolder 
        bucket
        debug
        ephemeris
        io
        magvar
        math
        misc
        nasal
        props
        route
        serial
        structure
        threads
        timing
        xml
    )

    add_subdirectory(${mylibfolder})

endforeach( mylibfolder )

if(NOT SIMGEAR_HEADLESS)
    add_subdirectory(environment)
    add_subdirectory(screen)
    add_subdirectory(scene)
    add_subdirectory(sound)
endif(NOT SIMGEAR_HEADLESS)


if(ENABLE_RTI)
    add_subdirectory(hla)
endif(ENABLE_RTI)


set(HEADERS compiler.h constants.h sg_inlines.h ${PROJECT_BINARY_DIR}/simgear/version.h)
install (FILES ${HEADERS}  DESTINATION include/simgear/)

if(SIMGEAR_SHARED)
    message(STATUS "building shared library")
    get_property(coreSources GLOBAL PROPERTY CORE_SOURCES)
    get_property(sceneSources GLOBAL PROPERTY SCENE_SOURCES)
    get_property(publicHeaders GLOBAL PROPERTY PUBLIC_HEADERS)

    add_library(SimGearCore SHARED ${coreSources})
    set_property(TARGET SimGearCore PROPERTY COMPILE_FLAGS "-DNO_OPENSCENEGRAPH_INTERFACE=1")
    
  # set_property(TARGET SimGearCore PROPERTY FRAMEWORK 1)
  #  message(STATUS "public header: ${publicHeaders}")
  #  set_property(TARGET SimGearCore PROPERTY PUBLIC_HEADER "${publicHeaders}")
    set_property(TARGET SimGearCore PROPERTY LINKER_LANGUAGE CXX)

    set_property(TARGET SimGearCore PROPERTY VERSION   ${SIMGEAR_VERSION})
    set_property(TARGET SimGearCore PROPERTY SOVERSION ${SIMGEAR_SOVERSION})

    target_link_libraries(SimGearCore ${ZLIB_LIBRARY} ${RT_LIBRARY})
    install(TARGETS SimGearCore LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

    if(NOT SIMGEAR_HEADLESS)
        if(LIBSVN_FOUND)
            add_definitions(${APR_CFLAGS})

            IF(APPLE)
                set_property(SOURCE scene/tsync/terrasync.cxx PROPERTY COMPILE_FLAGS "-iwithsysroot ${LIBSVN_INCLUDE_DIR}")
            ELSE()
                include_directories(${LIBSVN_INCLUDE_DIR})
            ENDIF(APPLE)
        endif(LIBSVN_FOUND)
        
        list(APPEND sceneSources scene/util/SGCoreOSGDependant.cxx)
        
        add_library(SimGearScene SHARED ${sceneSources})
      # set_property(TARGET SimGearScene PROPERTY FRAMEWORK 1)
      # set_property(TARGET SimGearScene PROPERTY PUBLIC_HEADER "${publicHeaders}")
        set_property(TARGET SimGearScene PROPERTY LINKER_LANGUAGE CXX)
        set_property(TARGET SimGearScene PROPERTY VERSION   ${SIMGEAR_VERSION})
        set_property(TARGET SimGearScene PROPERTY SOVERSION ${SIMGEAR_SOVERSION})
    
        target_link_libraries(SimGearScene 
            SimGearCore
            ${ZLIB_LIBRARY} 
            ${OPENSCENEGRAPH_LIBRARIES} 
            ${OPENAL_LIBRARY} ${ALUT_LIBRARY}
            ${OPENGL_LIBRARY})
    
        if(LIBSVN_FOUND)
            target_link_libraries(SimGearScene ${LIBSVN_LIBRARIES})
        endif(LIBSVN_FOUND)

        install(TARGETS SimGearScene LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
    endif()
   
endif(SIMGEAR_SHARED)
    
