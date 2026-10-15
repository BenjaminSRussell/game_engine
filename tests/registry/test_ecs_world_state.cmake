# Issue #184: ECS archetype count, world save/load and system destruction.
# Not add_engine_test: that macro links Engine, which does not build on
# origin/main. ecs.c is compiled directly instead.
add_executable(test_ecs_world_state
    tests/unit/ecs/ecs_world_state_test.c
    src/engine/ecs/ecs.c
)
add_test(NAME test_ecs_world_state COMMAND test_ecs_world_state)
