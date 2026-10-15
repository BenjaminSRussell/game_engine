# Bitstream delta snapshots: pack changed bytes, then apply them.
# Not add_engine_test: that macro links Engine, which does not build
# (missing GLFW, then gpu_profiler.c). Compiles delta_compression.c directly.
add_executable(test_delta_compression
    tests/delta_compression_test.c
    src/engine/network/replication/delta_compression.c
)
target_include_directories(test_delta_compression PRIVATE
    ${CMAKE_SOURCE_DIR}/src/engine/include
    ${CMAKE_SOURCE_DIR}/src/engine
    ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(test_delta_compression PRIVATE m)
add_test(NAME test_delta_compression COMMAND test_delta_compression)
