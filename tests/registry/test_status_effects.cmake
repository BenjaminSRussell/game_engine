# Issue #162: status_effects.c had no test coverage. Compiles the unit and
# damage_system.c directly, because add_engine_test links Engine, which does
# not build on main.
add_executable(test_status_effects
    tests/unit/gameplay/status_effects_test.c
    src/engine/gameplay/combat/status_effects.c
    src/engine/gameplay/combat/damage_system.c
)
target_include_directories(test_status_effects PRIVATE
    ${CMAKE_SOURCE_DIR}/src/engine/include
    ${CMAKE_SOURCE_DIR}/src/engine
    ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(test_status_effects PRIVATE m)
add_test(NAME test_status_effects COMMAND test_status_effects)
