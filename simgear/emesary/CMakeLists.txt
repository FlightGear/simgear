

include (SimGearComponent)

set(HEADERS 
    Emesary.hxx
    INotification.hxx
    IReceiver.hxx
    ITransmitter.hxx
    ReceiptStatus.hxx
    Transmitter.hxx
    notifications.hxx
    )
    
set(SOURCES 
    Emesary.cxx
    )

simgear_component(emesary emesary "${SOURCES}" "${HEADERS}")


if(ENABLE_TESTS)

add_executable(test_emesary test_emesary.cxx)

set_target_properties(test_emesary PROPERTIES
  COMPILE_DEFINITIONS "SRC_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}\"" )

target_link_libraries(test_emesary ${TEST_LIBS})
add_test(emesary ${EXECUTABLE_OUTPUT_PATH}/test_emesary)


endif(ENABLE_TESTS)
