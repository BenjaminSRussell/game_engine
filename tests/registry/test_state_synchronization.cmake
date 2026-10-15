# Entity state sync: register, update, snapshot, apply, and bandwidth.
# Not add_engine_test: that macro links Engine and
# tests/unit/test_framework_unified.c, which is missing. Engine does not
# build on origin/main (missing GLFW, then gpu_profiler.c).
add_executable(test_state_synchronization
    tests/state_synchronization_test.c
    src/engine/network/state/state_synchronization.c
)
target_include_directories(test_state_synchronization PRIVATE
    ${CMAKE_SOURCE_DIR}/src/engine/include
    ${CMAKE_SOURCE_DIR}/src/engine
    ${CMAKE_SOURCE_DIR}/src
)
add_test(NAME test_state_synchronization COMMAND test_state_synchronization)

