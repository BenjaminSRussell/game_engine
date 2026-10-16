# Issue #331: unit test for src/engine/ecs/mass_system.c. Compiles the unit directly, not Engine,
# because Engine does not build on main. quat_core.c supplies quat_look_rotation and the
# quaternion helpers that mass_system.c calls.
add_executable(test_mass_system
    tests/unit/ecs/mass_system_test.c
    src/engine/ecs/mass_system.c
    src/engine/math/linear_algebra/quaternions/quat/quat_core.c)
target_include_directories(test_mass_system PRIVATE
    ${CMAKE_SOURCE_DIR}/src/engine/include
    ${CMAKE_SOURCE_DIR}/src/engine
    ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(test_mass_system PRIVATE m)
add_test(NAME test_mass_system COMMAND test_mass_system)
