# Issue #377: entity sync, delta packets, and relevancy culling.
# Not add_engine_test: that macro links Engine, which does not build
# (missing GLFW, then gpu_profiler.c). Compiles entity_replication.c directly.
add_executable(test_entity_replication
    tests/entity_replication_test.c
    src/engine/network/replication/entity_replication.c
)
target_include_directories(test_entity_replication PRIVATE
    ${CMAKE_SOURCE_DIR}/src/engine/include
    ${CMAKE_SOURCE_DIR}/src/engine
    ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(test_entity_replication PRIVATE m)
add_test(NAME test_entity_replication COMMAND test_entity_replication)
