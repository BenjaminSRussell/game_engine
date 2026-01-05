# Minimum CMake version
cmake_minimum_required(VERSION 3.10)

# Test executable
add_executable(test_runner
  test_runner.c
  test_framework.c
  math/test_vec3.c
  # Add more test files here as they are created
)

# Include directories
target_include_directories(test_runner PRIVATE
  ${CMAKE_SOURCE_DIR}/src/engine/include
  ${CMAKE_SOURCE_DIR}/src/engine/test
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
add_test(NAME MathTests COMMAND test_runner Math)

# Set test properties
set_tests_properties(AllTests PROPERTIES
  TIMEOUT 30
  PASS_REGULAR_EXPRESSION "Passed"
)

# Custom target to run tests
add_custom_target(run_tests
  COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
  DEPENDS test_runner
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  COMMENT "Running unit tests..."
)
