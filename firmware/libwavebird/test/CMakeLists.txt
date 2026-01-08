# Include the unity test framework
if(NOT unity_FOUND)
  include(FetchContent)
  FetchContent_Declare(Unity GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity.git)
  FetchContent_MakeAvailable(Unity)
endif()

# Helper function to create a test
function(add_libwavebird_test TEST_NAME)
  # Create the test executable
  add_executable(${TEST_NAME} ${ARGN})

  # Link against the wavebird library and Unity
  target_link_libraries(${TEST_NAME} wavebird unity::framework)

  # Add the test to CTest
  add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endfunction()

# GameCube controller target tests
add_libwavebird_test(test_bch3121 test_bch3121.c)
add_libwavebird_test(test_packet test_packet.c)
