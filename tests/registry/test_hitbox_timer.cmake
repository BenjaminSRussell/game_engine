# Issue #119. Temporary hitboxes are destroyed when their lifetime TimerComponent expires.
# Not add_engine_test: that macro links Engine, which does not build on origin/main
# (missing GLFW, then gpu_profiler.c). Compiles hitbox.c directly; the ECS, logger and
# damage calls it makes are stubbed in the test source.
add_executable(test_hitbox_timer
    tests/hitbox_timer_test.c
    src/engine/gameplay/combat/hitbox.c
)
target_include_directories(test_hitbox_timer PRIVATE
    ${CMAKE_SOURCE_DIR}/src/engine/include
    ${CMAKE_SOURCE_DIR}/src/engine
)
target_link_libraries(test_hitbox_timer PRIVATE m)
add_test(NAME test_hitbox_timer COMMAND test_hitbox_timer)
