
macro(simgear_component name includePath sources headers)

    if (SIMGEAR_SHARED)
        foreach(s ${sources})
            set_property(GLOBAL
                APPEND PROPERTY ALL_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/${s}")
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
