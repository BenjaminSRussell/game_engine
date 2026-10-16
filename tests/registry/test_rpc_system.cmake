# Handler registration, dispatch by name, received-packet dispatch, and response
# callbacks in rpc_system.c (issue #238).
# Not add_engine_test: that macro links Engine, which does not build on origin/main
# (missing GLFW, then gpu_profiler.c). Compiles rpc_system.c and packet.c directly;
# the logger and the network send functions are stubbed in the test.
add_executable(test_rpc_system
    tests/rpc_system_test.c
    src/engine/network/packet.c
    src/engine/network/rpc_system.c
)
target_include_directories(test_rpc_system PRIVATE
    ${CMAKE_SOURCE_DIR}/src/engine/include
    ${CMAKE_SOURCE_DIR}/src/engine
    ${CMAKE_SOURCE_DIR}/src
)
add_test(NAME test_rpc_system COMMAND test_rpc_system)
