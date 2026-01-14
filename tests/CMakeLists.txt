cmake_minimum_required(VERSION 3.16)
project(EngineTests C)

# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Enable debugging symbols
set(CMAKE_BUILD_TYPE Debug)

# Compiler flags
if(CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Werror -g -O0")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DDEBUG")
elseif(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W4 /WX /Zi /Od")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /DDEBUG")
endif()

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../src)

# Source files
set(FRAMEWORK_SOURCES
    engine_coverage_test_framework.c
    engine_memory_tests.c
    engine_audio_tests.c
    engine_physics_tests.c
    engine_math_tests.c
)

set(TEST_RUNNER_SOURCES
    engine_test_runner.c
)

# Create object library for framework
add_library(engine_test_framework OBJECT ${FRAMEWORK_SOURCES})

# Create test executable
add_executable(engine_test_runner ${TEST_RUNNER_SOURCES} $<TARGET_OBJECTS:engine_test_framework>)

# Link libraries
target_link_libraries(engine_test_runner 
    pthread
    m  # Math library
)

# Platform-specific libraries
if(UNIX AND NOT APPLE)
    target_link_libraries(engine_test_runner rt)
endif()

# Set output directory
set_target_properties(engine_test_runner PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Custom target for running all tests
add_custom_target(run_tests
    COMMAND engine_test_runner --all --coverage --verbose
    DEPENDS engine_test_runner
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    COMMENT "Running all engine tests with coverage"
)

# Custom target for running stress tests
add_custom_target(run_stress_tests
    COMMAND engine_test_runner --stress --verbose
    DEPENDS engine_test_runner
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    COMMENT "Running engine stress tests"
)

# Custom target for running specific test suites
add_custom_target(run_memory_tests
    COMMAND engine_test_runner --memory --verbose
    DEPENDS engine_test_runner
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    COMMENT "Running memory tests"
)

add_custom_target(run_audio_tests
    COMMAND engine_test_runner --audio --verbose
    DEPENDS engine_test_runner
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    COMMENT "Running audio tests"
)

add_custom_target(run_physics_tests
    COMMAND engine_test_runner --physics --verbose
    DEPENDS engine_test_runner
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    COMMENT "Running physics tests"
)

add_custom_target(run_math_tests
    COMMAND engine_test_runner --math --verbose
    DEPENDS engine_test_runner
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    COMMENT "Running math tests"
)

# Custom target for running integration tests
add_custom_target(run_integration_tests
    COMMAND engine_test_runner --integration --verbose
    DEPENDS engine_test_runner
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    COMMENT "Running integration tests"
)

# Install target
install(TARGETS engine_test_runner
    RUNTIME DESTINATION bin
)

# Create test reports directory
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/test_reports)

# Print configuration
message(STATUS "Engine Tests Configuration:")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "  C Flags: ${CMAKE_C_FLAGS}")
message(STATUS "  Output Directory: ${CMAKE_BINARY_DIR}/bin")
message(STATUS "  Test Reports Directory: ${CMAKE_BINARY_DIR}/test_reports")
