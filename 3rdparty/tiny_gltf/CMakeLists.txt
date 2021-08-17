add_library(FGTinyGLTF INTERFACE)

target_include_directories(FGTinyGLTF
  INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:include>)

# in the non-shared case, we need to export these as well
if (NOT SIMGEAR_SHARED)
  install(TARGETS FGTinyGLTF
    EXPORT SimGearTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()
