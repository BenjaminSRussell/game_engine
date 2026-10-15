# Issue #340. Unit test for src/engine/environment/landscape/landscape_spline.c.
# Not add_engine_test: that macro links Engine, which does not build on
# origin/main (missing GLFW, then gpu_profiler.c:80). Compiles the unit directly.
add_executable(test_landscape_spline
    tests/landscape_spline_test.c
    src/engine/environment/landscape/landscape_spline.c
)
target_include_directories(test_landscape_spline PRIVATE
    ${CMAKE_SOURCE_DIR}/src/engine/include
    ${CMAKE_SOURCE_DIR}/src/engine
    ${CMAKE_SOURCE_DIR}/src
)
add_test(NAME test_landscape_spline COMMAND test_landscape_spline)
