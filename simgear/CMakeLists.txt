
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

    add_library(SimGear SHARED ${coreSources})
  # set_property(TARGET SimGear PROPERTY FRAMEWORK 1)
  #  message(STATUS "public header: ${publicHeaders}")
    set_property(TARGET SimGear PROPERTY PUBLIC_HEADER "${publicHeaders}")
    set_property(TARGET SimGear PROPERTY LINKER_LANGUAGE CXX)
    
    target_link_libraries(SimGear ${ZLIB_LIBRARY})					
    install(TARGETS SimGear LIBRARY DESTINATION lib${LIB_SUFFIX}
                               PUBLIC_HEADER DESTINATION include/simgear)
                               
	if(NOT SIMGEAR_HEADLESS)
    	if(LIBSVN_FOUND)
        	add_definitions(${APR_CFLAGS})

        	IF(APPLE)
        		set_property(SOURCE scene/tsync/terrasync.cxx PROPERTY COMPILE_FLAGS "-iwithsysroot ${LIBSVN_INCLUDE_DIR}")
        	ELSE()
        		include_directories(${LIBSVN_INCLUDE_DIR})
        	ENDIF(APPLE)
        endif(LIBSVN_FOUND)
    
    	add_library(SimGearScene SHARED ${sceneSources})
      # set_property(TARGET SimGearScene PROPERTY FRAMEWORK 1)
      # set_property(TARGET SimGearScene PROPERTY PUBLIC_HEADER "${publicHeaders}")
        set_property(TARGET SimGearScene PROPERTY LINKER_LANGUAGE CXX)
        
        target_link_libraries(SimGearScene ${ZLIB_LIBRARY} 
    			${OPENSCENEGRAPH_LIBRARIES} 
    			${OPENAL_LIBRARY} ${ALUT_LIBRARY}
    			${OPENGL_LIBRARY})
    	
    	install(TARGETS SimGearScene LIBRARY DESTINATION lib${LIB_SUFFIX})
    endif()
   
endif(SIMGEAR_SHARED)
    
