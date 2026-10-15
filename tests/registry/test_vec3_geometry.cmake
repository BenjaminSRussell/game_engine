# Issue #369: Vec3 angle, slerp, reflect, and projection helpers.
# Not add_engine_test: that macro links Engine and
# tests/unit/test_framework_unified.c, which is missing. Engine does not
# build on origin/main (missing GLFW, then gpu_profiler.c).
add_executable(test_vec3_geometry
    tests/vec3_geometry_test.c
    src/engine/math/linear_algebra/vectors/vec3/vec3_geometry.c
)
target_include_directories(test_vec3_geometry PRIVATE
    ${CMAKE_SOURCE_DIR}/src/engine/include
    ${CMAKE_SOURCE_DIR}/src/engine
    ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(test_vec3_geometry PRIVATE m)
add_test(NAME test_vec3_geometry COMMAND test_vec3_geometry)
