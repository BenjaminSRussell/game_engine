# Issue #128. Shift + mouse wheel scrolls scroll_view horizontally.
# Not add_engine_test: that macro links Engine, which does not build on
# origin/main, so this compiles scroll_view.c and widget.c directly.
add_executable(test_scroll_view_shift
    tests/unit/ui/scroll_view_shift_test.c
    src/engine/ui/widgets/scroll_view.c
    src/engine/ui/widgets/widget.c)
target_include_directories(test_scroll_view_shift PRIVATE
    ${CMAKE_SOURCE_DIR}/src/engine/include
    ${CMAKE_SOURCE_DIR}/src/engine
    ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(test_scroll_view_shift PRIVATE m)
add_test(NAME test_scroll_view_shift COMMAND test_scroll_view_shift)
