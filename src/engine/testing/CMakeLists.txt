# Test Framework CMake Configuration

cmake_minimum_required(VERSION 3.16)
project(MinecraftEngineTests)

# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../core
    ${CMAKE_CURRENT_SOURCE_DIR}/../math
    ${CMAKE_CURRENT_SOURCE_DIR}/../rendering
)

# Find required packages
find_package(PkgConfig REQUIRED)

# Test framework executable
add_executable(test_framework
    test_framework.c
    test_core_systems.c
    run_tests.c
)

# Link with core libraries
target_link_libraries(test_framework
    # Add any required libraries here
)

# Enable testing
enable_testing()

# Add test cases
add_test(NAME CoreSystems COMMAND test_framework --suite CoreSystems)
add_test(NAME RenderingTests COMMAND test_framework --suite RenderingTests)
add_test(NAME PhysicsTests COMMAND test_framework --suite PhysicsTests)
add_test(NAME AudioTests COMMAND test_framework --suite AudioTests)
add_test(NAME NetworkTests COMMAND test_framework --suite NetworkTests)
add_test(NAME AnimationTests COMMAND test_framework --suite AnimationTests)
add_test(NAME AssetTests COMMAND test_framework --suite AssetTests)

# Run all tests
add_test(NAME AllTests COMMAND test_framework)

# Set test properties
set_tests_properties(AllTests PROPERTIES
    PASS_REGULAR_EXPRESSION "🎉 All tests passed"
    FAIL_REGULAR_EXPRESSION "❌ Tests failed"
)

# Coverage configuration
if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo"))
    # Enable coverage for debug builds
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        target_compile_options(test_framework PRIVATE --coverage)
        target_link_options(test_framework PRIVATE --coverage)
    endif()
endif()

# Compiler flags
target_compile_options(test_framework PRIVATE
    -Wall
    -Wextra
    -Werror
    -g
    -O0
)

# Define test macros
target_compile_definitions(test_framework PRIVATE
    TESTING=1
)
