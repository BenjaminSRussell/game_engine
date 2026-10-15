# Issue #192: mesh_create_cube must compute real tangents, not the {0,0,0,1}
# placeholder. Not add_engine_test: that macro links Engine, which does not
# build on origin/main (missing GLFW, then gpu_profiler.c). Compiles
# mesh_primitives.c directly.
add_executable(test_mesh_primitives_tangent
    tests/unit/geometry/mesh_primitives_tangent_test.c
    src/engine/geometry/mesh_primitives.c
)
target_include_directories(test_mesh_primitives_tangent PRIVATE
    ${CMAKE_SOURCE_DIR}/src/engine
    ${CMAKE_SOURCE_DIR}/src/engine/include
    ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(test_mesh_primitives_tangent PRIVATE m)
add_test(NAME test_mesh_primitives_tangent COMMAND test_mesh_primitives_tangent)
