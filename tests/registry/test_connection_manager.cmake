# Connection lifecycle, info, timeout, and server bind.
# Not add_engine_test: that macro links Engine, which does not build on
# origin/main (missing GLFW, then gpu_profiler.c). Compiles
# connection_manager.c directly.
add_executable(test_connection_manager
    tests/connection_manager_test.c
    src/engine/network/connection_manager.c
)
target_include_directories(test_connection_manager PRIVATE
    ${CMAKE_SOURCE_DIR}/src/engine/include
    ${CMAKE_SOURCE_DIR}/src/engine
    ${CMAKE_SOURCE_DIR}/src
)
target_compile_definitions(test_connection_manager PRIVATE _GNU_SOURCE)
target_link_libraries(test_connection_manager PRIVATE Threads::Threads)
add_test(NAME test_connection_manager COMMAND test_connection_manager)

