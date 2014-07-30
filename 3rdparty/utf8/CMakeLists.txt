include (SimGearComponent)

set(HEADERS
  source/utf8.h
)

set(HEADERS_utf8
  source/utf8/checked.h
  source/utf8/core.h
  source/utf8/unchecked.h
)

set(SOURCES
)

simgear_component(utf8 3rdparty/utf8 "${SOURCES}" "${HEADERS}")
simgear_component(utf8-internal 3rdparty/utf8/utf8 "" "${HEADERS_utf8}")