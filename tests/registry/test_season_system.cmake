# Five-minute seasons, init reset, and leftover time carried into the next season.
# Not add_engine_test: that macro links Engine, which does not build
# (missing GLFW, then gpu_profiler.c). Compiles season_system.c directly.
add_executable(test_season_system
    tests/season_system_test.c
    src/engine/environment/season_system.c
)
target_include_directories(test_season_system PRIVATE
    ${CMAKE_SOURCE_DIR}/src/engine/environment
)
add_test(NAME test_season_system COMMAND test_season_system)
