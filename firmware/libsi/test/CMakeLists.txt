# Include the unity test framework
if(NOT unity_FOUND)
  include(FetchContent)
  FetchContent_Declare(Unity GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity.git)
  FetchContent_MakeAvailable(Unity)
endif()

# Define the test and set the sources
add_executable(test_si "test_main.c" "test_commands.c" "test_gc_controller.c")

# Link dependencies
target_link_libraries(test_si si unity::framework)