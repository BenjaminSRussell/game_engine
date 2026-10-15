# LOD edge collapse remaps deleted vertices and honors generation config.
# Not add_engine_test: that macro links Engine and
# tests/unit/test_framework_unified.c, which is missing. Engine does not
# build on origin/main (missing GLFW, then gpu_profiler.c).
add_executable(test_lod_generator
    tests/lod_generator_test.c
    src/engine/geometry/lod/lod_generator.c
    src/engine/geometry/mesh.c
)
target_include_directories(test_lod_generator PRIVATE
    ${CMAKE_SOURCE_DIR}/src/engine/include
    ${CMAKE_SOURCE_DIR}/src/engine
    ${CMAKE_SOURCE_DIR}/src
)
add_test(NAME test_lod_generator COMMAND test_lod_generator)
