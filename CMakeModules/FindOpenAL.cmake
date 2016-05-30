#  Copyright Matus Chochlik.
#  Distributed under the Boost Software License, Version 1.0.
#  See accompanying file LICENSE_1_0.txt or copy at
#   http://www.boost.org/LICENSE_1_0.txt
#
#  2016-05-30: Updated for FlightGear --EMH--
#
unset(OPENAL_INCLUDE_DIRS)
set(OPENAL_FOUND 0)
#
# try to find AL/al.h
find_path(
	OPENAL_AL_H_DIR AL/al.h
	PATHS ${HEADER_SEARCH_PATHS}
	NO_DEFAULT_PATH
)
# if that didn't work try the system directories
if((NOT OPENAL_AL_H_DIR) OR (NOT EXISTS ${OPENAL_AL_H_DIR}))
	find_path(OPENAL_AL_H_DIR AL/al.h)
endif()
# if found append it to the include directories
if((OPENAL_AL_H_DIR) AND (EXISTS ${OPENAL_AL_H_DIR}))
	set(OPENAL_INCLUDE_DIRS ${OPENAL_INCLUDE_DIRS} ${OPENAL_AL_H_DIR})
	set(HAVE_AL_H true)
	set(OPENAL_FOUND 1)
endif()
#
#
# try to find AL/alext.h
find_path(
	OPENAL_ALEXT_H_DIR AL/alext.h
	PATHS ${HEADER_SEARCH_PATHS}
	NO_DEFAULT_PATH
)
# if that didn't work try the system directories
if((NOT OPENAL_ALEXT_H_DIR) OR (NOT EXISTS ${OPENAL_ALEXT_H_DIR}))
	find_path(OPENAL_ALEXT_H_DIR AL/alext.h)
endif()
#
if((OPENAL_ALEXT_H_DIR) AND (EXISTS ${OPENAL_ALEXT_H_DIR}))
	set(OPENAL_INCLUDE_DIRS ${OPENAL_INCLUDE_DIRS} ${OPENAL_ALEXT_H_DIR})
	set(HAVE_ALEXT_H true)
endif()

# try to find the AL library
find_library(
	OPENAL_LIBRARY NAMES openal
	PATHS ${LIBRARY_SEARCH_PATHS}
	NO_DEFAULT_PATH
)
if(NOT OPENAL_LIBRARY)
	find_library(OPENAL_LIBRARY NAMES openal)
endif()

set(OPENAL_LIBRARIES "")
if(OPENAL_LIBRARY AND EXISTS ${OPENAL_LIBRARY})
	set(OPENAL_LIBRARIES ${OPENAL_LIBRARIES} ${OPENAL_LIBRARY})
endif()

