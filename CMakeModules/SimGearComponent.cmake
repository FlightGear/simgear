
macro(simgear_component_common name includePath sourcesList sources headers)
    foreach(s ${sources})
        set_property(GLOBAL
            APPEND PROPERTY ${sourcesList} "${CMAKE_CURRENT_SOURCE_DIR}/${s}")
    endforeach()

	foreach(h ${headers})
		set_property(GLOBAL
			APPEND PROPERTY PUBLIC_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/${h}")
	endforeach()
    
    install (FILES ${headers}  DESTINATION include/simgear/${includePath})
endmacro()

function(simgear_component name includePath sources headers)
    simgear_component_common(${name} ${includePath} CORE_SOURCES "${sources}" "${headers}")
endfunction()

function(simgear_scene_component name includePath sources headers)
    simgear_component_common(${name} ${includePath} SCENE_SOURCES "${sources}" "${headers}")
endfunction()
