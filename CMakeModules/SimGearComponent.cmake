
macro(simgear_component name includePath sources header)

    if (${SIMGEAR_SHARED})
        foreach(s ${sources})
            set_property(DIRECTORY "${PROJECT_SOURCE_DIR}/simgear" 
                APPEND PROPERTY ALL_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/${s}")
        endforeach()
        
    else()
        set(libName "sg${name}")
        add_library(${libName} STATIC ${sources} ${headers} )

        install (TARGETS ${libName} ARCHIVE DESTINATION lib)
        install (FILES ${headers}  DESTINATION include/simgear/${includePath})
    endif()
    
endmacro()
