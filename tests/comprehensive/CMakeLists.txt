# Comprehensive Game Engine Test Suite - CMake Configuration

cmake_minimum_required(VERSION 3.15)

# Test framework library
add_library(test_framework STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/test_framework_unified.c
)

target_include_directories(test_framework PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src/engine/include
)

# Comprehensive test executable (all tests)
add_executable(run_all_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/comprehensive/run_all_tests.c
    ${CMAKE_CURRENT_SOURCE_DIR}/comprehensive/test_rendering_complete.c
    ${CMAKE_CURRENT_SOURCE_DIR}/comprehensive/test_physics_complete.c
    ${CMAKE_CURRENT_SOURCE_DIR}/comprehensive/test_animation_complete.c
    ${CMAKE_CURRENT_SOURCE_DIR}/comprehensive/test_audio_complete.c
    ${CMAKE_CURRENT_SOURCE_DIR}/comprehensive/test_ecs_complete.c
    ${CMAKE_CURRENT_SOURCE_DIR}/comprehensive/test_ai_complete.c
    ${CMAKE_CURRENT_SOURCE_DIR}/comprehensive/test_networking_complete.c
    ${CMAKE_CURRENT_SOURCE_DIR}/comprehensive/test_stubs.c
    # Unreal Engine Parity Tests
    ${CMAKE_CURRENT_SOURCE_DIR}/comprehensive/ue_parity_stubs.c
    ${CMAKE_CURRENT_SOURCE_DIR}/comprehensive/test_ue_nanite_lumen.c
    ${CMAKE_CURRENT_SOURCE_DIR}/comprehensive/test_ue_chaos.c
    ${CMAKE_CURRENT_SOURCE_DIR}/comprehensive/test_ue_niagara.c
    ${CMAKE_CURRENT_SOURCE_DIR}/comprehensive/test_ue_gas.c
    ${CMAKE_CURRENT_SOURCE_DIR}/comprehensive/test_ue_world.c
    ${CMAKE_CURRENT_SOURCE_DIR}/comprehensive/test_ue_ai_systems.c
    ${CMAKE_CURRENT_SOURCE_DIR}/comprehensive/test_ue_media.c
)

target_link_libraries(run_all_tests PRIVATE
    test_framework
    GameEngine
    m  # Math library
)

target_include_directories(run_all_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src/engine/include
)

# Individual test suite executables
# These allow running specific subsystems in isolation

add_executable(test_rendering_suite
    ${CMAKE_CURRENT_SOURCE_DIR}/comprehensive/test_rendering_complete.c
    ${CMAKE_CURRENT_SOURCE_DIR}/test_framework_unified.c
)
target_link_libraries(test_rendering_suite PRIVATE GameEngine m)

add_executable(test_physics_suite
    ${CMAKE_CURRENT_SOURCE_DIR}/comprehensive/test_physics_complete.c
    ${CMAKE_CURRENT_SOURCE_DIR}/test_framework_unified.c
)
target_link_libraries(test_physics_suite PRIVATE GameEngine m)

add_executable(test_animation_suite
    ${CMAKE_CURRENT_SOURCE_DIR}/comprehensive/test_animation_complete.c
    ${CMAKE_CURRENT_SOURCE_DIR}/test_framework_unified.c
)
target_link_libraries(test_animation_suite PRIVATE GameEngine m)

add_executable(test_audio_suite
    ${CMAKE_CURRENT_SOURCE_DIR}/comprehensive/test_audio_complete.c
    ${CMAKE_CURRENT_SOURCE_DIR}/test_framework_unified.c
)
target_link_libraries(test_audio_suite PRIVATE GameEngine m)

add_executable(test_ecs_suite
    ${CMAKE_CURRENT_SOURCE_DIR}/comprehensive/test_ecs_complete.c
    ${CMAKE_CURRENT_SOURCE_DIR}/test_framework_unified.c
)
target_link_libraries(test_ecs_suite PRIVATE GameEngine m)

add_executable(test_ai_suite
    ${CMAKE_CURRENT_SOURCE_DIR}/comprehensive/test_ai_complete.c
    ${CMAKE_CURRENT_SOURCE_DIR}/test_framework_unified.c
)
target_link_libraries(test_ai_suite PRIVATE GameEngine m)

add_executable(test_networking_suite
    ${CMAKE_CURRENT_SOURCE_DIR}/comprehensive/test_networking_complete.c
    ${CMAKE_CURRENT_SOURCE_DIR}/test_framework_unified.c
)
target_link_libraries(test_networking_suite PRIVATE GameEngine m)

# CTest integration
enable_testing()

add_test(NAME AllTests COMMAND run_all_tests)
add_test(NAME RenderingTests COMMAND run_all_tests --suite Rendering)
add_test(NAME PhysicsTests COMMAND run_all_tests --suite Physics)
add_test(NAME AnimationTests COMMAND run_all_tests --suite Animation)
add_test(NAME AudioTests COMMAND run_all_tests --suite Audio)
add_test(NAME ECSTests COMMAND run_all_tests --suite ECS)
add_test(NAME AITests COMMAND run_all_tests --suite AI)
add_test(NAME NetworkingTests COMMAND run_all_tests --suite Networking)

# Custom target to run all tests
add_custom_target(test_all
    COMMAND ${CMAKE_BINARY_DIR}/tests/run_all_tests
    DEPENDS run_all_tests
    COMMENT "Running comprehensive test suite"
)

# Test summary message
message(STATUS "")
message(STATUS "═══════════════════════════════════════════════════")
message(STATUS "Comprehensive Test Suite Configured")
message(STATUS "═══════════════════════════════════════════════════")
message(STATUS "Run all tests: cmake --build . --target test_all")
message(STATUS "Run via CTest: ctest --output-on-failure")
message(STATUS "═══════════════════════════════════════════════════")
message(STATUS "")
