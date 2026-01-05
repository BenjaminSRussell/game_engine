# Fetch Catch2 for unit testing

# Set C++ standard before fetching Catch2
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.5.0
)

set(CATCH_BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(CATCH_INSTALL_DOCS OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(Catch2)

# Enable testing
enable_testing()

# Helper macro to add test
macro(add_unit_test name source)
    add_executable(${name} ${source})
    target_link_libraries(${name} PRIVATE Engine Catch2::Catch2WithMain)
    target_include_directories(${name} PRIVATE
        ${CMAKE_SOURCE_DIR}/src/engine/include
        ${CMAKE_SOURCE_DIR}/src/engine
    )
    add_test(NAME ${name} COMMAND ${name})
endmacro()

# ECS Tests
add_unit_test(test_ecs_archetype ecs/test_archetype_system.cpp)

# Memory Tests
add_unit_test(test_linear_allocator memory/test_linear_allocator.cpp)
add_unit_test(test_buddy_allocator memory/test_buddy_allocator.cpp)

# Physics Tests
add_unit_test(test_rigid_body physics/test_rigid_body.cpp)

message(STATUS "Unit tests configured with Catch2")
