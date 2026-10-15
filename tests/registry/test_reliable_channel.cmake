# Reliable-UDP send, in-order delivery, acks, and retransmission.
# Not add_engine_test: that macro links Engine, which does not build
# (missing GLFW, then gpu_profiler.c). Compiles reliable_channel.c directly.
add_executable(test_reliable_channel
    tests/reliable_channel_test.c
    src/engine/network/transport/reliable_channel.c
)
target_include_directories(test_reliable_channel PRIVATE
    ${CMAKE_SOURCE_DIR}/src/engine/include
    ${CMAKE_SOURCE_DIR}/src/engine
    ${CMAKE_SOURCE_DIR}/src
)
add_test(NAME test_reliable_channel COMMAND test_reliable_channel)
