# Issue #353: cube, sphere, cylinder, plane, cone, and torus generators.
# Not add_engine_test: that macro links Engine, which does not build on
# origin/main (missing GLFW, then gpu_profiler.c). Compiles mesh_primitives.c
# directly.
add_executable(test_mesh_primitives
    tests/mesh_primitives_test.c
    src/engine/geometry/mesh_primitives.c
)
target_include_directories(test_mesh_primitives PRIVATE
    ${CMAKE_SOURCE_DIR}/src/engine/include
    ${CMAKE_SOURCE_DIR}/src/engine
    ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(test_mesh_primitives PRIVATE m)
add_test(NAME test_mesh_primitives COMMAND test_mesh_primitives)
