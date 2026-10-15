# Issue #433: behavior test for the shared EditorApp context. Not add_engine_test: that
# macro links Engine, which does not build on origin/main. The unit is compiled directly,
# and the Engine symbols it calls are stubbed in tests/editor_context_test.c.
add_executable(test_editor_context
    tests/editor_context_test.c
    editor/editor_context.c
)
target_include_directories(test_editor_context PRIVATE
    ${CMAKE_SOURCE_DIR}/editor
    ${CMAKE_SOURCE_DIR}/src/engine/include
    ${CMAKE_SOURCE_DIR}/src/engine
    ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(test_editor_context PRIVATE m)
add_test(NAME test_editor_context COMMAND test_editor_context)
