# Translation, scale, and axis / axis-angle rotation matrices.
# Not add_engine_test: that macro links Engine, which does not build
# (missing GLFW, then gpu_profiler.c). Compiles mat4_transform.c directly.
add_executable(test_mat4_transform
    tests/mat4_transform_test.c
    src/engine/math/linear_algebra/matrices/mat4/mat4_transform.c
)
target_include_directories(test_mat4_transform PRIVATE
    ${CMAKE_SOURCE_DIR}/src/engine/include
    ${CMAKE_SOURCE_DIR}/src/engine
    ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(test_mat4_transform PRIVATE m)
set_target_properties(test_mat4_transform PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON)
add_test(NAME test_mat4_transform COMMAND test_mat4_transform)
