# Issue #195: bar and line draw calls from analytics_dashboard.c.
# Not add_engine_test: that macro links Engine, which does not build on
# origin/main (missing GLFW, then gpu_profiler.c:80). Compiles the unit directly.
add_executable(test_analytics_dashboard_chart
    tests/unit/tools/analytics_dashboard_test.c
    src/engine/tools/analytics_dashboard.c
)
target_include_directories(test_analytics_dashboard_chart PRIVATE
    ${CMAKE_SOURCE_DIR}/src/engine/include
    ${CMAKE_SOURCE_DIR}/src/engine
    ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(test_analytics_dashboard_chart PRIVATE m)
add_test(NAME test_analytics_dashboard_chart COMMAND test_analytics_dashboard_chart)
