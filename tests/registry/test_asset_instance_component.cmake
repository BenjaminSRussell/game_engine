# Issue #325. Not add_engine_test: that macro links Engine, which does not build
# on origin/main (missing GLFW, then gpu_profiler.c:80). This target compiles the
# component directly; ecs_register_component is stubbed in the test source.
add_executable(test_asset_instance_component
    tests/unit/ecs/asset_instance_component_test.c
    src/engine/ecs/components/asset_instance_component.c)
target_include_directories(test_asset_instance_component PRIVATE
    ${CMAKE_SOURCE_DIR}/src/engine/include
    ${CMAKE_SOURCE_DIR}/src/engine)
add_test(NAME test_asset_instance_component COMMAND test_asset_instance_component)
