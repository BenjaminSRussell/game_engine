# Nanite hierarchy grouping and single-LOD selection.
# Not add_engine_test: that macro links Engine, which does not build on
# origin/main (missing GLFW, then gpu_profiler.c). Compiles the CPU side of
# nanite_render directly.
add_executable(test_nanite_render
    tests/nanite_render_test.c
    src/engine/geometry/nanite/nanite_render_cpu.c
)
target_include_directories(test_nanite_render PRIVATE
    ${CMAKE_SOURCE_DIR}/src/engine/include
    ${CMAKE_SOURCE_DIR}/src/engine
    ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(test_nanite_render PRIVATE m)
add_test(NAME test_nanite_render COMMAND test_nanite_render)
