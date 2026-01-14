# CMakeLists.txt for Deployment Validation Tool
cmake_minimum_required(VERSION 3.16)
project(DeploymentValidation)

# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Compiler flags
if(CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wpedantic -O2")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -DDEBUG")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3 -DNDEBUG")
endif()

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../include)

# Source files
set(VALIDATOR_SOURCES
    deployment_validator.c
    deployment_validation_main.c
)

# Create executable
add_executable(deployment_validator ${VALIDATOR_SOURCES})

# Link libraries
if(UNIX AND NOT APPLE)
    target_link_libraries(deployment_validator pthread m)
elseif(UNIX)
    target_link_libraries(deployment_validator pthread)
endif()

# Install target
install(TARGETS deployment_validator
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

# Custom target for running validation
add_custom_target(run_validation
    COMMAND deployment_validator
    DEPENDS deployment_validator
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running deployment validation checks"
)

# Custom target for generating report
add_custom_target(generate_report
    COMMAND deployment_validator -o validation_report.json -f json
    DEPENDS deployment_validator
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating deployment validation report"
)

# Add tests
enable_testing()

# Test for basic functionality
add_test(NAME validator_basic_test 
         COMMAND deployment_validator --help
         WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

# Test for code quality checks
add_test(NAME validator_code_quality_test 
         COMMAND deployment_validator -c code
         WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

# Test for performance checks
add_test(NAME validator_performance_test 
         COMMAND deployment_validator -c performance
         WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

# Test for compatibility checks
add_test(NAME validator_compatibility_test 
         COMMAND deployment_validator -c compatibility
         WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

# Print configuration
message(STATUS "Deployment Validation Tool Configuration:")
message(STATUS "  C Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "  C Flags: ${CMAKE_C_FLAGS}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Install Prefix: ${CMAKE_INSTALL_PREFIX}")
