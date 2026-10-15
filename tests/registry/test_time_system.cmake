# Issue #387: unit test for src/engine/platform/time_system.c. Compiles the unit directly,
# not add_engine_test, because that macro links Engine, which does not build on main.
add_executable(test_time_system
    tests/unit/platform/time_system_test.c
    src/engine/platform/time_system.c
)
target_include_directories(test_time_system PRIVATE
    ${CMAKE_SOURCE_DIR}/src/engine/include
    ${CMAKE_SOURCE_DIR}/src/engine
    ${CMAKE_SOURCE_DIR}/src
)
add_test(NAME test_time_system COMMAND test_time_system)
