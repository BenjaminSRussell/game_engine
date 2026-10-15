# Issue #249: unit tests for src/engine/environment/landscape_spline.c.
# Not add_engine_test: that macro links Engine, which does not build on
# origin/main (missing GLFW, then gpu_profiler.c:80). The unit is compiled
# directly, as the test_widget_focus block in the root CMakeLists.txt does.
add_executable(test_landscape_spline
    tests/unit/environment/landscape_spline_test.c
    src/engine/environment/landscape_spline.c
)
target_include_directories(test_landscape_spline PRIVATE
    ${CMAKE_SOURCE_DIR}/src/engine/include
    ${CMAKE_SOURCE_DIR}/src/engine
    ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(test_landscape_spline PRIVATE m)
add_test(NAME test_landscape_spline COMMAND test_landscape_spline)
