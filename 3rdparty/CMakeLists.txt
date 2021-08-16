if (NOT SYSTEM_EXPAT)
	add_subdirectory(expat)
endif()

if (NOT SYSTEM_UDNS)
	add_subdirectory(udns)
endif()

add_subdirectory(tiny_gltf)
