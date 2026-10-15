# Issue #190: mesh_optimize_vertex_cache reorders triangles for the GPU vertex cache.
# Compiles the unit directly because Engine does not build on this host. The unit
# uses sqrtf, so the test links libm.
add_executable(test_mesh_vertex_cache
    tests/unit/rendering/mesh_optimize_vertex_cache_test.c
    src/engine/rendering/core/mesh.c)
target_include_directories(test_mesh_vertex_cache PRIVATE
    src/engine/include
    src/engine
    src)
target_link_libraries(test_mesh_vertex_cache PRIVATE m)
add_test(NAME test_mesh_vertex_cache COMMAND test_mesh_vertex_cache)
