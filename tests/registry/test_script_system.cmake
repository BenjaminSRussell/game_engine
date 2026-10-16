# Issue #147: script_system.c was excluded from Engine for a "syntax error" that was
# already fixed in 51b25130. Engine does not build on main, so this target compiles the
# unit directly and checks the command and variable behaviour its descriptions promise.
add_executable(test_script_system
    tests/unit/scripting/script_system_test.c
    src/engine/scripting/script_system.c
)
target_include_directories(test_script_system PRIVATE
    ${CMAKE_SOURCE_DIR}/src/engine/include
    ${CMAKE_SOURCE_DIR}/src/engine
    ${CMAKE_SOURCE_DIR}/src
)
add_test(NAME test_script_system COMMAND test_script_system)
