# Issue #253. Compiles init_sequence.c directly, with stubs for the platform,
# subsystem registry and error recovery calls it makes. Not add_engine_test:
# that macro links Engine, which does not build on origin/main (missing GLFW,
# then gpu_profiler.c:80).
add_executable(init_sequence_test
    tests/unit/core/initialization/init_sequence_test.c
    src/engine/core/initialization/private/init_sequence.c
)
target_include_directories(init_sequence_test PRIVATE
    ${CMAKE_SOURCE_DIR}/src/engine/include
    ${CMAKE_SOURCE_DIR}/src/engine
)
add_test(NAME init_sequence_test COMMAND init_sequence_test)
