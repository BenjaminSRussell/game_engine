# GPU compute culling lifecycle, dirty processing, and memory accounting.
# Not add_engine_test: that macro links Engine, which does not build on
# origin/main (missing GLFW, then gpu_profiler.c). Compiles
# gpu_cull_compute.c directly.
add_executable(test_gpu_cull_compute
    tests/gpu_cull_compute_test.c
    src/engine/geometry/culling/gpu_culling/gpu_cull_compute.c
)
target_include_directories(test_gpu_cull_compute PRIVATE
    ${CMAKE_SOURCE_DIR}/src/engine/include
    ${CMAKE_SOURCE_DIR}/src/engine
    ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(test_gpu_cull_compute PRIVATE Threads::Threads)
add_test(NAME test_gpu_cull_compute COMMAND test_gpu_cull_compute)
