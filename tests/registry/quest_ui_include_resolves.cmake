# Issue #438: quest_ui.h include must resolve to the real quest header.
# Not add_engine_test: that macro links Engine, which does not build
# (missing GLFW, then gpu_profiler.c:80).
add_test(
    NAME quest_ui_include_resolves
    COMMAND ${CMAKE_COMMAND}
        -DQUEST_UI_H=${CMAKE_SOURCE_DIR}/src/engine/gameplay/quest_ui.h
        -DCANONICAL_HEADER=${CMAKE_SOURCE_DIR}/src/engine/include/gameplay/quest_system.h
        -P ${CMAKE_SOURCE_DIR}/tests/check_quest_ui_include.cmake
)
