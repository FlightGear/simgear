
macro(simgear_component_common name includePath sourcesList sources headers)
    set(fc${sourcesList} ${name})
    set(fh${sourcesList} ${name})
    foreach(s ${sources})
        set_property(GLOBAL
            APPEND PROPERTY ${sourcesList} "${CMAKE_CURRENT_SOURCE_DIR}/${s}")
        set(fc${sourcesList} "${fc${sourcesList}}#${CMAKE_CURRENT_SOURCE_DIR}/${s}")
    endforeach()

	foreach(h ${headers})
		set_property(GLOBAL
			APPEND PROPERTY PUBLIC_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/${h}")
        set(fh${sourcesList} "${fh${sourcesList}}#${CMAKE_CURRENT_SOURCE_DIR}/${h}")

        # also append headers to the sources list, so that IDEs find the files
        # correctly (otherwise they are not in the project)
        set_property(GLOBAL
            APPEND PROPERTY ${sourcesList} "${CMAKE_CURRENT_SOURCE_DIR}/${h}")
	endforeach()

    set_property(GLOBAL APPEND PROPERTY FG_GROUPS_${sourcesList}_C "${fc${sourcesList}}@")
    set_property(GLOBAL APPEND PROPERTY FG_GROUPS_${sourcesList}_H "${fh${sourcesList}}@")
    
    install (FILES ${headers}  DESTINATION include/simgear/${includePath})
endmacro()

function(simgear_component name includePath sources headers)
    simgear_component_common(${name} ${includePath} CORE_SOURCES "${sources}" "${headers}")
endfunction()

function(simgear_scene_component name includePath sources headers)
    simgear_component_common(${name} ${includePath} SCENE_SOURCES "${sources}" "${headers}")
endfunction()
