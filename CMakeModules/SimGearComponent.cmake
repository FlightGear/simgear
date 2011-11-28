
macro(simgear_component_common name includePath sourcesList sources headers)
    if (SIMGEAR_SHARED)

        foreach(s ${sources})
            set_property(GLOBAL
                APPEND PROPERTY ${sourcesList} "${CMAKE_CURRENT_SOURCE_DIR}/${s}")
        endforeach()

		foreach(h ${headers})
			set_property(GLOBAL
				APPEND PROPERTY PUBLIC_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/${h}")
		endforeach()
        
    else()
        set(libName "sg${name}")
        add_library(${libName} STATIC ${sources} ${headers})

        install (TARGETS ${libName} ARCHIVE DESTINATION lib${LIB_SUFFIX})
        install (FILES ${headers}  DESTINATION include/simgear/${includePath})
    endif()
endmacro()

function(simgear_component name includePath sources headers)
    simgear_component_common(${name} ${includePath} CORE_SOURCES "${sources}" "${headers}")
endfunction()

function(simgear_scene_component name includePath sources headers)
    simgear_component_common(${name} ${includePath} SCENE_SOURCES "${sources}" "${headers}")
endfunction()
