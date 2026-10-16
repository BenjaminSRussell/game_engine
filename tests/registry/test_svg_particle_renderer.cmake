# Issue #333: unit test for svg_particle_renderer.c. Not add_engine_test: that macro links
# Engine, which does not build on origin/main (missing GLFW, then gpu_profiler.c). The test
# includes the renderer directly and compiles svg_particle_system.c alongside it.
add_executable(test_svg_particle_renderer
    tests/unit/effects/svg_particle_renderer_test.c
    src/engine/effects/svg_particles/svg_particle_system.c
)
target_include_directories(test_svg_particle_renderer PRIVATE
    ${CMAKE_SOURCE_DIR}/src/engine/include
    ${CMAKE_SOURCE_DIR}/src/engine
    ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(test_svg_particle_renderer PRIVATE m)
add_test(NAME test_svg_particle_renderer COMMAND test_svg_particle_renderer)
