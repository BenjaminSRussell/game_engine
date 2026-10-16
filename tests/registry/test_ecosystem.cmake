# Issue #401. Unit test for the ecosystem simulation (ecosystem.c). Engine does not build on
# origin/main, so this target compiles ecosystem.c directly, as test_widget_focus does.
add_executable(test_ecosystem
    tests/unit/world_building/ecosystem_test.c
    src/engine/scene/world_building/ecosystem/ecosystem.c
)
target_include_directories(test_ecosystem PRIVATE
    ${CMAKE_SOURCE_DIR}/src/engine/scene/world_building/ecosystem
)
target_link_libraries(test_ecosystem PRIVATE m)
add_test(NAME test_ecosystem COMMAND test_ecosystem)
