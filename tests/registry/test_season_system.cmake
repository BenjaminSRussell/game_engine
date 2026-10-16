# Issue #250: unit tests for src/engine/environment/season_system.c.
# Not add_engine_test: that macro links Engine, which does not build on
# origin/main (missing GLFW, then gpu_profiler.c:80). The test includes the
# unit's .c file directly, so only the test source is listed here.
add_executable(test_season_system
    tests/unit/environment/season_system_test.c
)
target_include_directories(test_season_system PRIVATE
    ${CMAKE_SOURCE_DIR}/src/engine
)
add_test(NAME test_season_system COMMAND test_season_system)
