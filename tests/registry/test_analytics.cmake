# Issue #407: unit test for src/engine/tools/analytics.c. Not add_engine_test: that macro links
# Engine, which does not build on origin/main (missing GLFW, then gpu_profiler.c:80). The unit
# is compiled directly. The test stubs unified_logger_log, the entry point LOG_INFO expands to.
add_executable(test_analytics
    tests/unit/tools/analytics_test.c
    src/engine/tools/analytics.c
)
target_include_directories(test_analytics PRIVATE
    ${CMAKE_SOURCE_DIR}/src/engine/include
    ${CMAKE_SOURCE_DIR}/src/engine
    ${CMAKE_SOURCE_DIR}/src
)
add_test(NAME test_analytics COMMAND test_analytics)
