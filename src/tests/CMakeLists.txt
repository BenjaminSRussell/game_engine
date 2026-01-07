cmake_minimum_required(VERSION 3.15)

# Rendering Pipeline Tests
add_executable(test_rendering_pipeline
    rendering_pipeline_tests.c
)

# Test compilation flags
target_compile_options(test_rendering_pipeline PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -std=c99
    -O2
)

# Link required libraries
target_link_libraries(test_rendering_pipeline PRIVATE
    m  # Math library
)

# Set output directory
set_target_properties(test_rendering_pipeline PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/tests"
)

# Register test
enable_testing()
add_test(
    NAME rendering_pipeline
    COMMAND test_rendering_pipeline
)

# Print test info
message(STATUS "Added test: test_rendering_pipeline")
