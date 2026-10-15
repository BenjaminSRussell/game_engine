# Issue #347: damage calculation, queue limits, and health application.
# Not add_engine_test: that macro links Engine, which does not build on
# origin/main (missing GLFW, then gpu_profiler.c). Compiles damage_system.c
# directly and stubs ECS.
add_executable(test_damage_system
    tests/damage_system_test.c
    src/engine/gameplay/combat/damage_system.c
)
target_include_directories(test_damage_system PRIVATE
    ${CMAKE_SOURCE_DIR}/src/engine/include
    ${CMAKE_SOURCE_DIR}/src/engine
    ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(test_damage_system PRIVATE m)
add_test(NAME test_damage_system COMMAND test_damage_system)
