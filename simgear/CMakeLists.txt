
file(WRITE ${PROJECT_BINARY_DIR}/simgear/version.h "#define SIMGEAR_VERSION ${SIMGEAR_VERSION}")

foreach( mylibfolder
        bucket
        bvh
        debug
        embedded_resources
        emesary
        ephemeris
        io
        magvar
        math
        misc
        nasal
        nasal/cppbind
        props
        serial
        std
        structure
        threads
        timing
        xml
        package
    )

    add_subdirectory(${mylibfolder})

endforeach( mylibfolder )

if(NOT SIMGEAR_HEADLESS)
    add_subdirectory(canvas)
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

get_property(coreSources GLOBAL PROPERTY CORE_SOURCES)
get_property(sceneSources GLOBAL PROPERTY SCENE_SOURCES)
get_property(publicHeaders GLOBAL PROPERTY PUBLIC_HEADERS)
get_property(localExpatSources GLOBAL PROPERTY LOCAL_EXPAT_SOURCES)

if(SIMGEAR_SHARED)
    message(STATUS "Library building mode: SHARED LIBRARIES")
    add_library(SimGearCore SHARED ${coreSources} ${localExpatSources})

    set_property(TARGET SimGearCore PROPERTY LINKER_LANGUAGE CXX)
    set_property(TARGET SimGearCore PROPERTY VERSION   ${SIMGEAR_VERSION})
    set_property(TARGET SimGearCore PROPERTY SOVERSION ${SIMGEAR_SOVERSION})


    if(NOT SIMGEAR_HEADLESS)
        add_library(SimGearScene SHARED ${sceneSources})
        set_property(TARGET SimGearScene PROPERTY LINKER_LANGUAGE CXX)
        set_property(TARGET SimGearScene PROPERTY VERSION   ${SIMGEAR_VERSION})
        set_property(TARGET SimGearScene PROPERTY SOVERSION ${SIMGEAR_SOVERSION})
    endif()

    export_debug_symbols(SimGearCore)
    export_debug_symbols(SimGearScene)
else()
    message(STATUS "Library building mode: STATIC LIBRARIES")

	get_property(FG_GROUPS_CORE_SOURCES_C GLOBAL PROPERTY FG_GROUPS_CORE_SOURCES_C)
	string(REPLACE "@" ";" groups ${FG_GROUPS_CORE_SOURCES_C} )
	foreach(g ${groups})
		string(REPLACE "#" ";" g2 ${g})
		list(GET g2 0 name)
		list(REMOVE_AT g2 0)
		source_group("${name}\\Sources" FILES ${g2})
	endforeach()

	get_property(FG_GROUPS_CORE_SOURCES_H GLOBAL PROPERTY FG_GROUPS_CORE_SOURCES_H)
	string(REPLACE "@" ";" groups ${FG_GROUPS_CORE_SOURCES_H} )
	foreach(g ${groups})
		string(REPLACE "#" ";" g2 ${g})
		list(GET g2 0 name)
		list(REMOVE_AT g2 0)
		source_group("${name}\\Headers" FILES ${g2})
	endforeach()

    add_library(SimGearCore STATIC ${coreSources} ${localExpatSources})

    if(NOT SIMGEAR_HEADLESS)
		get_property(FG_GROUPS_SCENE_SOURCES_C GLOBAL PROPERTY FG_GROUPS_SCENE_SOURCES_C)
		string(REPLACE "@" ";" groups ${FG_GROUPS_SCENE_SOURCES_C} )
		foreach(g ${groups})
			string(REPLACE "#" ";" g2 ${g})
			list(GET g2 0 name)
			list(REMOVE_AT g2 0)
			source_group("${name}\\Sources" FILES ${g2})
		endforeach()

		get_property(FG_GROUPS_SCENE_SOURCES_H GLOBAL PROPERTY FG_GROUPS_SCENE_SOURCES_H)
		string(REPLACE "@" ";" groups ${FG_GROUPS_SCENE_SOURCES_H} )
		foreach(g ${groups})
			string(REPLACE "#" ";" g2 ${g})
			list(GET g2 0 name)
			list(REMOVE_AT g2 0)
			source_group("${name}\\Headers" FILES ${g2})
		endforeach()

        add_library(SimGearScene STATIC ${sceneSources})
    endif(NOT SIMGEAR_HEADLESS)
endif(SIMGEAR_SHARED)

target_include_directories(SimGearCore BEFORE PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>)

# so simgear/simgear_config.h is found
target_include_directories(SimGearCore BEFORE PUBLIC
    $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}>
    $<INSTALL_INTERFACE:include>)

target_include_directories(SimGearCore PUBLIC
  ${Boost_INCLUDE_DIRS} ${ZLIB_INCLUDE_DIR})
target_include_directories(SimGearCore PRIVATE
  ${EXPAT_INCLUDE_DIRS} ${CURL_INCLUDE_DIRS})

if (NOT SYSTEM_EXPAT)
    # XML_STATIC is important to avoid sg_expat_external.h
    # declaring symbols as declspec(import)
    target_compile_definitions(SimGearCore PRIVATE HAVE_SIMGEAR_EXPAT_CONFIG_H XML_STATIC)
endif()

    install(TARGETS SimGearCore
            EXPORT SimGearTargets
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

if (NOT SIMGEAR_HEADLESS)
  install(TARGETS SimGearScene
          EXPORT SimGearTargets
          LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
          ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

  target_include_directories(SimGearScene BEFORE PUBLIC
      $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>)

    target_include_directories(SimGearScene PUBLIC ${OPENSCENEGRAPH_INCLUDE_DIRS})

    if (USE_AEONWAVE)
      target_include_directories(SimGearScene PRIVATE ${AAX_INCLUDE_DIR} )
    else()
      target_include_directories(SimGearScene PRIVATE ${OPENAL_INCLUDE_DIR} )
    endif()
endif()

# we expose ZLib in some of our headers
target_link_libraries(SimGearCore PUBLIC ${ZLIB_LIBRARY})

target_link_libraries(SimGearCore PRIVATE
    ${RT_LIBRARY}
    ${DL_LIBRARY}
    ${CMAKE_THREAD_LIBS_INIT}
    ${COCOA_LIBRARY}
    ${CURL_LIBRARIES}
    ${WINSOCK_LIBRARY}
    ${SHLWAPI_LIBRARY}
    LibLZMA::LibLZMA
)

if(SYSTEM_EXPAT)
    target_link_libraries(SimGearCore PRIVATE ${EXPAT_LIBRARIES})
endif()

if(ENABLE_DNS AND SYSTEM_UDNS)
    target_link_libraries(SimGearCore PRIVATE ${UDNS_LIBRARIES})
endif()

if(NOT SIMGEAR_HEADLESS)
    target_include_directories(SimGearScene PRIVATE ${PROJECT_SOURCE_DIR}/simgear/canvas/ShivaVG/include)

    target_link_libraries(SimGearScene PUBLIC         
        SimGearCore
        ${OPENSCENEGRAPH_LIBRARIES}
    )

    target_link_libraries(SimGearScene PRIVATE
        ${ZLIB_LIBRARY}
        ${OPENAL_LIBRARY}
        ${OPENGL_LIBRARY}
        ${JPEG_LIBRARY})

    if(ENABLE_GDAL)
        target_link_libraries(SimGearScene PRIVATE ${GDAL_LIBRARIES})
    endif()

    # only actually needed by canvas/KeyboardEvent.cxx
    target_include_directories(SimGearScene PRIVATE ${PROJECT_SOURCE_DIR}/3rdparty/utf8/source)
endif()

if(ENABLE_RTI)
    # Ugly first aid to make hla compile agian
    set_property(SOURCE hla/RTI13InteractionClass.cxx hla/RTI13ObjectClass.cxx
        hla/RTI13ObjectInstance.cxx hla/RTI13Federate.cxx
        hla/RTI13FederateFactory.cxx
        APPEND PROPERTY COMPILE_FLAGS "-I${RTI_INCLUDE_DIRS}")
endif(ENABLE_RTI)
