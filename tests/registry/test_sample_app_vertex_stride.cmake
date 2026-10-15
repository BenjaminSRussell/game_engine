# Issue #444: sample app vertex-buffer stride must match the Metal vertex
# descriptor. Not add_engine_test: that macro links Engine, which does not
# build on origin/main (missing GLFW, then gpu_profiler.c).
add_executable(test_sample_app_vertex_stride
    tests/sample_app_vertex_stride_test.c
)
target_include_directories(test_sample_app_vertex_stride PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)
target_compile_definitions(test_sample_app_vertex_stride PRIVATE
    "SAMPLE_APP_M=\"${CMAKE_SOURCE_DIR}/src/sample_app.m\""
)
set_target_properties(test_sample_app_vertex_stride PROPERTIES C_STANDARD 11)
add_test(NAME test_sample_app_vertex_stride COMMAND test_sample_app_vertex_stride)
