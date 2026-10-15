# Issue #185. Compiles ecs_advanced_impl.c directly, not add_engine_test: that
# macro links Engine, which does not build on origin/main (missing GLFW, then
# gpu_profiler.c:80).
add_executable(test_ecs_advanced_serialize
    tests/unit/ecs/ecs_advanced_impl_serialize_test.c
    src/engine/ecs/ecs_advanced_impl.c
)
add_test(NAME test_ecs_advanced_serialize COMMAND test_ecs_advanced_serialize)
