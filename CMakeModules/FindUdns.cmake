# - Try to find UDNS library
# Once done this will define
#
#  UDNS_FOUND - system has UDNS
#  UDNS_INCLUDE_DIRS - the UDNS include directory
#  UDNS_LIBRARIES - Link these to use UDNS
#  UDNS_DEFINITIONS - Compiler switches required for using UDNS
#
#  Copyright (c) 2016 Maciej Mrozowski <reavertm@gmail.com>
#
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)


if (UDNS_LIBRARIES AND UDNS_INCLUDE_DIRS)
  # in cache already
  set(UDNS_FOUND TRUE)
else ()
  set(UDNS_DEFINITIONS "")

  find_path(UDNS_INCLUDE_DIRS NAMES udns.h)
  find_library(UDNS_LIBRARIES NAMES udns)

  if (UDNS_INCLUDE_DIRS AND UDNS_LIBRARIES)
    set(UDNS_FOUND TRUE)
  endif ()

  if (UDNS_FOUND)
    if (NOT Udns_FIND_QUIETLY)
      message(STATUS "Found UDNS: ${UDNS_LIBRARIES}")
    endif ()
  else ()
    if (Udns_FIND_REQUIRED)
      message(FATAL_ERROR "Could not find UDNS")
    endif ()
  endif ()

  # show the UDNS_INCLUDE_DIRS and UDNS_LIBRARIES variables only in the advanced view
  mark_as_advanced(UDNS_INCLUDE_DIRS UDNS_LIBRARIES)
endif ()

if(UDNS_FOUND)

  if(NOT TARGET Udns::Udns)
    add_library(Udns::Udns UNKNOWN IMPORTED)
    set_target_properties(Udns::Udns PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${UDNS_INCLUDE_DIRS}")

      set_target_properties(Udns::Udns PROPERTIES
        IMPORTED_LINK_INTERFACE_LANGUAGES "C"
        IMPORTED_LOCATION "${UDNS_LIBRARIES}")
    
  endif()
endif()
