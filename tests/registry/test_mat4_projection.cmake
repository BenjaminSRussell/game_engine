# Orthographic, perspective, and look-at in mat4_projection.c.
# Not add_engine_test: that macro links Engine, which does not build on
# origin/main (missing GLFW, then gpu_profiler.c).
add_executable(test_mat4_projection
    tests/mat4_projection_test.c
    src/engine/math/linear_algebra/matrices/mat4/mat4_projection.c
)
target_include_directories(test_mat4_projection PRIVATE
    ${CMAKE_SOURCE_DIR}/src/engine/include
    ${CMAKE_SOURCE_DIR}/src/engine
    ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(test_mat4_projection PRIVATE m)
add_test(NAME test_mat4_projection COMMAND test_mat4_projection)
