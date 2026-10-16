# Issue #320: unit tests for src/engine/core/string_utils.c, which had no coverage.
# Not add_engine_test: that macro links Engine, which does not build on origin/main.
# Compiles the unit directly, as test_widget_focus does.
add_executable(test_string_utils
    tests/unit/core/string_utils_test.c
    src/engine/core/string_utils.c
)
target_include_directories(test_string_utils PRIVATE
    ${CMAKE_SOURCE_DIR}/src/engine/include
    ${CMAKE_SOURCE_DIR}/src/engine
    ${CMAKE_SOURCE_DIR}/src
)
add_test(NAME test_string_utils COMMAND test_string_utils)
# A regression in the resize loop of string_format could spin forever.
set_tests_properties(test_string_utils PROPERTIES TIMEOUT 60)
