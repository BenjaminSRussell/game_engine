# Shadow-cascade map resolution, texel density, and quality presets.
# Not add_engine_test: that macro links Engine, which does not build on
# origin/main (missing GLFW, then gpu_profiler.c). Compiles
# cascade_resolution.c directly.
add_executable(test_cascade_resolution
    tests/cascade_resolution_test.c
    src/engine/lighting/cascades/cascade_resolution.c
)
target_include_directories(test_cascade_resolution PRIVATE
    ${CMAKE_SOURCE_DIR}/src/engine/include
    ${CMAKE_SOURCE_DIR}/src/engine
    ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(test_cascade_resolution PRIVATE m)
add_test(NAME test_cascade_resolution COMMAND test_cascade_resolution)

