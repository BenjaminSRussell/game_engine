# TCP address parsing, loopback send/receive, and socket stats.
# Not add_engine_test: that macro links Engine, which does not build
# (missing GLFW, then gpu_profiler.c). Compiles tcp_socket.c directly.
add_executable(test_tcp_socket
    tests/tcp_socket_test.c
    src/engine/network/transport/tcp_socket.c
)
target_include_directories(test_tcp_socket PRIVATE
    ${CMAKE_SOURCE_DIR}/src/engine/include
    ${CMAKE_SOURCE_DIR}/src/engine
    ${CMAKE_SOURCE_DIR}/src
)
add_test(NAME test_tcp_socket COMMAND test_tcp_socket)
set_tests_properties(test_tcp_socket PROPERTIES TIMEOUT 20)

