# Minimum CMake version
cmake_minimum_required(VERSION 3.10)

# Include directories for tests
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)

# Test executable
add_executable(test_runner
  test_runner.c
  test_framework.c
  core/test_memory.c
  math/test_vec3.c  # Existing file, keeping it
  unit/ecs/test_ecs.c
  # Add more test files here as they are created
)

# Link against engine library
target_link_libraries(test_runner PRIVATE
  Engine
  m  # Math library
)

# Enable testing
enable_testing()

# Add test
add_test(NAME AllTests COMMAND test_runner)

# Custom target to run tests
add_custom_target(run_tests
  COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
  DEPENDS test_runner
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/src/engine/test
  COMMENT "Running unit tests..."
)
