
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

    add_library(SimGearCore STATIC ${coreSources})

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

# for simgear_config.h
target_include_directories(SimGearCore PRIVATE ${PROJECT_BINARY_DIR}/simgear)

target_include_directories(SimGearCore PUBLIC
  ${Boost_INCLUDE_DIRS})

install(TARGETS SimGearCore
        EXPORT SimGearTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

# we expose ZLib in some of our headers
target_link_libraries(SimGearCore PUBLIC ZLIB::ZLIB)

if (ENABLE_RTI)
    target_sources(SimGearCore PRIVATE $<TARGET_OBJECTS:rti> $<TARGET_OBJECTS:rti13>)
    target_link_libraries(SimGearCore PRIVATE PkgConfig::RTI)
endif()

# all of these we keep privately
target_link_libraries(SimGearCore PRIVATE
    ${SHLWAPI_LIBRARY}
    ${RT_LIBRARY}
    ${DL_LIBRARY}
    Threads::Threads
    ${COCOA_LIBRARY}
    CURL::libcurl
    EXPAT::EXPAT
    Udns::Udns
    ${WINSOCK_LIBRARY})

if(NOT SIMGEAR_HEADLESS)
    target_include_directories(SimGearScene PRIVATE ${PROJECT_SOURCE_DIR}/simgear/canvas/ShivaVG/include)

    target_link_libraries(SimGearScene PUBLIC         
        SimGearCore
        ${OPENSCENEGRAPH_LIBRARIES}
    )

    if(HAVE_INTTYPES_H)
        # ShivaVG needs inttypes.h
        target_compile_definitions(SimGearScene PRIVATE HAVE_INTTYPES_H)
    endif()

    target_link_libraries(SimGearScene PRIVATE
        ZLIB::ZLIB
        ${OPENGL_LIBRARY}
        ${JPEG_LIBRARY})

    # ToDo: define an ALIAS target for the sound backend, to get
    # rid of this logic here
    if (ENABLE_SOUND)
        if (USE_AEONWAVE)
            target_link_libraries(SimGearScene PRIVATE ${AAX_LIBRARY})
        else()
            target_link_libraries(SimGearScene PRIVATE ${OPENAL_LIBRARY} )
        endif()
    endif()

    if(ENABLE_GDAL)
        # FIXME: should be RIVATE, but we leak GDAL headers
        # in our own headers :(
        target_link_libraries(SimGearScene PUBLIC GDAL::GDAL)
    endif()

    # only actually needed by canvas/KeyboardEvent.cxx
    target_include_directories(SimGearScene PRIVATE ${PROJECT_SOURCE_DIR}/3rdparty/utf8/source)

    install(TARGETS SimGearScene
        EXPORT SimGearTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

    target_include_directories(SimGearScene BEFORE PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>)

    # for simgear_config.h
    target_include_directories(SimGearScene PRIVATE ${PROJECT_BINARY_DIR}/simgear)
    target_include_directories(SimGearScene PUBLIC ${OPENSCENEGRAPH_INCLUDE_DIRS})

    if (USE_AEONWAVE)
        target_include_directories(SimGearScene PRIVATE ${AAX_INCLUDE_DIR} )
    else()
        target_include_directories(SimGearScene PRIVATE ${OPENAL_INCLUDE_DIR} )
    endif()
endif()
