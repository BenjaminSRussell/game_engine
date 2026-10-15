# Homing projectiles aim at the target entity's transform, not the origin.
# Not add_engine_test: that macro links Engine, which does not build on
# origin/main (missing GLFW, then gpu_profiler.c). Compiles projectile.c
# directly and stubs ECS, hitboxes, damage events, and status effects.
add_executable(test_projectile_homing
    tests/projectile_homing_test.c
    src/engine/gameplay/combat/projectile.c
)
target_include_directories(test_projectile_homing PRIVATE
    ${CMAKE_SOURCE_DIR}/src/engine/include
    ${CMAKE_SOURCE_DIR}/src/engine
    ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(test_projectile_homing PRIVATE m)
add_test(NAME test_projectile_homing COMMAND test_projectile_homing)
