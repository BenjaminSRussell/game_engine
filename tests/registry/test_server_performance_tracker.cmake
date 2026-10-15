# Server performance load, threshold health, history, and recovery.
# Not add_engine_test: that macro links Engine, which does not build on
# origin/main (missing GLFW, then gpu_profiler.c). Compiles the tracker
# directly and stubs the logger and clock.
add_executable(test_server_performance_tracker
    tests/server_performance_tracker_test.c
    src/engine/network/server_performance_tracker.c
)
target_include_directories(test_server_performance_tracker PRIVATE
    ${CMAKE_SOURCE_DIR}/src/engine/include
    ${CMAKE_SOURCE_DIR}/src/engine
    ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(test_server_performance_tracker PRIVATE m)
add_test(NAME test_server_performance_tracker COMMAND test_server_performance_tracker)
