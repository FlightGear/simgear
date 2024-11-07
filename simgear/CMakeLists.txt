
file(WRITE ${PROJECT_BINARY_DIR}/simgear/version.h "#define SIMGEAR_VERSION ${SIMGEAR_VERSION}")

if(SIMGEAR_SHARED)
    add_library(SimGearCore SHARED ${localExpatSources})

    set_property(TARGET SimGearCore PROPERTY LINKER_LANGUAGE CXX)
    set_property(TARGET SimGearCore PROPERTY VERSION   ${SIMGEAR_VERSION})
    set_property(TARGET SimGearCore PROPERTY SOVERSION ${SIMGEAR_SOVERSION})

    if (NOT SIMGEAR_HEADLESS)
        add_library(SimGearScene SHARED ${sceneSources})
        set_property(TARGET SimGearScene PROPERTY LINKER_LANGUAGE CXX)
        set_property(TARGET SimGearScene PROPERTY VERSION   ${SIMGEAR_VERSION})
        set_property(TARGET SimGearScene PROPERTY SOVERSION ${SIMGEAR_SOVERSION})
    endif()

    export_debug_symbols(SimGearCore)
    export_debug_symbols(SimGearScene)
else()
    add_library(SimGearCore STATIC)
    if (NOT SIMGEAR_HEADLESS)
        add_library(SimGearScene STATIC)
    endif()
endif(SIMGEAR_SHARED)

foreach( mylibfolder
        bucket
        bvh
        canvas
        debug
        embedded_resources
        emesary
        environment
        ephemeris
        io
        magvar
        math
        misc
        nasal
        nasal/cppbind
        props
        scene
        screen
        serial
        sound
        std
        structure
        threads
        timing
        xml
        package

    )

    add_subdirectory(${mylibfolder})
endforeach( mylibfolder )

if(ENABLE_RTI)
    add_subdirectory(hla)
endif(ENABLE_RTI)

set(HEADERS compiler.h constants.h sg_inlines.h ${PROJECT_BINARY_DIR}/simgear/version.h)
install (FILES ${HEADERS}  DESTINATION include/simgear/)

target_include_directories(SimGearCore BEFORE PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>)

target_precompile_headers(SimGearCore PRIVATE
        "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/simgear_precompile.h>")
# for simgear_config.h
target_include_directories(SimGearCore PRIVATE ${PROJECT_BINARY_DIR}/simgear)

# we leak Boost in our public APIs :(
target_include_directories(SimGearCore PUBLIC ${Boost_INCLUDE_DIRS})

install(TARGETS SimGearCore
        EXPORT SimGearTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

# we expose ZLib in some of our headers
target_link_libraries(SimGearCore PUBLIC ZLIB::ZLIB)

if (SG_HAVE_DDS)
    target_link_libraries(SimGearCore PRIVATE CycloneDDS::ddsc)
endif()

if (ENABLE_RTI)
    target_sources(SimGearCore PRIVATE $<TARGET_OBJECTS:rti> $<TARGET_OBJECTS:rti13>)
    target_link_libraries(SimGearCore PRIVATE PkgConfig::RTI)
endif()


if (APPLE)
    # PCH doesn't work for Objective-C++
    set_property(SOURCE misc/CocoaHelpers.mm PROPERTY SKIP_PRECOMPILE_HEADERS ON)
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
    LibLZMA::LibLZMA
    ${WINSOCK_LIBRARY})

if(TARGET c-ares::cares)
    target_link_libraries(SimGearCore PRIVATE c-ares::cares)
else()
    target_link_libraries(SimGearCore PRIVATE Udns::Udns)
endif()

if(NOT SIMGEAR_HEADLESS)
    # On Windows FindOpenGL only gives us support for OpenGL 1.2, so we need to
    # manually include a more modern header.
    # OpenGL is only used by ShaderVG.
    if(WIN32)
        target_include_directories(SimGearScene BEFORE PUBLIC 
            $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/3rdparty/glcore>
        )
    endif()

    target_include_directories(SimGearScene PRIVATE ${PROJECT_SOURCE_DIR}/simgear/canvas/ShaderVG/include)

    target_link_libraries(SimGearScene PUBLIC
        SimGearCore
        OSG::OSG
        OSG::osgText
        OSG::osgDB
        OSG::osgSim
        OSG::osgGA
        OSG::osgUtil
        OSG::osgTerrain
        OSG::osgParticle
        OSG::osgViewer
    )

    target_precompile_headers(SimGearScene PRIVATE
        "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/simgear_precompile.h>"
        "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/simgear_scene_precompile.h>"
    )

    if(HAVE_INTTYPES_H)
        # ShaderVG needs inttypes.h
        target_compile_definitions(SimGearScene PRIVATE HAVE_INTTYPES_H)
    endif()

    target_link_libraries(SimGearScene PRIVATE
        ZLIB::ZLIB
        ${OPENGL_LIBRARY}
        ${JPEG_LIBRARY}
        ${FFMPEG_LIBRARIES}
        FGTinyGLTF)

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
        # FIXME: should be PRIVATE, but we leak GDAL headers
        # in our own headers :(
        target_link_libraries(SimGearScene PUBLIC GDAL::GDAL)
    endif()

    # only actually needed by canvas/KeyboardEvent.cxx
    target_include_directories(SimGearScene PRIVATE ${PROJECT_SOURCE_DIR}/3rdparty/utf8/source)
    target_include_directories(SimGearScene PRIVATE ${FFMPEG_INCLUDE_DIRS})

    install(TARGETS SimGearScene
        EXPORT SimGearTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

    target_include_directories(SimGearScene BEFORE PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>)

    # for simgear_config.h
    target_include_directories(SimGearScene PRIVATE ${PROJECT_BINARY_DIR}/simgear)
    #target_include_directories(SimGearScene SYSTEM PUBLIC ${OPENSCENEGRAPH_INCLUDE_DIRS})

    if (USE_AEONWAVE)
        target_include_directories(SimGearScene PRIVATE ${AAX_INCLUDE_DIR} )
    else()
        target_include_directories(SimGearScene PRIVATE ${OPENAL_INCLUDE_DIR} )
    endif()
endif()
