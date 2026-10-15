# Issue #252: unit tests for gpu_profiler marker recording.
# Not add_engine_test: that macro links Engine, which does not build on origin/main.
# Compiles the unit directly, as test_vulkan_shader_fallback does.
add_executable(test_gpu_profiler
    tests/unit/profiling/gpu_profiler_test.c
    src/engine/core/profiling/gpu_profiler.c
)
target_include_directories(test_gpu_profiler PRIVATE
    ${CMAKE_SOURCE_DIR}/src/engine/include
    ${CMAKE_SOURCE_DIR}/src/engine
    ${CMAKE_SOURCE_DIR}/src
)
add_test(NAME test_gpu_profiler COMMAND test_gpu_profiler)
