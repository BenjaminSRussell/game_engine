# Issue #433: every editor/ source that EditorApp builds must compile. Syntax-only, so this
# does not link Engine. The file list is EDITOR_APP_SOURCES from the root CMakeLists.txt.
add_test(
    NAME test_editor_sources_compile
    COMMAND ${CMAKE_C_COMPILER} -std=c11 -fsyntax-only
        -I${CMAKE_SOURCE_DIR}/editor
        -I${CMAKE_SOURCE_DIR}/src/engine/include
        -I${CMAKE_SOURCE_DIR}/src/engine
        -I${CMAKE_SOURCE_DIR}/src
        ${EDITOR_APP_SOURCES}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
