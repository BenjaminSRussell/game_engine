# Issue #316. Unit test for the legacy memory_allocator.c shim.
# add_engine_test links Engine, which does not build on origin/main, so this
# target compiles the shim and the unified allocator it forwards to directly.
add_executable(test_memory_allocator
    tests/unit/core/memory_allocator_test.c
    src/engine/core/legacy/memory_allocator.c
    src/engine/core/memory/unified_memory_allocator.c)
target_include_directories(test_memory_allocator PRIVATE
    ${CMAKE_SOURCE_DIR}/src/engine/include
    ${CMAKE_SOURCE_DIR}/src/engine/include/core)
target_link_libraries(test_memory_allocator PRIVATE Threads::Threads)
add_test(NAME test_memory_allocator COMMAND test_memory_allocator)
