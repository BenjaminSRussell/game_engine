# Issue #122. add_engine_test links Engine, which does not build on origin/main,
# so this target compiles ui_label.c and widget.c directly (same pattern as
# test_widget_focus in the root CMakeLists.txt).
add_executable(test_ui_label
    tests/unit/ui/ui_label_test.c
    src/engine/ui/widgets/ui_label.c
    src/engine/ui/widgets/widget.c)
add_test(NAME test_ui_label COMMAND test_ui_label)
