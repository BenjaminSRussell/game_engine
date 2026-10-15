# Multiplayer connect, auth, broadcast, and disconnect in network_impl.c.
# Not add_engine_test: that macro links Engine, which does not build
# (missing GLFW, then gpu_profiler.c). Compiles network_impl.c directly.
add_executable(test_network_impl
    tests/network_impl_test.c
    src/engine/network/multiplayer/network_impl.c
)
target_include_directories(test_network_impl PRIVATE
    ${CMAKE_SOURCE_DIR}/src/engine/include
    ${CMAKE_SOURCE_DIR}/src/engine
    ${CMAKE_SOURCE_DIR}/src
)
add_test(NAME test_network_impl COMMAND test_network_impl)
