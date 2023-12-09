# Find OSG libraries
# and create imported targets corresponding to each lib


function(find_osg_library retval basename)
    find_library(lib
        NAMES ${basename}
        HINTS
        ENV OSG_DIR
        ENV OSGDIR
        ENV OSG_ROOT
        ${OSG_DIR}
        PATH_SUFFIXES lib
    )

    if (${lib})
        set(retval ${lib} PARENT_SCOPE) 
    else()
        return()
    endif()

    if (MSVC)
        find_library(dll
            NAMES ${basename}
            HINTS
            ENV OSG_DIR
            ENV OSGDIR
            ENV OSG_ROOT
            ${OSG_DIR}
            PATH_SUFFIXES bin
        )

        if (${dll})
            set(${retval}_dll ${dll} PARENT_SCOPE) 
        endif()
    endif()

endfunction()

function(setup_imported_lib target basename debugLibs releaseLibs noconfigLibs)

    find_osg_library(releaseLibs ${basename})
    find_osg_library(debugLibs ${basename}d)
    find_osg_library(reldbgLibs ${basename}rd)

    message(DEBUG "${basename}: debug lib: ${debugLibs}, release lib: ${releaseLibs}")

    # if we didn't find any libs, bail out now
    if (NOT debugLibs AND NOT releaseLibs AND NOT reldbgLibs)
        message(FATAL_ERROR "No libraries found for OSG${basename}")
    endif()

    set(libsList ${releaseLibs} ${debugLibs} ${reldbgLibs})

    message(DEBUG "Libs list is:${libsList}")

    list(LENGTH libsList numConfigs)
    if (numConfigs EQUAL 1)
        message(DEBUG "${basename}: single library configuraation found")
        
        set(libsList ${releaseLibs} ${debugLibs} ${reldbgLibs})

        set_property(TARGET ${target} PROPERTY IMPORTED_IMPLIB ${releaseLibs})
        set_property(TARGET ${target} PROPERTY IMPORTED_LOCATION ${release_dll})

    else()
        message(DEBUG "${basename}: multiple library configuraation found")

        if (releaseLibs)
            set_property(TARGET ${target} APPEND PROPERTY IMPORTED_CONFIGURATIONS Release)
            if (MSVC)
                set_property(TARGET ${target} PROPERTY IMPORTED_IMPLIB_RELEASE ${releaseLibs})
                set_property(TARGET ${target} PROPERTY IMPORTED_LOCATION_RELEASE ${releaseLibs_dll})
            else()
                set_property(TARGET ${target} PROPERTY IMPORTED_LOCATION_RELEASE ${releaseLibs})
            endif()
        endif()

        if (debugLibs)
            set_property(TARGET ${target} APPEND PROPERTY IMPORTED_CONFIGURATIONS Debug)
            if (MSVC)
                set_property(TARGET ${target} PROPERTY IMPORTED_IMPLIB_DEBUG ${debugLibs})
                set_property(TARGET ${target} PROPERTY IMPORTED_LOCATION_DEBUG ${debugLibs_dll})
            else()
                set_property(TARGET ${target} PROPERTY IMPORTED_LOCATION_DEBUG ${debugLibs})
            endif()
        endif()

        if (reldbgLibs)
            set_property(TARGET ${target} APPEND PROPERTY IMPORTED_CONFIGURATIONS RelWithDebInfo)
            if (MSVC)
                set_property(TARGET ${target} PROPERTY IMPORTED_IMPLIB_RELWITHDEBINFO ${reldbgLibs})
                set_property(TARGET ${target} PROPERTY IMPORTED_LOCATION_RELWITHDEBINFO ${debugLibs_dll})
            else()
                set_property(TARGET ${target} PROPERTY IMPORTED_LOCATION_RELWITHDEBINFO ${reldbgLibs})
            endif()
        endif()
    endif()

endfunction()

###############################################################################

set (Simgear_OSG_Components osgText osgSim osgDB osgParticle osgGA osgViewer osgUtil osgTerrain)
find_package(OpenSceneGraph 3.6.0 REQUIRED ${Simgear_OSG_Components})
find_package(OpenThreads REQUIRED)

if (MSVC)
  set(CMAKE_REQUIRED_INCLUDES ${OPENSCENEGRAPH_INCLUDE_DIRS})
  # ensure OSG was compiled with OSG_USE_UTF8_FILENAME set
  check_cxx_source_compiles(
     "#include <osg/Config>
     #if !defined(OSG_USE_UTF8_FILENAME)
      #error OSG UTF8 support not enabled
     #endif
     int main() { return 0; }"
     SIMGEAR_OSG_USE_UTF8_FILENAME)
  if (NOT SIMGEAR_OSG_USE_UTF8_FILENAME)
    message(FATAL_ERROR "Please rebuild OSG with OSG_USE_UTF8_FILENAME set to ON")
  endif()
endif()
 
###############################################################################
# OpenThreads library

add_library(imported_OT SHARED IMPORTED)
set_property(TARGET imported_OT PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${OPENTHREADS_INCLUDE_DIR})

setup_imported_lib(imported_OT OpenThreads "${OPENTHREADS_LIBRARY_DEBUG}" "${OPENTHREADS_LIBRARY_RELEASE}" "${OPENTHREADS_LIBRARY}")
add_library(OSG::OpenThreads ALIAS imported_OT)

###############################################################################
# Core OSG library
add_library(imported_OSG SHARED IMPORTED)

set_property(TARGET imported_OSG PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${OSG_INCLUDE_DIR})
setup_imported_lib(imported_OSG osg "${OSG_LIBRARY_DEBUG}" "${OSG_LIBRARY_RELEASE}" "${OSG_LIBRARY}")
target_link_libraries(imported_OSG INTERFACE OSG::OpenThreads)
add_library(OSG::OSG ALIAS imported_OSG)

###############################################################################
# OSG component libraries

foreach (comp ${Simgear_OSG_Components})
    string(TOUPPER ${comp} _osg_module_UC)
    set(tgt imported_${_osg_module_UC})
    add_library(${tgt} SHARED IMPORTED)
    add_library(OSG::${comp} ALIAS ${tgt})

    set_property(TARGET ${tgt} PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${${_osg_module_UC}_INCLUDE_DIR})

    set(debug_libs ${${_osg_module_UC}_LIBRARY_DEBUG})
    set(release_libs ${${_osg_module_UC}_LIBRARY_RELEASE})

    setup_imported_lib(${tgt} ${comp} ${debug_libs} ${release_libs} ${${_osg_module_UC}_LIBRARY})
    target_link_libraries(${tgt} INTERFACE OSG::OSG)
endforeach()

