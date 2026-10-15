# Animation state machine unit tests (sub-machines, state time, blending, debug info).
# Not add_engine_test: that macro links Engine, which does not build here
# (missing GLFW, then gpu_profiler.c). The unit is compiled directly.
add_executable(anim_state_machine_test
    tests/unit/anim_state_machine_test.c
    src/engine/animation/state_machines/anim_state_machine.c
)
target_include_directories(anim_state_machine_test PRIVATE
    ${CMAKE_SOURCE_DIR}/src/engine/include
    ${CMAKE_SOURCE_DIR}/src/engine
    ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(anim_state_machine_test PRIVATE m)
add_test(NAME anim_state_machine_test COMMAND anim_state_machine_test)
