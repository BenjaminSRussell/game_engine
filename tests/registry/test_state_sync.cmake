# Issue #383: register, snapshot, apply, interpolate, predict, and delta
# round-trip in src/engine/network/state_sync.c. Not add_engine_test: that
# macro links Engine and tests/unit/test_framework_unified.c, which is not in
# the tree. Engine does not build on origin/main (missing GLFW, then
# gpu_profiler.c). Compiles state_sync.c directly.
add_executable(test_state_sync
    tests/state_sync_test.c
    src/engine/network/state_sync.c
)
target_include_directories(test_state_sync PRIVATE
    ${CMAKE_SOURCE_DIR}/src/engine/network
)
target_compile_definitions(test_state_sync PRIVATE _POSIX_C_SOURCE=200809L)
target_link_libraries(test_state_sync PRIVATE m Threads::Threads)
add_test(NAME test_state_sync COMMAND test_state_sync)
