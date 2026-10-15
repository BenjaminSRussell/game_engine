# Instance-buffer and instance-pool stubs kept for test linking.
# Not add_engine_test: that macro links Engine and
# tests/unit/test_framework_unified.c, which is missing. Engine does not
# build on origin/main (missing GLFW, then gpu_profiler.c).
add_executable(test_instance_buffer_stubs
    tests/instance_buffer_stubs_test.c
    src/engine/geometry/instancing/instance_buffer_stubs.c
)
target_include_directories(test_instance_buffer_stubs PRIVATE
    ${CMAKE_SOURCE_DIR}/src/engine/include
    ${CMAKE_SOURCE_DIR}/src/engine
    ${CMAKE_SOURCE_DIR}/src
)
add_test(NAME test_instance_buffer_stubs COMMAND test_instance_buffer_stubs)
