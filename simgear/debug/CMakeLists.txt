
include (SimGearComponent)

set(HEADERS debug_types.h 
    logstream.hxx BufferedLogCallback.hxx OsgIoCapture.hxx
    LogCallback.hxx LogEntry.hxx
    ErrorReportingCallback.hxx)

set(SOURCES logstream.cxx BufferedLogCallback.cxx
    LogCallback.cxx LogEntry.cxx
    ErrorReportingCallback.cxx
    )

simgear_component(debug debug "${SOURCES}" "${HEADERS}")
