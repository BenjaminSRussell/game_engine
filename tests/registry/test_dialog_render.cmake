# Dialog shell rendering through the registered draw backend (dialog_render_impl).
# Not add_engine_test: that macro links Engine, which does not build on
# origin/main (missing GLFW, then gpu_profiler.c). Compiles dialog.c and its
# widget, button, label, text-field and modal collaborators directly.
add_executable(test_dialog_render
    tests/unit/ui/dialog_render_test.c
    src/engine/ui/widgets/dialog.c
    src/engine/ui/widgets/widget.c
    src/engine/ui/widgets/ui_button.c
    src/engine/ui/widgets/ui_label.c
    src/engine/ui/widgets/text_field.c
    src/engine/ui/modal_manager.c
)
target_include_directories(test_dialog_render PRIVATE
    ${CMAKE_SOURCE_DIR}/src/engine/include
    ${CMAKE_SOURCE_DIR}/src/engine
    ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(test_dialog_render PRIVATE m)
add_test(NAME test_dialog_render COMMAND test_dialog_render)
