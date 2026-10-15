# Issue #133: asset_render_system hands visible, linked instances to the renderer
# callback. Engine does not build on this host, so the unit is compiled directly
# with the test (see tests/registry/README.md).
add_executable(test_asset_render_system
    ${CMAKE_SOURCE_DIR}/tests/unit/ecs/asset_render_system_test.c
    ${CMAKE_SOURCE_DIR}/src/engine/ecs/systems/asset_render_system.c
    ${CMAKE_SOURCE_DIR}/src/engine/math/linear_algebra/quaternions/quat/quat_core.c)
target_include_directories(test_asset_render_system PRIVATE
    ${CMAKE_SOURCE_DIR}/src/engine/include
    ${CMAKE_SOURCE_DIR}/src/engine
    ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(test_asset_render_system PRIVATE m)
add_test(NAME test_asset_render_system COMMAND test_asset_render_system)
