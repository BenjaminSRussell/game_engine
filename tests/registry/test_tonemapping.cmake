# Issue #230: tonemapping.c had no unit coverage. Not add_engine_test: that macro
# links Engine, which does not build on origin/main (missing GLFW, then
# gpu_profiler.c). The test includes the unit source so its static tone curves
# are reachable, and stubs the render graph, shader and logger calls it makes.
add_executable(test_tonemapping
    tests/unit/post_processing/tonemapping_test.c
)
target_include_directories(test_tonemapping PRIVATE
    ${CMAKE_SOURCE_DIR}/src/engine/include
    ${CMAKE_SOURCE_DIR}/src/engine
    ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(test_tonemapping PRIVATE m)
add_test(NAME test_tonemapping COMMAND test_tonemapping)
