# Issue #168: src/include/core/math/types.h must forward to the canonical
# src/engine/include/core/math/types.h. Header-only, so it does not use
# add_engine_test: that macro links Engine, which does not build on origin/main
# (missing GLFW, then gpu_profiler.c).
add_executable(test_math_types_forwarding
    tests/math_types_forwarding_test.c
)
add_test(NAME test_math_types_forwarding COMMAND test_math_types_forwarding)
